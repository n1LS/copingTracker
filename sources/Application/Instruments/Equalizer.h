/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _EQUALIZER_H_
#define _EQUALIZER_H_

#include "Application/Utils/fixed.h"
#include "config/AudioConstants.h"
#include <stdint.h>

// 3 band equalizer working on single 16 bit samples.
//
// The signal is split with two cascaded pairs of one pole lowpasses:
//   low  = lowpass(low crossover)
//   mid  = lowpass(high crossover) - low
//   high = input - lowpass(high crossover)
// so low + mid + high is exactly the input and the equalizer is transparent
// when all gains are equal. Each band gets a gain of 0..2x.
//
// The crossover frequencies are compile time settings.
constexpr float EQ_LOW_CROSSOVER_HZ = 400.0f;
constexpr float EQ_HIGH_CROSSOVER_HZ = 2500.0f;

// gain setting (0..0xFF) that results in unity gain
constexpr int EQ_UNITY_GAIN = 0x80;

// One pole lowpass coefficient in the fixed point format. x / (1 + x) is the
// usual cheap approximation of 1 - exp(-x), x = 2 * pi * f / fs
constexpr int32_t eq_coefficient(float hz) {
  return (int32_t)((hz * 6.2831853f / SAMPLE_RATE_F) / (1.0f + hz * 6.2831853f / SAMPLE_RATE_F) * FIXED_SCALE);
}

constexpr int32_t EQ_LOW_COEFFICIENT = eq_coefficient(EQ_LOW_CROSSOVER_HZ);
constexpr int32_t EQ_HIGH_COEFFICIENT = eq_coefficient(EQ_HIGH_CROSSOVER_HZ);

// Extra precision bits of the filter states, the crossover coefficients are
// small enough that a state without them would stall (a one pole lowpass stops
// moving once |input - state| * coefficient < 1 LSB).
//
// The bits are limited so that (input - state) * coefficient fits an int32.
// That way a filter step is a single 32 bit multiply: the Cortex-M0+ has no
// 32x32->64 multiply, and splitting the operand like fp_mul_coef does costs
// more than the whole rest of the sample. The state is a mix of int16 samples,
// so |input - state| < 2^16 * 2^shift.
constexpr int EQ_LOW_STATE_SHIFT = 4;
constexpr int EQ_HIGH_STATE_SHIFT = 1;

static_assert(((int64_t)1 << (16 + EQ_LOW_STATE_SHIFT)) * EQ_LOW_COEFFICIENT < ((int64_t)1 << 31),
              "low crossover filter step overflows int32");
static_assert(((int64_t)1 << (16 + EQ_HIGH_STATE_SHIFT)) * EQ_HIGH_COEFFICIENT < ((int64_t)1 << 31),
              "high crossover filter step overflows int32");

// gain is 14 bit fixed point (1.0 = 0x4000), setting * 0x80
constexpr int EQ_GAIN_SHIFT = 7;
constexpr int EQ_GAIN_FRACTION_BITS = 14;

typedef struct equalizer_t {
  // gain per band, 1.0 = 1 << EQ_GAIN_FRACTION_BITS
  int32_t gainLow;
  int32_t gainMid;
  int32_t gainHigh;

  // filter state for the two sides (left/right), two poles per crossover
  int32_t lowState[2][2];
  int32_t highState[2][2];

  // false until the first sample after a reset, lets the user of the
  // equalizer restart it cleanly when it gets switched on
  bool active;

  void reset() {
    for (int side = 0; side < 2; side++) {
      for (int pole = 0; pole < 2; pole++) {
        lowState[side][pole] = 0;
        highState[side][pole] = 0;
      }
    }
    active = false;
  }

  // gains are in the range 0..0xFF, 0x80 is unity, 0xFF about 2x
  void set_low(uint8_t gain) {
    gainLow = (int32_t)gain << EQ_GAIN_SHIFT;
  }
  void set_mid(uint8_t gain) {
    gainMid = (int32_t)gain << EQ_GAIN_SHIFT;
  }
  void set_high(uint8_t gain) {
    gainHigh = (int32_t)gain << EQ_GAIN_SHIFT;
  }

  // side is 0 for left and 1 for right
  //
  // Forced inline: left alone the compiler emits a call per sample, and the
  // call overhead on the M0+ is a good part of the whole cost of the filter.
  __attribute__((always_inline)) inline int16_t process(int16_t in, int side) {
    int32_t *lo = lowState[side];
    int32_t xLo = (int32_t)in << EQ_LOW_STATE_SHIFT;
    lo[0] += ((xLo - lo[0]) * EQ_LOW_COEFFICIENT) >> FIXED_SHIFT;
    lo[1] += ((lo[0] - lo[1]) * EQ_LOW_COEFFICIENT) >> FIXED_SHIFT;

    int32_t *hi = highState[side];
    int32_t xHi = (int32_t)in << EQ_HIGH_STATE_SHIFT;
    hi[0] += ((xHi - hi[0]) * EQ_HIGH_COEFFICIENT) >> FIXED_SHIFT;
    hi[1] += ((hi[0] - hi[1]) * EQ_HIGH_COEFFICIENT) >> FIXED_SHIFT;

    // bring both lowpass outputs back to plain sample scale, the bands then
    // sum up to exactly the input
    int32_t lowpassLow = lo[1] >> EQ_LOW_STATE_SHIFT;
    int32_t lowpassHigh = hi[1] >> EQ_HIGH_STATE_SHIFT;
    int32_t low = lowpassLow;
    int32_t mid = lowpassHigh - lowpassLow;
    int32_t high = in - lowpassHigh;

    // shifted per band, the sum of the three products can exceed 32 bit
    int32_t out = ((low * gainLow) >> EQ_GAIN_FRACTION_BITS) + ((mid * gainMid) >> EQ_GAIN_FRACTION_BITS) +
                  ((high * gainHigh) >> EQ_GAIN_FRACTION_BITS);

    if (out > INT16_MAX) {
      out = INT16_MAX;
    } else if (out < INT16_MIN) {
      out = INT16_MIN;
    }
    return (int16_t)out;
  }

  // Processes an interleaved stereo buffer of fixed point samples in place,
  // size is the number of sample pairs.
  void process_buffer(fixed *buffer, int size) {
    for (int i = 0; i < size; i++) {
      for (int side = 0; side < 2; side++) {
        int v = fp2i(buffer[2 * i + side]);
        v = (v > INT16_MAX) ? INT16_MAX : ((v < INT16_MIN) ? INT16_MIN : v);
        buffer[2 * i + side] = i2fp(process((int16_t)v, side));
      }
    }
  }
} equalizer_t;

#endif
