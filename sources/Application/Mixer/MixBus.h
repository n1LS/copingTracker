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

#include "Application/Instruments/Compressor.h"
#include "Application/Instruments/Equalizer.h"
#include "Services/Audio/AudioMixer.h"

class MixBus : public AudioMixer {
public:
  MixBus() : AudioMixer("bus") {};
  virtual ~MixBus() {};
};

// The master bus, runs a compressor and an equalizer on the summed mix, before
// the master volume is applied
class MasterBus : public MixBus {
public:
  MasterBus() : eqOn_(false), compOn_(false) {
    eq_.reset();
    comp_.reset();
    SetEqualizer(false, EQ_UNITY_GAIN, EQ_UNITY_GAIN, EQ_UNITY_GAIN);
    SetCompressor(false, COMP_DEFAULT_THRESHOLD, COMP_DEFAULT_RATIO, COMP_DEFAULT_RELEASE);
  }

  // gains are 0..0xFF, 0x80 is unity
  void SetEqualizer(bool on, uint8_t low, uint8_t mid, uint8_t high) {
    eqOn_ = on;
    eq_.set_low(low);
    eq_.set_mid(mid);
    eq_.set_high(high);
  }

  void SetCompressor(bool on, uint8_t threshold, uint8_t ratio, uint8_t release) {
    compOn_ = on;
    comp_.set(threshold, ratio, release);
  }

  virtual bool Render(fixed *buffer, int samplecount) override {
    bool gotData = MixBus::Render(buffer, samplecount);
    if (!gotData) {
      eq_.active = false;
      comp_.active = false;
    }
    return gotData;
  }

protected:
  virtual void PreFader(fixed *buffer, int samplecount) override {
    // restart from a clean state when switched on, so it doesn't click
    if (compOn_) {
      if (!comp_.active) {
        comp_.reset();
        comp_.active = true;
      }
      comp_.process_buffer(buffer, samplecount);
    } else {
      comp_.active = false;
    }

    if (eqOn_) {
      if (!eq_.active) {
        eq_.reset();
        eq_.active = true;
      }
      eq_.process_buffer(buffer, samplecount);
    } else {
      eq_.active = false;
    }
  }

private:
  bool eqOn_;
  bool compOn_;
  equalizer_t eq_;
  compressor_t comp_;
};
#endif
