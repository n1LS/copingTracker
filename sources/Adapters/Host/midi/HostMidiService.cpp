/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "HostMidiService.h"

HostMidiInDevice::HostMidiInDevice() : MidiInDevice("Host MIDI In") {
}

HostMidiInDevice::~HostMidiInDevice() {
}

HostMidiOutDevice::HostMidiOutDevice() : MidiOutDevice("Host MIDI Out") {
}

HostMidiOutDevice::~HostMidiOutDevice() {
}

HostMidiService::HostMidiService() : inDevice_(), outDevice_() {
}

HostMidiService::~HostMidiService() {
}
