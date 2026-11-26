/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _UNDO_JOURNAL_H_
#define _UNDO_JOURNAL_H_

#include <stddef.h>
#include <stdint.h>

// The bytes the undo history may use, the ring is taken from RAM for good
#ifndef UNDO_BUFFER_SIZE
#define UNDO_BUFFER_SIZE 6144
#endif

// Undo / redo of everything in the song data (rows, chains, phrases, tables, grooves) that is plain memory.
//
// Before a piece of memory is changed its bytes are recorded (Record: address, length, the bytes as they are), the
// recorded blocks of one user action (a button press, with everything it triggers) are one entry of the history.
// Undo and redo are the same operation: the recorded bytes and the memory swap places, so redo needs no space of its
// own, and a new action simply cuts off whatever could still have been redone.
//
// The history is a ring of bytes. When it is full the oldest actions are dropped. An action that does not fit at all
// (bigger than the ring) clears the history, older entries could not be undone safely over data that changed without
// them.
//
//   action   = header (size u16, hint u32) records footer (size u16)
//   record   = length u8, address, the bytes, length u8
//
// The length is at both ends of a record and the size at both ends of an action, so both can be walked in both
// directions: undo swaps the records of an action last to first, redo first to last (blocks that overlap come out as
// they were then).
class UndoJournal {
public:
  UndoJournal(uint8_t *buffer, size_t size);

  // The history of the song data, in a buffer of UNDO_BUFFER_SIZE bytes.
  static UndoJournal &Instance();
  // What the data classes call before they change memory: nothing happens when no action is open (loading, the
  // player, ...) so only what the user does can be undone.
  static void RecordChange(const void *address, size_t length) {
    Instance().Record(address, length);
  }

  // Opens an action. Actions with the same key (not 0) that repeat on the same memory within MergeWindowMs, like a
  // value that is changed by several presses, are one entry of the history.
  void BeginAction(uint32_t key, uint32_t nowMs);
  // The bytes at address are about to change.
  void Record(const void *address, size_t length);
  // Closes the action, hint is handed back by Undo / Redo (the caller keeps where the change happened in it). An
  // action that recorded nothing is dropped.
  void CommitAction(uint32_t hint);
  bool ActionOpen() const {
    return open_;
  }

  bool CanUndo() const {
    return undoBytes_ > 0;
  }
  bool CanRedo() const {
    return used_ > undoBytes_ && !open_;
  }
  // false when there is nothing to undo / redo, hint is that of the action
  bool Undo(uint32_t &hint);
  bool Redo(uint32_t &hint);
  void Clear();

  size_t UsedBytes() const {
    return used_;
  }
  size_t Capacity() const {
    return size_;
  }
  // true once if the history was cleared because an action did not fit
  bool TakeOverflowed();

#ifdef UNDO_MERGE_WINDOW_MS
  static constexpr uint32_t MergeWindowMs = UNDO_MERGE_WINDOW_MS; // the self test of the history
#else
  static constexpr uint32_t MergeWindowMs = 800;
#endif

private:
  typedef uintptr_t Address;
  static constexpr size_t HeaderSize = 2 + 4;
  static constexpr size_t FooterSize = 2;
  static constexpr size_t RecordOverhead = 1 + sizeof(Address) + 1;

  size_t wrap(size_t position) const {
    return position >= size_ ? position - size_ : position;
  }
  void write(size_t position, const void *data, size_t length);
  void read(size_t position, void *data, size_t length) const;
  void swapMemory(size_t position, void *memory, size_t length);
  uint64_t readNumber(size_t position, size_t bytes) const;
  void writeNumber(size_t position, uint64_t value, size_t bytes);

  size_t actionSize(size_t position) const;
  void dropOldest();
  // makes room for bytes more in the open action
  bool reserve(size_t bytes);
  void applyAction(size_t start, size_t size, bool backwards);
  // is the range inside of one of the records of the action at start, whose records end recordsEnd bytes after it
  bool covered(size_t start, size_t recordsEnd, Address address, size_t length) const;
  // start of the action that ends at the top of the undoable history
  size_t topActionStart(size_t &size) const;

  uint8_t *buffer_;
  size_t size_;

  // the oldest action starts at tail_, used_ bytes from there on are filled: undoBytes_ of them are actions that can be
  // undone, the rest (and the action that is open) comes after
  size_t tail_ = 0;
  size_t used_ = 0;
  size_t undoBytes_ = 0;

  bool open_ = false;
  bool cutRedo_ = false;
  bool overflowed_ = false;
  bool overflowReported_ = false;
  size_t actionStart_ = 0; // position of the header of the open action
  size_t actionBytes_ = 0; // header and records so far
  uint32_t actionKey_ = 0;
  uint32_t actionNow_ = 0;

  // the last action that was committed, when it is still the top of the history
  bool haveLast_ = false;
  uint32_t lastKey_ = 0;
  uint32_t lastMs_ = 0;
};

#endif
