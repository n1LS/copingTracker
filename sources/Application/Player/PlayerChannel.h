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

#ifndef _PLAYER_CHANNEL_H_
#define _PLAYER_CHANNEL_H_

#include "Application/Instruments/I_Instrument.h"
#include "Application/Mixer/MixBus.h"
#include "Services/Audio/AudioModule.h"

class PlayerChannel : public AudioModule {
public:
  PlayerChannel(int index);
  virtual ~PlayerChannel();
  virtual bool Render(fixed *buffer, int samplecount);
  void StartInstrument(I_Instrument *instr, unsigned char note, uint8_t volume, bool newInstrument);
  void StopInstrument();
  I_Instrument *GetInstrument();
  void SetMute(bool muted);
  bool IsMuted();
  void SetMixBus(int i);
  void Reset();

private:
  int index_;
  I_Instrument *instr_;
  bool muted_;
  int busIndex_;
  MixBus *mixBus_;
};

#endif