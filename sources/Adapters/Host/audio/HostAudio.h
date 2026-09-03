/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef HOST_AUDIO_H_
#define HOST_AUDIO_H_

#include "Services/Audio/Audio.h"

class HostAudio : public Audio {
public:
  HostAudio(AudioSettings &hints);
  virtual ~HostAudio();

  virtual void Init() override;
  virtual void Close() override;
};

#endif
