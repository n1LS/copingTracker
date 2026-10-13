/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include <cstdint>

#include "Application/Model/Project.h"
#include "DrumPhraseGenerator.h"

using namespace DrumGenerator;

class PhraseGenerator {

  static int lcg_;
  
  public:
    
    static void GenerateDrumPhrases(Project *project, ChainStep *phraseIndexes, uint8_t length, Settings settings) {
      settings.phraseCount = length;
      DrumChain out;

      lcg_ = lcg_ * 1664525 + 1013904223;

      DrumGenerator::DrumPhraseGenerator::generate(settings, out, lcg_);

      for (uint8_t i = 0; i < out.phraseCount && i < length; ++i) {
        const uint8_t phrase = phraseIndexes[i].phrase;
        const DrumPattern &pattern = out.phrase[i];

        for (uint8_t step = 0; step < pattern.steps; ++step) {
          const StepMask mask = static_cast<StepMask>(1u << step);

          project->song_.phrase_.steps_[phrase][step].note = NO_NOTE;
          project->song_.phrase_.steps_[phrase][step].instrument = NO_INSTRUMENT;

          for (uint8_t sound = 0; sound < kSoundCount; ++sound) {
            if (pattern.sound[sound] & mask) {
              project->song_.phrase_.steps_[phrase][step].note = 36 + sound;
              project->song_.phrase_.steps_[phrase][step].instrument = 0; // very very temp
            }
          }
        }
      }
    }

 };