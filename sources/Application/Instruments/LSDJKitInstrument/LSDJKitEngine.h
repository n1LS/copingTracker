/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include "Application/Instruments/Panning.h"
#include "Application/Utils/fixed.h"
#include <cstdint>

#include "System/Console/Trace.h"

#include "LSDJKitEnums.h"
#include "LSDJKits.generated.h"

#define SAMPLE_LEVEL 0x0FFF'FFFF
#define HALF_SAMPLE_LEVEL (SAMPLE_LEVEL >> 1)
#define NOISE_PHASE_LENGTH 0x4000'0000

/******************************************************************************
 * voice                                                                      *
 ******************************************************************************/

typedef struct lsdjkit_parameters_t {
  uint8_t kit1;
  uint8_t kit2;
  uint8_t bit_depth;
  uint8_t pan;
} lsdjkit_parameters_t;

static_assert(sizeof(lsdjkit_parameters_t) == 4, "Check sizeof(lsdjkit_parameters_t) in error message");

// (!) alignment has to be manually kept in this struct to allow using pack()
//     to keep the size as small as possible
#pragma pack(push, 1)
typedef struct lsdjkit_voice_t {
  lsdjkit_parameters_t parameters; // parameters passed from instrument

  uint32_t phase[2];       // oscillator phases
  uint32_t lastSample = 0; // used for both the last sample for pulse smoothing
                           // and as the lcg register for the noise

  uint32_t time; // sample counter
  uint16_t tick; // sample counter for 100Hz updates
  uint8_t tock;  // sample counter for 1000Hz updates

  uint8_t volume;
  uint8_t level;
  uint8_t bit_depth;
  uint8_t notes[2];

  uint8_t note;
  uint8_t drive;
  uint8_t buffer;

  lsdjkit_flags flags;

  uint32_t timeToLive;

  char sampleName[2][4] = {{0}, {0}};

  uint16_t wavetableLength[2];
  const int8_t *wavetable[2];

  panlaw_state pan;

  // implementation ------------------------------------------------------------

  inline void stop() {
  }

  inline void tick_100Hz() {
    // processing at ~100Hz
  }

  inline void tick_1000Hz() {
    if (timeToLive == 0) {
      if (flags.retrigger) {
        flags.retrigger = 0; // clear retrigger flag
        // retrigger without resetting clocks
        note_on(note, volume, false, parameters, true);
      } else {
        // note off, kill everything
        volume = 0;
      }
    } else {
      // length
      timeToLive--;
    }

    pan.tick();
  }

  inline void sample(fixed *left, fixed *right) {
    // precompute the gain, it doesn't need to be updated every sample

    // cold loop @ 100 Hz ------------------------------------------------------
    if (tick == 0) {
      tick = lsdjkitTicks100Hz;
      tick_100Hz();
    }

    // warm loop @ ~1000 Hz ----------------------------------------------------
    if (tock == 0) {
      tock = lsdjkitTicks1000Hz;
      tick_1000Hz(); // update at 1kHz for smoother pan and volume slides
    }

    // hot loop @ ~44100 Hz ----------------------------------------------------
    tick--;
    tock--;
    time++;

    // advance phase
    int32_t sample = 0;

    for (int kit = 0; kit < 2; kit++) {
      uint32_t index = phase[kit] >> 1;

      if (index < wavetableLength[kit]) {
        int8_t wave = wavetable[kit][index];
        uint8_t bits = static_cast<uint8_t>(wave);

        switch (bit_depth) {
          case 1:
            bits = (bits & 0x80) ? 0xff : 0x00;
            break;
          case 2:
            {
              uint8_t v = bits & 0xc0;
              bits = v | (v >> 2) | (v >> 4) | (v >> 6);
              break;
            }
          case 3:
            {
              uint8_t v = bits & 0xe0;
              bits = v | (v >> 3) | (v >> 6);
              break;
            }
          case 4:
            {
              uint8_t v = bits & 0xf0;
              bits = v | (v >> 4);
              break;
            }
          case 5:
            {
              uint8_t v = bits & 0xf8;
              bits = v | (v >> 5);
              break;
            }
          case 6:
            {
              uint8_t v = bits & 0xfc;
              bits = v | (v >> 6);
              break;
            }
          case 7:
            {
              uint8_t v = bits & 0xfe;
              bits = v | (v >> 7);
              break;
            }
        }

        wave = static_cast<int8_t>(bits);

        phase[kit] += 1;
        sample += wave << 21;
      }
    }

    // apply gain. volume already has the instrument volume folded in by
    // I_Instrument::EffectiveVolume and only changes at note on / step
    // volume, so there is nothing to precompute per tick here. Shifting
    // first keeps the result in the same range it had before the gain stage
    // existed (>> 8 then * 255 is a no-op within rounding).
    sample = (sample >> 8) * volume;

    // apply panning
    *left = fp_mul_coef(sample, (fixed)pan.left);
    *right = fp_mul_coef(sample, (fixed)pan.right);
  }

  inline void note_on(unsigned char note, uint8_t inVolume, bool retrigger, const lsdjkit_parameters_t inParameters,
                      bool keepClocks = false) {
    // bool retrigger is currently unused
    parameters = inParameters;

    // store volume
    volume = inVolume;

    // store settings
    this->note = note;
    notes[0] = note / 15;
    notes[1] = note % 15;

    bit_depth = inParameters.bit_depth;

    // pan jumps to the instrument's setting on note on; PAN commands slew
    // from there via pan.slew_to().
    pan.set(inParameters.pan);

    // setup the wavetables
    uint8_t kitIndex[2] = {parameters.kit1, parameters.kit2};

    for (int kit = 0; kit < 2; kit++) {
      if (notes[kit] != 0) {
        const lsdjKits::Kit &kitData = lsdjKits::kits[kitIndex[kit]];
        const lsdjKits::Sample &sample = kitData.samples[notes[kit] - 1];
        wavetable[kit] = (int8_t *)(kitData.data + sample.position);
        wavetableLength[kit] = sample.length;
        phase[kit] = 0;

        strcpy(sampleName[kit], sample.name);
      }
    }

    // reset oscillator state and timers
    timeToLive = -1;

    // don't reset timers on internal retrigger via command (IRT, ...)
    // they might be mid-execution and will underflow
    if (!keepClocks) {
      time = 0;
      tick = 0;
      tock = 0;
    }
  }

  /****************************************************************************
   *  command processing                                                     *
   ****************************************************************************/

  void set_instrument_parameter(uint8_t param, uint8_t value) {
    Trace::Error("Set parameter %d to %d", param, value);
    switch (param) {
      default:
        // invalid parameter index, ignore for now
        break;
    }
  }

  void set_step_volume(uint8_t inVolume) {
    volume = inVolume;
  }
} lsdjkit_voice_t;
#pragma pack(pop)

// 128 bytes per voice max to keep the entire thing under 1kB for the 8 voices,
// also struct needs to be aligned to 4 bytes to prevent unaligned access
static_assert(sizeof(lsdjkit_voice_t) <= 128, "Check sizeof(lsdjkit_voice_t) in error message");
static_assert((sizeof(lsdjkit_voice_t) % 4) == 0, "lsdjkit_voice_t size must be multiple of 4");
