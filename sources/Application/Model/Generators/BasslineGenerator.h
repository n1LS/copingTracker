/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include <stdint.h>

namespace BasslineGenerator {

constexpr uint8_t kSteps = 16;
constexpr uint8_t kMaxPhrases = 4;
constexpr uint8_t kChordSlots = 4; // one per quarter note
constexpr uint8_t kCarry = 0xFF;   // chord slot value: keep the previous chord
constexpr uint8_t kNoNote = 0xFF;
constexpr uint8_t kLowestNote = 28;  // E1
constexpr uint8_t kHighestNote = 55; // G3

enum class Style : uint8_t { Funky, Octave, Pulse, Count };

enum class Scale : uint8_t { Minor, Major, PentMinor, PentMajor, Dorian, Blues, Count };

struct Settings {
  Style style;
  uint8_t key; // 0..11, C = 0
  Scale scale;
  uint8_t phraseCount; // 1..4
  uint8_t density;     // 0..255
  uint8_t variation;   // 0..255, how much later phrases differ from the first
  uint8_t movement;    // 0..255
  uint8_t instrument;  // instrument slot the phrases are written for
  // Scale degree per phrase and quarter note, kCarry continues the previous
  // slot. A carry in the very first slot means the tonic.
  uint8_t chords[kMaxPhrases][kChordSlots];
};

struct Chain {
  uint8_t phraseCount;
  uint8_t note[kMaxPhrases][kSteps]; // MIDI note or kNoNote
};

uint8_t scaleSize(Scale scale);

// "ii A", "V7 E", ... for the chord built on a degree of the scale, at most 9 chars + 0
void chordLabel(uint8_t key, Scale scale, uint8_t degree, char *out, uint8_t size);

bool generate(const Settings &settings, Chain &out, uint32_t seed = 1);

} // namespace BasslineGenerator
