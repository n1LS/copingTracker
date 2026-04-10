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

#include "MidiOutDevice.h"

MidiOutDevice::MidiOutDevice(const char *name) {
  name_ = name;
}

MidiOutDevice::~MidiOutDevice() {};

const char *MidiOutDevice::GetName() {
  return name_.c_str();
}

void MidiOutDevice::SetName(const char *name) {
  name_ = name;
}

void MidiOutDevice::SendQueue(etl::vector<MidiMessage, MIDI_MAX_MESG_QUEUE> &queue) {
  for (auto &msg : queue) {
    SendMessage(msg);
  }
}
