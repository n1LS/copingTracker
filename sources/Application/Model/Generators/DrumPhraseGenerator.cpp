/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "DrumPhraseGenerator.h"
#include "DrumTemplates.h"

namespace DrumGenerator {

uint8_t DrumPhraseGenerator::random8(uint32_t &rng) {
  rng ^= rng << 13;
  rng ^= rng >> 17;
  rng ^= rng << 5;
  return static_cast<uint8_t>(rng >> 24);
}

bool DrumPhraseGenerator::chance(uint32_t &rng, uint8_t probability) {
  if (probability == 0)
    return false;
  if (probability == 255)
    return true;
  return random8(rng) < probability;
}

void DrumPhraseGenerator::clear(DrumPattern &pattern, uint8_t steps) {
  pattern.steps = steps;

  for (uint8_t i = 0; i < kSoundCount; ++i) {
    pattern.sound[i] = 0;
  }
}

const DrumTemplate *DrumPhraseGenerator::selectTemplate(const Settings &settings, uint32_t &rng) {
  uint8_t matches = 0;

  for (uint8_t i = 0; i < kDrumTemplateCount; ++i) {
    const DrumTemplate &t = kDrumTemplates[i];

    if (t.style == settings.style && t.feel == settings.feel && t.steps == settings.steps) {
      ++matches;
    }
  }

  if (!matches)
    return nullptr;

  uint8_t selected = random8(rng) % matches;

  for (uint8_t i = 0; i < kDrumTemplateCount; ++i) {
    const DrumTemplate &t = kDrumTemplates[i];

    if (t.style != settings.style || t.feel != settings.feel || t.steps != settings.steps) {
      continue;
    }

    if (selected-- == 0)
      return &t;
  }

  return nullptr;
}

void DrumPhraseGenerator::varyHats(DrumPattern &pattern, const Settings &settings, uint32_t &rng) {
  // Variation can remove existing closed hats or replace them
  // with open hats. The first step remains unchanged.
  for (uint8_t step = 1; step < pattern.steps; ++step) {
    const StepMask bit = static_cast<StepMask>(1u << step);

    if (!(pattern.sound[ClosedHat] & bit))
      continue;
    if (!chance(rng, settings.variation))
      continue;

    if (!chance(rng, settings.density)) {
      pattern.sound[ClosedHat] &= static_cast<StepMask>(~bit);
    } else if (chance(rng, 32)) {
      pattern.sound[ClosedHat] &= static_cast<StepMask>(~bit);
      pattern.sound[OpenHat] |= bit;
    }
  }

  // Density controls how likely additional hat taps are.
  const uint8_t tapChance = static_cast<uint8_t>((static_cast<uint16_t>(settings.density) * settings.variation) / 255);

  for (uint8_t step = 1; step < pattern.steps; ++step) {
    const StepMask bit = static_cast<StepMask>(1u << step);

    if (pattern.sound[ClosedHat] & bit)
      continue;
    if (pattern.sound[OpenHat] & bit)
      continue;
    if (pattern.sound[HatTap] & bit)
      continue;

    if (chance(rng, tapChance)) {
      pattern.sound[HatTap] |= bit;
    }
  }
}

void DrumPhraseGenerator::addFill(DrumPattern &pattern, const Settings &settings, bool stronger, uint32_t &rng) {
  uint8_t fillLength = pattern.steps / 4;

  if (fillLength < 1)
    fillLength = 1;
  if (fillLength > 4)
    fillLength = 4;

  const uint8_t start = pattern.steps - fillLength;

  for (uint8_t step = start; step < pattern.steps; ++step) {
    if (!stronger && step == start)
      continue;
    if (!chance(rng, settings.variation))
      continue;

    const StepMask bit = static_cast<StepMask>(1u << step);

    // Keep the kick untouched. Replace hats with toms.
    pattern.sound[ClosedHat] &= static_cast<StepMask>(~bit);
    pattern.sound[HatTap] &= static_cast<StepMask>(~bit);
    pattern.sound[OpenHat] &= static_cast<StepMask>(~bit);

    const uint8_t choice = random8(rng);

    if (choice < 96) {
      pattern.sound[LowTom] |= bit;
    } else if (choice < 192) {
      pattern.sound[MidTom] |= bit;
    } else {
      pattern.sound[HighTom] |= bit;
    }
  }

  // Phrase four gets a stronger snare pickup.
  const uint8_t last = pattern.steps - 1;
  const StepMask lastBit = static_cast<StepMask>(1u << last);

  if (stronger && !(pattern.sound[BassDrum] & lastBit) && chance(rng, settings.variation)) {
    pattern.sound[Snare] |= lastBit;
  }
}

static uint8_t soundPriority(uint8_t sound) {
  switch (sound) {
    case BassDrum:
      return 255;
    case Snare:
      return 240;
    case Crash:
      return 220;
    case Clap:
      return 200;
    case RimShot:
      return 190;
    case Snare2:
      return 180;
    case LowTom:
      return 160;
    case MidTom:
      return 150;
    case HighTom:
      return 140;
    case OpenHat:
      return 120;
    case HatTap:
      return 100;
    case ClosedHat:
      return 80;
    default:
      return 0;
  }
}

void DrumPhraseGenerator::resolve(DrumPattern &pattern, VoiceCount voices) {
  uint8_t maxVoices = static_cast<uint8_t>(voices);

  if (maxVoices < 1)
    maxVoices = 1;
  if (maxVoices > 2)
    maxVoices = 2;

  for (uint8_t step = 0; step < pattern.steps; ++step) {
    uint8_t selected[2] = {0, 0};
    uint8_t priorities[2] = {0, 0};
    uint8_t count = 0;

    for (uint8_t sound = 0; sound < kSoundCount; ++sound) {
      const StepMask bit = static_cast<StepMask>(1u << step);

      if (!(pattern.sound[sound] & bit))
        continue;

      const uint8_t priority = soundPriority(sound);

      if (count < maxVoices) {
        selected[count] = sound;
        priorities[count] = priority;
        ++count;
        continue;
      }

      uint8_t lowest = 0;

      for (uint8_t i = 1; i < count; ++i) {
        if (priorities[i] < priorities[lowest]) {
          lowest = i;
        }
      }

      if (priority > priorities[lowest]) {
        selected[lowest] = sound;
        priorities[lowest] = priority;
      }
    }

    const StepMask bit = static_cast<StepMask>(1u << step);

    for (uint8_t sound = 0; sound < kSoundCount; ++sound) {
      pattern.sound[sound] &= static_cast<StepMask>(~bit);
    }

    for (uint8_t i = 0; i < count; ++i) {
      pattern.sound[selected[i]] |= bit;
    }
  }
}

bool DrumPhraseGenerator::generate(const Settings &settings, DrumChain &out, uint32_t seed) {
  out.phraseCount = 0;

  if (settings.steps < 2 || settings.steps > kMaxSteps || settings.phraseCount < 1 ||
      settings.phraseCount > kMaxPhrases) {
    return false;
  }

  uint32_t rng = seed ? seed : 1;

  const DrumTemplate *base = selectTemplate(settings, rng);
  if (!base)
    return false;

  out.phraseCount = settings.phraseCount;

  for (uint8_t i = 0; i < settings.phraseCount; ++i) {
    DrumPattern &pattern = out.phrase[i];
    clear(pattern, settings.steps);

    // Copy the complete authored rhythm.
    for (uint8_t sound = 0; sound < kSoundCount; ++sound) {
      pattern.sound[sound] = base->sound[sound];
    }

    // Vary secondary percussion while retaining the kick backbone.
    varyHats(pattern, settings, rng);

    // Fill phrases two and four (zero-based indices 1 and 3).
    if (i == 1 || i == 3) {
      addFill(pattern, settings, i == 3, rng);
    }

    resolve(pattern, settings.voices);
  }

  return true;
}

} // namespace DrumGenerator