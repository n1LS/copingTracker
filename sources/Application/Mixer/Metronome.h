/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _METRONOME_H_
#define _METRONOME_H_

#include "Application/Utils/fixed.h"
#include "Services/Audio/AudioModule.h"
#include "config/AudioConstants.h"
#include <stdint.h>

// Click generator: a short triangle beep. Trigger() is called from the player
// when a step starts, Render() writes the beep into the buffers that follow, it
// returns false (no data) when idle.
class Metronome : public AudioModule {
public:
  static constexpr uint32_t AccentSamples = SAMPLE_RATE_HZ * 10 / 1000; // 10ms
  static constexpr uint32_t StepSamples = SAMPLE_RATE_HZ * 5 / 1000;    // 5ms
  static constexpr int AccentLevel = 14000;                             // 16 bit domain
  static constexpr int StepLevel = 9000;
  // phase increment per sample: 65536 * freq / SAMPLE_RATE_HZ
  static constexpr uint16_t AccentInc = 65536ull * 1000 / SAMPLE_RATE_HZ; // ~1000Hz
  static constexpr uint16_t StepInc = 65536ull * 800 / SAMPLE_RATE_HZ;    // ~800Hz
  static constexpr uint32_t FadeSamples = SAMPLE_RATE_HZ / 1000;          // 1ms fade out

  Metronome() : remaining_(0), level_(0), inc_(0), phase_(0) {
  }
  virtual ~Metronome() {};

  // accent: primary click (first step of a phrase), otherwise the secondary one
  void Trigger(bool accent) {
    level_ = accent ? AccentLevel : StepLevel;
    inc_ = accent ? AccentInc : StepInc;
    phase_ = 0xC000; // zero crossing of the triangle
    remaining_ = accent ? AccentSamples : StepSamples;
  }

  void Clear() {
    remaining_ = 0;
  }

  virtual bool Render(fixed *buffer, int samplecount) override {
    uint32_t left = remaining_;
    if (left == 0) {
      return false;
    }
    int n = (left < (uint32_t)samplecount) ? (int)left : samplecount;
    int level = level_;
    uint16_t inc = inc_;
    uint16_t phase = phase_;
    fixed *dst = buffer;
    for (int i = 0; i < n; i++) {
      phase += inc;
      int ramp = (int)(int16_t)(phase ^ 0x8000);              // -32768..32767
      int tri = ((ramp < 0 ? -ramp : ramp) << 1) - 32768;     // folded: -32768..32768
      int s = (tri * level) >> 15;                            // +-level
      uint32_t remain = left - i;
      if (remain < FadeSamples) {
        s = (int)((s * remain) / FadeSamples); // linear fade out, avoids a click at the end
      }
      fixed v = i2fp(s);
      *dst++ = v;
      *dst++ = v;
    }
    phase_ = phase;
    // the rest of the buffer stays silent
    for (int i = n * 2; i < samplecount * 2; i++) {
      buffer[i] = 0;
    }
    remaining_ = left - n;
    return true;
  }

private:
  volatile uint32_t remaining_;
  volatile int level_;
  uint16_t inc_;
  uint16_t phase_;
};

#endif
