/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "picoTrackerUSBMidiOutDevice.h"
#include "Adapters/copingTracker/platform/platform.h"
#include "System/Console/Trace.h"
#include "pico/stdlib.h"
#include "usb_utils.h"

picoTrackerUSBMidiOutDevice::picoTrackerUSBMidiOutDevice(const char *name) : MidiOutDevice(name) {
}

bool picoTrackerUSBMidiOutDevice::Init() {
  return true;
}

void picoTrackerUSBMidiOutDevice::Close() {};

bool picoTrackerUSBMidiOutDevice::Start() {
  return true;
}

void picoTrackerUSBMidiOutDevice::Stop() {
}

void picoTrackerUSBMidiOutDevice::SendMessage(MidiMessage &msg) {
  uint8_t midicmd[3] = {0, 0, 0};

  midicmd[0] = msg.status_;
  if (msg.status_ < 0xF0) {
    midicmd[1] = msg.data1_;
    midicmd[2] = msg.data2_;
    sendUSBMidiMessage(midicmd, 3);
  } else {
    sendUSBMidiMessage(midicmd, 1);
  }
}