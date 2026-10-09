/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _DELAY_EFFECT_H_
#define _DELAY_EFFECT_H_

#include "Application/Utils/fixed.h"
#include "Services/Audio/AudioDriver.h" // for MAX_SAMPLE_COUNT
#include "Services/Audio/AudioModule.h"
#include "config/AudioConstants.h"
#include <stdint.h>

// Single instance send effect: every channel whose instrument routes its
// output to the delay adds its (pre fader) signal to the summing input via
// AddSend(), Render() then outputs the wet signal only. The dry signal stays
// on the channel's own bus.
//
// The delay line is mono, runs at half the engine rate (22050Hz) and stores
// 8 bit samples, so the 10k line holds ~464ms.
//
// Render() relies on being called after all AddSend() calls of the same
// buffer, ie. the delay's bus must be added to the master after the channel
// buses.
class DelayEffect : public AudioModule {
public:
  static constexpr int LineSize = 16384;

  // Longest delay in ticks that still fits the line at the given tempo. A
  // tick is SAMPLE_RATE_HZ * 60 * 2 / (tempo * 8 * AUDIO_SLICES_PER_STEP)
  // samples (see SyncMaster::SetTempo), the line holds LineSize * 2 of them.
  static constexpr int MaxTicks(int tempo) {
    return (int)(((int64_t)LineSize * 2 * tempo * 8 * AUDIO_SLICES_PER_STEP) / ((int64_t)SAMPLE_RATE_HZ * 60 * 2));
  }

  DelayEffect();
  virtual ~DelayEffect() {};

  // buffer is interleaved stereo, samplecount the number of sample pairs
  void AddSend(const fixed *buffer, int samplecount);

  virtual bool Render(fixed *buffer, int samplecount);

  // delaySlots: delay length in 22050Hz samples (1..LineSize)
  // feedback  : 0..255 (0..~1.0)
  // wet       : output level, |wet| <= FP_ONE
  void SetParameters(int delaySlots, int feedback, fixed wet);

  // silences the delay line and the pending input
  void Clear();

private:
  int readPos() {
    int p = writePos_ - delaySlots_;
    return (p < 0) ? p + LineSize : p;
  }

  int8_t line_[LineSize];
  // summing input, already mono and downsampled, in the 16 bit domain. The
  // last slot of a buffer can be half filled when the buffer has an odd
  // sample count, it's carried over to the next buffer.
  int32_t send_[MAX_SAMPLE_COUNT / 2 + 2];

  int writePos_;
  int delaySlots_;
  int feedback_;
  fixed wet_;

  // 1 if the next incoming sample is the second of a 22050Hz slot
  int phase_;
  // last sample read from the line, used for the linear upsampling
  int8_t cur_;
  // true if any channel fed the input during the current buffer
  bool active_;
  // consecutive silent writes, once a whole line worth is silent we can stop
  // rendering
  int silentSlots_;
};

#endif
