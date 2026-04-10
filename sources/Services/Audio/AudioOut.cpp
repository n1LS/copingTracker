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

#include "AudioOut.h"
#include "Application/Player/SyncMaster.h"

AudioOut::AudioOut() : AudioMixer("AudioOut"), sampleOffset_(0) {};

AudioOut::~AudioOut() {};

int AudioOut::getPlaySampleCount() {
  sampleOffset_ += SyncMaster::GetInstance()->GetPlaySampleCount();
  int count = int(sampleOffset_);
  sampleOffset_ -= count;
  return count;
}