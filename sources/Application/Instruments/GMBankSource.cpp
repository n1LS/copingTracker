/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "GMBankSource.h"
#include "GMBank_data.generated.h"

bool GMBankSource::lookup(int note, const SampleEntry **entry) const {
  if (note < 0 || note > 0xFF) {
    return false;
  }
  return GMBank::sampleForNote(instrument_, static_cast<uint8_t>(note), 0, entry);
}

int GMBankSource::GetLoopStart(int note) {
  const SampleEntry *entry;
  if (!lookup(note, &entry)) {
    return -1;
  }
  return static_cast<int>(entry->loopStart);
}

int GMBankSource::GetLoopEnd(int note) {
  const SampleEntry *entry;
  if (!lookup(note, &entry)) {
    return -1;
  }
  return static_cast<int>(entry->loopEnd);
}

int GMBankSource::GetSize(int note) {
  const SampleEntry *entry;
  if (!lookup(note, &entry)) {
    return 0;
  }
  return static_cast<int>(entry->length);
}

int GMBankSource::GetSampleRate(int note) {
  (void)note;
  // Runtime PCM sample rate is fixed at 22050 Hz for all GM bank entries (see
  // GMBank_data.generated.h).
  return 22050;
}

int GMBankSource::GetChannelCount(int note) {
  (void)note;
  return 1; // GM bank PCM data is mono
}

void *GMBankSource::GetSampleBuffer(int note) {
  const SampleEntry *entry;
  if (!lookup(note, &entry)) {
    return nullptr;
  }
  return const_cast<int16_t *>(pcmData + entry->pcmOffset);
}

int GMBankSource::GetRootNote(int note) {
  const SampleEntry *entry;
  if (!lookup(note, &entry)) {
    return note; // no entry for this note; fall back to played note (no transposition)
  }
  return entry->rootNote;
}

float GMBankSource::GetLengthInSec() {
  // This is a multi-sample source: the played length depends on which
  // note/entry is being triggered, so there is no single overall length to
  // report here.
  return 0.0f;
}

uint8_t GMBankSource::GetAttack(int note) {
  const SampleEntry *entry;
  if (!lookup(note, &entry)) {
    return 0;
  }
  return entry->attack;
}

uint8_t GMBankSource::GetDecay(int note) {
  const SampleEntry *entry;
  if (!lookup(note, &entry)) {
    return 0;
  }
  return entry->decay;
}

uint8_t GMBankSource::GetSustain(int note) {
  const SampleEntry *entry;
  if (!lookup(note, &entry)) {
    return 0xFF;
  }
  return entry->sustain;
}

uint8_t GMBankSource::GetRelease(int note) {
  const SampleEntry *entry;
  if (!lookup(note, &entry)) {
    return 0;
  }
  return entry->release;
}
