/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "picoTrackerMidiService.h"

picoTrackerMidiService::picoTrackerMidiService()
    : // Initialize static member variables with their respective names
      midiOutDevice_("MidiOut"), usbMidiOutDevice_("USB"), midiInDevice_("MidiIn"), usbMidiInDevice_("USBMidiIn") {
  // Add MIDI output devices to the output device list
  outList_.insert(outList_.end(), &midiOutDevice_);
  outList_.insert(outList_.end(), &usbMidiOutDevice_);

  // Add MIDI input devices to the input device list
  inList_.insert(inList_.end(), &midiInDevice_);
  inList_.insert(inList_.end(), &usbMidiInDevice_);
}

picoTrackerMidiService::~picoTrackerMidiService() {};

void picoTrackerMidiService::poll() {
  // Poll all MIDI input devices
  for (auto dev : inList_) {
    picoTrackerMidiInDevice *ptDev = (picoTrackerMidiInDevice *)dev;
    if (ptDev) {
      ptDev->poll();
    }
  }
}
