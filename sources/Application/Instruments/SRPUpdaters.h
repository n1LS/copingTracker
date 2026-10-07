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

#ifndef _SRP_UPDATERS_H_
#define _SRP_UPDATERS_H_

#include "Foundation/Types/Types.h"
#include "I_SRPUpdater.h"

// Vol#ume envelope ramp
struct VolumeRamp {
  void SetData(float target, float speed, float start);
  void Trigger(bool tableTick);
  void UpdateSRP(struct RUParams &rup);

  fixed current_;
  fixed target_;
  fixed speed_;
};

// Filter cut-off ramp
struct FCRamp {
  void SetData(float target, float speed, float start);
  void Trigger(bool tableTick);
  void UpdateSRP(struct RUParams &rup);

  fixed current_;
  fixed target_;
  fixed speed_;
};

// Filter resonance ramp
struct FRRamp {
  void SetData(float target, float speed, float start);
  void Trigger(bool tableTick);
  void UpdateSRP(struct RUParams &rup);

  fixed current_;
  fixed target_;
  fixed speed_;
};

// Logarithmic speed/frequency ramp (legato, pitch fine tune)
struct LogSpeedRamp {
  void SetData(float target, float speed, float start);
  float GetCurrent();
  void Trigger(bool tableTick);
  void UpdateSRP(struct RUParams &rup);

  fixed current_;
  fixed target_;
  fixed speed_;
};

// Linear speed/frequency ramp (pitch slide)
struct LinSpeedRamp {
  void SetData(float target, float speed, float start);
  void Trigger(bool tableTick);
  void UpdateSRP(struct RUParams &rup);

  fixed current_;
  fixed target_;
  fixed speed_;
};

// Arpeggiator
struct Arp {
  void SetData(uint32_t data);
  void Trigger(bool tableTick);
  void UpdateSRP(struct RUParams &rup);

  fixed current_;
  uint8_t arp_[5];      // Arp setting
  uint8_t arpPosition_; // Position in the arpegiator
  uint8_t arpLength_;   // Length of arp data
};

// Stereo pan ramp
struct Panner {
  void SetData(float target, float speed, float start);
  void Trigger(bool tableTick);
  void UpdateSRP(struct RUParams &rup);

  fixed current_;
  fixed target_;
  fixed speed_;
};

// Vibrato LFO
struct Vibrato {
  void SetData(uint8_t rate, uint8_t depth);
  void Trigger(bool tableTick);
  void UpdateSRP(struct RUParams &rup);

  fixed current_;
  uint16_t phase_;
  uint16_t rate_;
  uint8_t depth_;
};

#endif
