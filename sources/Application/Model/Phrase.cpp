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

Phrase::Phrase() {
  Reset();
}

Phrase::~Phrase() {
}

void Phrase::Reset() {
  for (int i = 0; i < PHRASE_COUNT; i++) {
    for (int j = 0; j < STEPS_PER_PHRASE; j++) {
      steps_[i][j].note = NO_NOTE;
      steps_[i][j].setInstrument(NO_INSTRUMENT);
      steps_[i][j].setCmd1(Token::InstrumentCommandNone);
      steps_[i][j].param1 = 0x00;
      steps_[i][j].setCmd2(Token::InstrumentCommandNone);
      steps_[i][j].param2 = 0x00;
      steps_[i][j].setVolume(NO_VOLUME);
    }
    isUsed_[i] = false;
  }
}

bool Phrase::IsEmpty(uint8_t phrase) const {
  for (int i = 0; i < STEPS_PER_PHRASE; i++) {
    const PhraseStep &step = steps_[phrase][i];
    if (step.note != NO_NOTE || step.getInstrument() != NO_INSTRUMENT || step.getVolume() != NO_VOLUME ||
        step.getCmd1() != Token::InstrumentCommandNone || step.getCmd2() != Token::InstrumentCommandNone) {
      return false;
    }
  }
  return true;
}

uint16_t Phrase::GetNext() {
  for (int i = 0; i < PHRASE_COUNT; i++) {
    if (!isUsed_[i]) {
      isUsed_[i] = true;
      return i;
    }
  }
  return NO_MORE_PHRASE;
}

void Phrase::SetUsed(unsigned char c) {
  isUsed_[c] = true;
}

void Phrase::ClearAllocation() {
  for (int i = 0; i < PHRASE_COUNT; i++) {
    isUsed_[i] = false;
  }
}
