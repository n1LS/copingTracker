/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef HOST_MIDI_SERVICE_H_
#define HOST_MIDI_SERVICE_H_

#include "Services/Midi/MidiInDevice.h"
#include "Services/Midi/MidiOutDevice.h"
#include "Services/Midi/MidiService.h"

class HostMidiInDevice : public MidiInDevice {
public:
  HostMidiInDevice();
  virtual ~HostMidiInDevice();

  virtual bool Start() override {
    return true;
  }
  virtual void Stop() override {
  }

protected:
  virtual bool initDriver() override {
    return true;
  }
  virtual bool startDriver() override {
    return true;
  }
  virtual void stopDriver() override {
  }
  virtual void closeDriver() override {
  }
  virtual void poll() override {
  }
};

class HostMidiOutDevice : public MidiOutDevice {
public:
  HostMidiOutDevice();
  virtual ~HostMidiOutDevice();

  virtual bool Init() override {
    return true;
  }
  virtual void Close() override {
  }
  virtual bool Start() override {
    return true;
  }
  virtual void Stop() override {
  }
  virtual void SendMessage(MidiMessage &msg) override {
  }
};

class HostMidiService : public MidiService {
public:
  HostMidiService();
  virtual ~HostMidiService();

private:
  HostMidiInDevice inDevice_;
  HostMidiOutDevice outDevice_;
};

#endif
