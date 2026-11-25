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

#include "Mixer.h"

Mixer::Mixer() : Persistent(JSON_NODE_MIXER) {
  Clear();
};

Mixer::~Mixer() {};

void Mixer::Clear() {

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    channelBus_[i] = i;
  }
}

void Mixer::SaveContent(JsonWriter &) {
}

void Mixer::RestoreContent(JsonReader &reader) {
  while (reader.NextKey()) {
    reader.Skip();
  }
}
