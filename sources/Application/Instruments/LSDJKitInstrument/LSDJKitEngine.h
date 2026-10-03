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
  uint8_t volume;
  uint8_t speed;
  uint8_t offset[2];
  uint8_t length[2];
  
  lsdjkit_loop_mode_e loop_mode[2];
  uint8_t clip_mode;
} lsdjkit_parameters_t;

static_assert(sizeof(lsdjkit_parameters_t) % 4 == 0, "Check sizeof(lsdjkit_parameters_t) in error message");

// (!) alignment has to be manually kept in this struct to allow using pack()
//     to keep the size as small as possible
#pragma pack(push, 1)
typedef struct lsdjkit_voice_t {
  lsdjkit_parameters_t parameters; // parameters passed from instrument

  uint32_t phase[2];       // wavetable index/oscillator phases in q24.8
  uint32_t lastSample = 0; // used for both the last sample for pulse smoothing
  // and as the lcg register for the noise
  
  uint32_t time; // sample counter
  uint16_t tick; // sample counter for 100Hz updates
  uint8_t tock;  // sample counter for 1000Hz updates
  
  uint32_t offset[2]; // start offset per sample

  uint8_t volume;
  uint8_t level;
  uint8_t bit_depth;
  uint8_t notes[2];
  
  uint8_t note;
  uint8_t drive;
  uint8_t buffer;

  uint8_t loop_mode[2];

  uint16_t speed;
  lsdjkit_flags flags;

  uint32_t timeToLive[2];

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
    for (int kit = 0; kit < 2; kit++) {
      // TODO nILS: check how to handle that with 2 individual voices...
      if (timeToLive[kit] == 0) {
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
        timeToLive[kit]--;
      }
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
      uint32_t index = phase[kit] >> 9; // convert from q24.8 to integer index and also downsample to 22050

      // handle loop modes
      bool safe = (index < wavetableLength[kit]);

      if (!safe) {
        if (loop_mode[kit] == loopModeOn) {
          safe = true;
          index = parameters.offset[kit];
          phase[kit] = index << 9; // convert back to q24.8
        } else if (loop_mode[kit] == loopModeAttack) {
          safe = true;
          index = 0;
          phase[kit] = 0;
        }
      }

      if (safe) {
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

        phase[kit] += speed;
        sample += wave;
      }
    }

    // sample is 2x 8bit --> 9bits, we need to get it up to 32
    // handle clip modes
    constexpr int32_t maxSample = 127; // this clips well below the 8bit range, to get a good compromise between loudness and distortion
    constexpr int32_t minSample = -128;

    if (sample > maxSample || sample < minSample) {
      const bool positive = sample > maxSample;

      switch (parameters.clip_mode) {
        case clipModeHard:
          sample = positive ? maxSample : minSample;
          break;

        case clipModeSoft:
          sample = positive
            ? maxSample + ((sample - maxSample) >> 1)
            : minSample + ((sample - minSample) >> 1);
          break;

        case clipModeFold:
          sample = positive
            ? maxSample - (sample - maxSample)
            : minSample - (sample - minSample);
          break;

        case clipModeWrap:
          sample = positive
            ? minSample + (sample + minSample)
            : maxSample + (sample + maxSample);
          break;
      }
    }
    sample <<= 23;

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

    // bit crushing
    bit_depth = inParameters.bit_depth;

    // loop mode
    loop_mode[0] = inParameters.loop_mode[0];
    loop_mode[1] = inParameters.loop_mode[1];

    // pan jumps to the instrument's setting on note on; PAN commands slew
    // from there via pan.slew_to().
    pan.set(inParameters.pan);

    // setup the wavetables
    uint8_t kitIndex[2] = {parameters.kit1, parameters.kit2};

    for (int kit = 0; kit < 2; kit++) {
      // reset oscillator state and timers
      timeToLive[kit] = (parameters.length[kit] == 0) ? 0x7FFF'FFFF : (parameters.length[kit]);

      if (notes[kit] != 0) {
        const LSDJKits::Kit &kitData = LSDJKits::kits[kitIndex[kit]];
        const LSDJKits::Sample &sample = kitData.samples[notes[kit] - 1];
        wavetable[kit] = (int8_t *)(kitData.data + sample.position);
        wavetableLength[kit] = sample.length;
        offset[kit] = (sample.length * inParameters.offset[kit]) >> 8; // offset is in 0-255, scale to sample length
        phase[kit] = offset[kit] << 9; // convert to q24.8

        strcpy(sampleName[kit], sample.name);
      }
    }


    // speed                           .25 .5   1.0  2.0 
    const int rates[speedModeCount] = {64, 128, 256, 512};
    speed = rates[inParameters.speed];

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
