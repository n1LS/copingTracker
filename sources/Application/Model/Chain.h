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

  ChainStep steps_[CHAIN_COUNT][PHRASES_PER_CHAIN];

private:
  std::bitset<CHAIN_COUNT> isUsed_;
};

#endif
