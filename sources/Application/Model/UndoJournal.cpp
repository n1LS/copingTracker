/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "UndoJournal.h"
#include <string.h>

static_assert(UNDO_BUFFER_SIZE <= 0xFFFF, "the size of an action is 16 bit");

UndoJournal &UndoJournal::Instance() {
  static uint8_t buffer[UNDO_BUFFER_SIZE];
  static UndoJournal journal(buffer, sizeof(buffer));
  return journal;
}

UndoJournal::UndoJournal(uint8_t *buffer, size_t size) : buffer_(buffer), size_(size) {
}

void UndoJournal::write(size_t position, const void *data, size_t length) {
  const uint8_t *source = static_cast<const uint8_t *>(data);
  position = wrap(position);
  const size_t first = (size_ - position < length) ? size_ - position : length;
  memcpy(buffer_ + position, source, first);
  if (first < length) {
    memcpy(buffer_, source + first, length - first);
  }
}

void UndoJournal::read(size_t position, void *data, size_t length) const {
  uint8_t *destination = static_cast<uint8_t *>(data);
  position = wrap(position);
  const size_t first = (size_ - position < length) ? size_ - position : length;
  memcpy(destination, buffer_ + position, first);
  if (first < length) {
    memcpy(destination + first, buffer_, length - first);
  }
}

// the bytes in the ring and the memory trade places
void UndoJournal::swapMemory(size_t position, void *memory, size_t length) {
  uint8_t *bytes = static_cast<uint8_t *>(memory);
  for (size_t i = 0; i < length; i++) {
    uint8_t &slot = buffer_[wrap(position + i)];
    const uint8_t temporary = slot;
    slot = bytes[i];
    bytes[i] = temporary;
  }
}

uint64_t UndoJournal::readNumber(size_t position, size_t bytes) const {
  uint64_t value = 0;
  for (size_t i = 0; i < bytes; i++) {
    value |= static_cast<uint64_t>(buffer_[wrap(position + i)]) << (8 * i);
  }
  return value;
}

void UndoJournal::writeNumber(size_t position, uint64_t value, size_t bytes) {
  for (size_t i = 0; i < bytes; i++) {
    buffer_[wrap(position + i)] = static_cast<uint8_t>(value >> (8 * i));
  }
}

size_t UndoJournal::actionSize(size_t position) const {
  return static_cast<size_t>(readNumber(position, 2));
}

size_t UndoJournal::topActionStart(size_t &size) const {
  const size_t end = wrap(tail_ + undoBytes_);
  size = static_cast<size_t>(readNumber(wrap(end + size_ - FooterSize), 2));
  return wrap(end + size_ - size);
}

void UndoJournal::Clear() {
  tail_ = 0;
  used_ = 0;
  undoBytes_ = 0;
  haveLast_ = false;
  if (open_) {
    // what the open action records from here on starts a new history
    cutRedo_ = false;
    overflowed_ = false;
  }
}

bool UndoJournal::TakeOverflowed() {
  const bool reported = overflowReported_;
  overflowReported_ = false;
  return reported;
}

void UndoJournal::dropOldest() {
  const size_t size = actionSize(tail_);
  tail_ = wrap(tail_ + size);
  used_ -= size;
  undoBytes_ -= size;
  if (undoBytes_ == 0) {
    haveLast_ = false;
  }
}

bool UndoJournal::reserve(size_t bytes) {
  if (actionBytes_ + bytes + FooterSize > size_) {
    return false;
  }
  while (size_ - used_ < bytes + FooterSize) {
    if (undoBytes_ == 0) {
      return false;
    }
    dropOldest();
  }
  return true;
}

void UndoJournal::BeginAction(uint32_t key, uint32_t nowMs) {
  // an action that was left open is closed
  if (open_) {
    CommitAction(0);
  }
  open_ = true;
  cutRedo_ = false;
  overflowed_ = false;
  actionStart_ = 0;
  actionBytes_ = 0;
  actionKey_ = key;
  actionNow_ = nowMs;
}

bool UndoJournal::covered(size_t start, size_t recordsEnd, Address address, size_t length) const {
  size_t offset = HeaderSize;
  while (offset < recordsEnd) {
    const size_t recordLength = buffer_[wrap(start + offset)];
    const Address recordAddress = static_cast<Address>(readNumber(start + offset + 1, sizeof(Address)));
    if (recordAddress <= address && address + length <= recordAddress + recordLength) {
      return true;
    }
    offset += RecordOverhead + recordLength;
  }
  return false;
}

void UndoJournal::Record(const void *address, size_t length) {
  if (!open_ || overflowed_ || length == 0) {
    return;
  }

  // the first change of the action cuts off what could have been redone
  if (!cutRedo_) {
    cutRedo_ = true;
    used_ = undoBytes_;
    actionStart_ = wrap(tail_ + used_);
    actionBytes_ = 0;
    if (!reserve(HeaderSize)) {
      overflowed_ = true;
      return;
    }
    // the size and hint are written when the action is closed
    used_ += HeaderSize;
    actionBytes_ = HeaderSize;
  }

  Address from = reinterpret_cast<Address>(address);
  while (length > 0) {
    const size_t part = length > 255 ? 255 : length;

    // already recorded in this action: the first record has the bytes as they were
    if (!covered(actionStart_, actionBytes_, from, part)) {
      const size_t bytes = RecordOverhead + part;
      if (!reserve(bytes)) {
        overflowed_ = true;
        return;
      }
      const size_t position = actionStart_ + actionBytes_;
      buffer_[wrap(position)] = static_cast<uint8_t>(part);
      writeNumber(position + 1, from, sizeof(Address));
      write(position + 1 + sizeof(Address), reinterpret_cast<const void *>(from), part);
      buffer_[wrap(position + 1 + sizeof(Address) + part)] = static_cast<uint8_t>(part);
      actionBytes_ += bytes;
      used_ += bytes;
    }
    from += part;
    length -= part;
  }
}

void UndoJournal::CommitAction(uint32_t hint) {
  if (!open_) {
    return;
  }
  open_ = false;
  if (!cutRedo_) {
    // nothing changed
    return;
  }
  cutRedo_ = false;

  if (overflowed_) {
    // it did not fit: older entries can not be undone over data that changed without them
    overflowed_ = false;
    tail_ = 0;
    used_ = 0;
    undoBytes_ = 0;
    haveLast_ = false;
    overflowReported_ = true;
    return;
  }

  // the bytes were recorded but nothing changed (a value at its limit, ...): no entry
  bool changed = false;
  for (size_t offset = HeaderSize; !changed && offset < actionBytes_;) {
    const size_t length = buffer_[wrap(actionStart_ + offset)];
    const uint8_t *memory =
        reinterpret_cast<const uint8_t *>(static_cast<Address>(readNumber(actionStart_ + offset + 1, sizeof(Address))));
    for (size_t i = 0; i < length && !changed; i++) {
      changed = buffer_[wrap(actionStart_ + offset + 1 + sizeof(Address) + i)] != memory[i];
    }
    offset += RecordOverhead + length;
  }
  if (!changed) {
    used_ = undoBytes_;
    return;
  }

  // the same change again, soon: the first one holds everything that is needed
  if (haveLast_ && actionKey_ != 0 && actionKey_ == lastKey_ && actionNow_ - lastMs_ <= MergeWindowMs &&
      undoBytes_ > 0) {
    size_t previousSize;
    const size_t previousStart = topActionStart(previousSize);
    const size_t previousEnd = previousSize - FooterSize;
    bool all = true;
    size_t offset = HeaderSize;
    while (all && offset < actionBytes_) {
      const size_t length = buffer_[wrap(actionStart_ + offset)];
      const Address address = static_cast<Address>(readNumber(actionStart_ + offset + 1, sizeof(Address)));
      all = covered(previousStart, previousEnd, address, length);
      offset += RecordOverhead + length;
    }
    if (all) {
      used_ = undoBytes_;
      lastMs_ = actionNow_;
      return;
    }
  }

  const size_t total = actionBytes_ + FooterSize;
  writeNumber(actionStart_, total, 2);
  writeNumber(actionStart_ + 2, hint, 4);
  writeNumber(actionStart_ + actionBytes_, total, 2);
  used_ += FooterSize;
  undoBytes_ = used_;
  haveLast_ = true;
  lastKey_ = actionKey_;
  lastMs_ = actionNow_;
}

// swaps the records of the action with the memory they belong to, last to first for undo and first to last for redo
void UndoJournal::applyAction(size_t start, size_t size, bool backwards) {
  const size_t recordsEnd = size - FooterSize;
  if (!backwards) {
    size_t offset = HeaderSize;
    while (offset < recordsEnd) {
      const size_t length = buffer_[wrap(start + offset)];
      void *memory = reinterpret_cast<void *>(static_cast<Address>(readNumber(start + offset + 1, sizeof(Address))));
      swapMemory(start + offset + 1 + sizeof(Address), memory, length);
      offset += RecordOverhead + length;
    }
  } else {
    size_t offset = recordsEnd;
    while (offset > HeaderSize) {
      const size_t length = buffer_[wrap(start + offset - 1)];
      offset -= RecordOverhead + length;
      void *memory = reinterpret_cast<void *>(static_cast<Address>(readNumber(start + offset + 1, sizeof(Address))));
      swapMemory(start + offset + 1 + sizeof(Address), memory, length);
    }
  }
}

bool UndoJournal::Undo(uint32_t &hint) {
  // records that are open are part of the history first
  if (open_ && cutRedo_) {
    CommitAction(0);
  }
  if (undoBytes_ == 0) {
    return false;
  }
  size_t size;
  const size_t start = topActionStart(size);
  hint = static_cast<uint32_t>(readNumber(start + 2, 4));
  applyAction(start, size, true);
  undoBytes_ -= size;
  haveLast_ = false;
  return true;
}

bool UndoJournal::Redo(uint32_t &hint) {
  if (open_ && cutRedo_) {
    CommitAction(0);
  }
  if (used_ <= undoBytes_) {
    return false;
  }
  const size_t start = wrap(tail_ + undoBytes_);
  const size_t size = actionSize(start);
  hint = static_cast<uint32_t>(readNumber(start + 2, 4));
  applyAction(start, size, false);
  undoBytes_ += size;
  haveLast_ = false;
  return true;
}
