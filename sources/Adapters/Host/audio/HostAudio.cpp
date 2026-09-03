/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "HostAudio.h"
#include "HostAudioDriver.h"
#include "Services/Audio/AudioOutDriver.h"

HostAudio::HostAudio(AudioSettings &hints) : Audio(hints) {
}

HostAudio::~HostAudio() {
}

void HostAudio::Init() {
  AudioSettings audioSettings;
  audioSettings.bufferSize_ = 1024;
  audioSettings.preBufferCount_ = 2;

  alignas(HostAudioDriver) static char audioDriver[sizeof(HostAudioDriver)];
  HostAudioDriver *drv = new (audioDriver) HostAudioDriver(audioSettings);

  alignas(AudioOutDriver) static char audioOutDriver[sizeof(AudioOutDriver)];
  AudioOutDriver *out = new (audioOutDriver) AudioOutDriver(*drv);

  AddOutput(*out);

  if (!drv->Init()) {
    return;
  }

  if (!drv->Start()) {
    return;
  }
}

void HostAudio::Close() {
  // Audio::Close() is not implemented in the base class
  // Just stub this for now
}
