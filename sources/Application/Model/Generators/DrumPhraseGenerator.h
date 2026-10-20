/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include <stdint.h>

namespace DrumGenerator {

constexpr uint8_t kMaxSteps = 16;
constexpr uint8_t kSoundCount = 12;
constexpr uint8_t kMaxPhrases = 4;

enum Sound : uint8_t {
  BassDrum = 0,
  RimShot,
  Snare,
  Clap,
  Snare2,
  LowTom,
  ClosedHat,
  MidTom,
  HatTap,
  HighTom,
  OpenHat,
  Crash
};

enum class Feel : uint8_t { Straight, Triplet };

enum class Style : uint8_t { Straight, Sparse, Syncopated, Broken, Driving, HalfTime };

// Instrument the generated phrases are meant to play on. Decides how the
// generated sounds are mapped to notes / instruments.
enum class Target : uint8_t { DrumInstrument, LSDJKit, SlicedSample, SampleGMSet };

enum class VoiceCount : uint8_t { Mono = 1, Duo = 2 };

using StepMask = uint16_t;

struct DrumTemplate {
  Style style;
  Feel feel;
  uint8_t steps;
  StepMask sound[kSoundCount];
};

struct DrumPattern {
  uint8_t steps;
  StepMask sound[kSoundCount];
};

struct Settings {
  Style style;
  Feel feel;
  uint8_t steps;       // 2..16
  uint8_t phraseCount; // 1..4
  uint8_t density;     // 0..255
  uint8_t variation;   // 0..255
  VoiceCount voices;
  Target target;
  uint8_t instrument; // instrument slot the phrases are written for
};

struct DrumChain {
  uint8_t phraseCount;
  DrumPattern phrase[kMaxPhrases];
};

class DrumPhraseGenerator {
public:
  static bool generate(const Settings &settings, DrumChain &out, uint32_t seed = 1);

private:
  static const DrumTemplate *selectTemplate(const Settings &settings, uint32_t &rng);

  static void varyHats(DrumPattern &pattern, const Settings &settings, uint32_t &rng);

  static void addFill(DrumPattern &pattern, const Settings &settings, bool stronger, uint32_t &rng);

  static void resolve(DrumPattern &pattern, VoiceCount voices);

  static uint8_t random8(uint32_t &rng);
  static bool chance(uint32_t &rng, uint8_t probability);
  static void clear(DrumPattern &pattern, uint8_t steps);
};

} // namespace DrumGenerator