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

#ifndef _PHRASE_H_
#define _PHRASE_H_

#include "Foundation/Types/Types.h"

#define PHRASE_COUNT 0x80
#define NO_MORE_PHRASE 0x81
#define STEPS_PER_PHRASE 16

struct PhraseStep {
  uint8_t note;
  uint8_t instr;
  uint8_t cmd1;
  uint8_t cmd2;
  uint16_t param1;
  uint16_t param2;
};

class Phrase {
public:
  Phrase();
  ~Phrase();
  void Reset();
  unsigned short GetNext();
  bool IsUsed(uint8_t i) { return isUsed_[i]; };
  void SetUsed(uint8_t c);
  void ClearAllocation();

  inline FourCC getCmd1(int phrase, int step) const {
    return FourCC::enum_type(steps_[phrase * STEPS_PER_PHRASE + step].cmd1);
  }
  inline FourCC getCmd2(int phrase, int step) const {
    return FourCC::enum_type(steps_[phrase * STEPS_PER_PHRASE + step].cmd2);
  }
  inline void setCmd1(int phrase, int step, FourCC f) {
    steps_[phrase * STEPS_PER_PHRASE + step].cmd1 = f.raw();
  }
  inline void setCmd2(int phrase, int step, FourCC f) {
    steps_[phrase * STEPS_PER_PHRASE + step].cmd2 = f.raw();
  }

  PhraseStep steps_[PHRASE_COUNT * STEPS_PER_PHRASE];

private:
  bool isUsed_[PHRASE_COUNT];
};

#endif
