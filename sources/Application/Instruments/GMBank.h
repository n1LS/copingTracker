/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include <stdint.h>

struct SampleEntry {
  uint32_t pcmOffset;
  uint32_t length;
  uint32_t loopStart;
  uint32_t loopEnd;

  uint8_t rootNote;
  int8_t fineTune;

  uint16_t flags;

  uint8_t volume;

  // Plain ADSR envelope (SF2 DAHDSR with delay/hold folded away). attack,
  // decay and release are stage durations in milliseconds; sustain is the
  // held amplitude level, 0 (silent) .. 0xFFFF (full volume).
  uint16_t attack;
  uint16_t decay;
  uint16_t sustain;
  uint16_t release;
};

struct PresetInfo {
  const char *name;
  uint16_t instrumentIndex;
  uint8_t bank;
  uint8_t preset;
};

class GMBank {

public:
  static bool sampleForNote(uint16_t instrument, uint8_t note, uint8_t velocity, const SampleEntry **entry);
};
