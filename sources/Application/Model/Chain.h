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

#ifndef _CHAIN_H_
#define _CHAIN_H_

#include "Application/Model/UndoJournal.h"
#include <bitset>
#include <stdint.h>

#define CHAIN_COUNT 0xFF
#define NO_MORE_CHAIN 0x100
#define PHRASES_PER_CHAIN 0x10

struct ChainStep {
  uint8_t phrase;
  uint8_t transpose;
};

class Chain {
public:
  Chain();
  ~Chain();
  void Reset();
  uint16_t GetNext();
  bool IsUsed(unsigned char i) {
    return isUsed_[i];
  };
  void SetUsed(unsigned char c);
  void ClearAllocation();

  // The steps. Everything that changes one goes through EditStep / EditSteps, they record the bytes in the undo
  // history before they hand out the memory (call them right before the change, not to look).
  const ChainStep &Step(int chain, int step) const {
    return steps_[chain][step];
  }
  const ChainStep *Steps(int chain) const {
    return steps_[chain];
  }
  ChainStep &EditStep(int chain, int step) {
    UndoJournal::RecordChange(&steps_[chain][step], sizeof(ChainStep));
    return steps_[chain][step];
  }
  // all PHRASES_PER_CHAIN steps of the chain
  ChainStep *EditSteps(int chain) {
    UndoJournal::RecordChange(steps_[chain], sizeof(steps_[chain]));
    return steps_[chain];
  }

private:
  friend class Song; // saves and loads the steps as they are

  ChainStep steps_[CHAIN_COUNT][PHRASES_PER_CHAIN];
  std::bitset<CHAIN_COUNT> isUsed_;
};

#endif
