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

#ifndef _MIX_BUS_H_
#define _MIX_BUS_H_

#include "Application/Instruments/Equalizer.h"
#include "Services/Audio/AudioMixer.h"

class MixBus : public AudioMixer {
public:
  MixBus() : AudioMixer("bus") {};
  virtual ~MixBus() {};
};

// The master bus, adds an equalizer on the final mix (after the master volume)
class MasterBus : public MixBus {
public:
  MasterBus() : on_(false) {
    eq_.reset();
    SetEqualizer(false, EQ_UNITY_GAIN, EQ_UNITY_GAIN, EQ_UNITY_GAIN);
  }

  // gains are 0..0xFF, 0x80 is unity
  void SetEqualizer(bool on, uint8_t low, uint8_t mid, uint8_t high) {
    on_ = on;
    eq_.set_low(low);
    eq_.set_mid(mid);
    eq_.set_high(high);
  }

  virtual bool Render(fixed *buffer, int samplecount) override {
    bool gotData = MixBus::Render(buffer, samplecount);
    if (gotData && on_) {
      // restart from silence so switching it on doesn't click on stale state
      if (!eq_.active) {
        eq_.reset();
        eq_.active = true;
      }
      eq_.process_buffer(buffer, samplecount);
    } else {
      eq_.active = false;
    }
    return gotData;
  }

private:
  bool on_;
  equalizer_t eq_;
};
#endif
