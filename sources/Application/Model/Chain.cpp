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

#include "Chain.h"
#include "Song.h"
#include "System/System/System.h"

Chain::Chain() {
  Reset();
}

Chain::~Chain() {
}

void Chain::Reset() {
  for (int i = 0; i < CHAIN_COUNT; i++) {
    for (int j = 0; j < PHRASES_PER_CHAIN; j++) {
      steps_[i][j].phrase = EMPTY_CHAIN_VALUE;
      steps_[i][j].transpose = 0x00;
    }
    isUsed_[i] = false;
  }
}

uint16_t Chain::GetNext() {
  for (int i = 0; i < CHAIN_COUNT; i++) {
    if (!isUsed_[i]) {
      isUsed_[i] = true;
      return i;
    }
  }
  return NO_MORE_CHAIN;
}

void Chain::SetUsed(unsigned char c) {
  isUsed_[c] = true;
}

void Chain::ClearAllocation() {
  for (int i = 0; i < CHAIN_COUNT; i++) {
    isUsed_[i] = false;
  }
}
