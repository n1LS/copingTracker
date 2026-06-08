/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "Phrase.h"
#include "Song.h"
#include "System/System/System.h"
#include <stdlib.h>
#include <string.h>

Phrase::Phrase() { Reset(); };

Phrase::~Phrase() {};

void Phrase::Reset() {
  static const uint8_t NO_COMMAND = static_cast<uint8_t>(static_cast<char>(FourCC::InstrumentCommandNone));

  for (int i = 0; i < PHRASE_COUNT * STEPS_PER_PHRASE; i++) {
    steps_[i].note   = NO_NOTE;
    steps_[i].instr  = NO_INSTRUMENT;
    steps_[i].cmd1   = NO_COMMAND;
    steps_[i].param1 = 0x00;
    steps_[i].cmd2   = NO_COMMAND;
    steps_[i].param2 = 0x00;
  }
  for (int i = 0; i < PHRASE_COUNT; i++) {
    isUsed_[i] = false;
  }
}

unsigned short Phrase::GetNext() {
  for (int i = 0; i < PHRASE_COUNT; i++) {
    if (!isUsed_[i]) {
      isUsed_[i] = true;
      return i;
    }
  }
  return NO_MORE_PHRASE;
}

void Phrase::SetUsed(unsigned char c) { isUsed_[c] = true; }

void Phrase::ClearAllocation() {
  for (int i = 0; i < PHRASE_COUNT; i++) {
    isUsed_[i] = false;
  }
}
