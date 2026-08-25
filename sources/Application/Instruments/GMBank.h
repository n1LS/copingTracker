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

  // Plain ADSR envelope (SF2 DAHDSR with delay/hold folded away).
  // attack, decay, release: 0-255 LUT indices for envelope coefficients
  // sustain: 0-255 sustain level (255 = full volume, 0 = silent)
  uint8_t attack;
  uint8_t decay;
  uint8_t sustain;
  uint8_t release;
};

struct PresetInfo {
  const char *name;
  uint16_t instrumentIndex;
  uint8_t bank;
  uint8_t preset;
  uint8_t attack;
  uint8_t decay;
  uint8_t sustain;
  uint8_t release;
};

class GMBank {

public:
  static bool sampleForNote(uint16_t instrument, uint8_t note, uint8_t velocity, const SampleEntry **entry);
  static const char *nameForPreset(int index);
};
