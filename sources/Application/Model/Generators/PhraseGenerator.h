/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include <cstdint>

#include "Application/Model/Project.h"
#include "BasslineGenerator.h"
#include "DrumPhraseGenerator.h"
#include "System/Console/Trace.h"

using namespace DrumGenerator;

class PhraseGenerator {

  static int lcg_;

  // Prints every generated phrase as one line per sound, X for a hit and - for
  // none, prefixed with the sound's name padded to 8 characters.
  static void debugPrint(const DrumChain &chain) {
    static const char *kNames[kSoundCount] = {"bassdrum", "rim     ", "snare   ", "clap    ", "snare2  ", "lowtom  ",
                                              "hat-clsd", "midtom  ", "hat-tap ", "hightom ", "hat-open", "crash   "};

    for (uint8_t i = 0; i < chain.phraseCount; ++i) {
      const DrumPattern &pattern = chain.phrase[i];
      Trace::Debug("phrase %d (%d steps)", i, pattern.steps);

      for (uint8_t sound = 0; sound < kSoundCount; ++sound) {
        char line[kMaxSteps + 1];
        for (uint8_t step = 0; step < pattern.steps && step < kMaxSteps; ++step) {
          line[step] = (pattern.sound[sound] & (1u << step)) ? 'X' : '-';
        }
        line[pattern.steps < kMaxSteps ? pattern.steps : kMaxSteps] = 0;
        Trace::Debug("%s %s", kNames[sound], line);
      }
    }
  }

public:
  // Writes one generated bassline phrase per chain step (at most 4) with the
  // settings' instrument. Rests clear note and instrument of a step.
  static void GenerateBasslinePhrases(Project *project, ChainStep *phraseIndexes, uint8_t length,
                                      BasslineGenerator::Settings settings) {
    if (length > BasslineGenerator::kMaxPhrases) {
      length = BasslineGenerator::kMaxPhrases;
    }
    if (length == 0) {
      return;
    }
    settings.phraseCount = length;
    BasslineGenerator::Chain out;

    lcg_ = lcg_ * 1664525 + 1013904223;
    if (!BasslineGenerator::generate(settings, out, lcg_)) {
      return;
    }

    for (uint8_t i = 0; i < out.phraseCount; ++i) {
      const uint8_t phrase = phraseIndexes[i].phrase;
      for (uint8_t step = 0; step < BasslineGenerator::kSteps; ++step) {
        PhraseStep &s = project->song_.phrase_.steps_[phrase][step];
        const uint8_t note = out.note[i][step];
        s.note = note == BasslineGenerator::kNoNote ? NO_NOTE : note;
        s.instrument = note == BasslineGenerator::kNoNote ? NO_INSTRUMENT : settings.instrument;
      }
    }
  }

  static void GenerateDrumPhrases(Project *project, ChainStep *phraseIndexes, uint8_t length, Settings settings) {
    settings.phraseCount = length;
    settings.voices = (settings.target == Target::LSDJKit) ? VoiceCount::Duo : VoiceCount::Mono;
    DrumChain out;

    lcg_ = lcg_ * 1664525 + 1013904223;

    DrumGenerator::DrumPhraseGenerator::generate(settings, out, lcg_);

    // debugPrint(out); <-- check the output visually

    // LSDJ kits follow a different general ordering than gm kits: the typical
    // first 8 samples are bd, snare, closed hat, open hat, hi tom, mid tom, low
    // tom, crash. The remaining sounds follow in generator order.
    // Indexed by Sound, the value is the 0 based sample index in the kit.
    static const uint8_t LSDJKitMap[kSoundCount] = {
        /* BassDrum  */ 0,
        /* RimShot   */ 8,
        /* Snare     */ 1,
        /* Clap      */ 9,
        /* Snare2    */ 10,
        /* LowTom    */ 6,
        /* ClosedHat */ 2,
        /* MidTom    */ 5,
        /* HatTap    */ 11,
        /* HighTom   */ 4,
        /* OpenHat   */ 3,
        /* Crash     */ 7,
    };

    for (uint8_t i = 0; i < out.phraseCount && i < length; ++i) {
      const uint8_t phrase = phraseIndexes[i].phrase;
      const DrumPattern &pattern = out.phrase[i];

      for (uint8_t step = 0; step < pattern.steps; step++) {
        const StepMask mask = static_cast<StepMask>(1u << step);

        project->song_.phrase_.steps_[phrase][step].note = NO_NOTE;
        project->song_.phrase_.steps_[phrase][step].instrument = NO_INSTRUMENT;

        int voices = 0;

        for (uint8_t sound = 0; sound < kSoundCount; ++sound) {
          if (pattern.sound[sound] & mask) {
            project->song_.phrase_.steps_[phrase][step].instrument = settings.instrument;

            if (settings.target == Target::LSDJKit) {
              // A note holds two samples, note / 15 and note % 15, where 0 is
              // "no sample" and n is the kit's sample n - 1. At most two
              // sounds fit in a step.
              const int sample = LSDJKitMap[sound] + 1;
              if (voices == 0) {
                project->song_.phrase_.steps_[phrase][step].note = sample * 15; // left
              } else if (voices == 1) {
                project->song_.phrase_.steps_[phrase][step].note += sample; // right
              }
              voices++;
            } else if (settings.target == Target::SlicedSample) {
              // single slices, no note offset
              project->song_.phrase_.steps_[phrase][step].note = sound;
            } else {
              // gm instrument or DrumInstrument, use note at C3 offset
              project->song_.phrase_.steps_[phrase][step].note = 36 + sound;
            }
          }
        }
      }
    }
  }
};
