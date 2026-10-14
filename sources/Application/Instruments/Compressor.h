/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _COMPRESSOR_H_
#define _COMPRESSOR_H_

#include "Application/Utils/fixed.h"
#include <stdint.h>

// Simple feed forward, stereo linked compressor with automatic makeup gain.
//
// The cheap way on a core without divide, float or 64 bit multiply:
//   * the detector takes the peak of a block of COMP_BLOCK_SIZE sample pairs
//     (~2.7kHz control rate) and turns it into a log2 value with a small table
//   * threshold, ratio and gain are plain additions and one multiply in the log
//     domain, the result is turned back into a linear gain with a second table
//   * the attack is fixed: the gain drops to the target within a block. The
//     release slews the gain back up at a settable rate
//   * the linear gain is ramped across the block, so there is no zipper noise
//   * per sample it is then one multiply per channel
//
// Gain reduction is applied above the threshold with slope = 1 - 1/ratio. The
// makeup gain is derived from threshold and ratio so a full scale input still
// leaves at full scale, i.e. the quiet parts are brought up.

// number of sample pairs per detector block, has to be a power of 2
constexpr int COMP_BLOCK_SIZE = 16;
constexpr int COMP_BLOCK_SHIFT = 4;

// log2 values are in 8.8 fixed point, 256 = one octave = 6dB
constexpr int COMP_FULL_SCALE_LOG = 15 * 256; // log2(32768)
constexpr int COMP_THRESHOLD_RANGE = 8 * 256; // threshold setting 0 is 48dB below full scale
constexpr int COMP_MAX_MAKEUP = 2 * 256;      // the automatic makeup is limited to 12dB

// parameter defaults: the threshold at full scale and the ratio at 1:1 make a
// freshly enabled compressor transparent
constexpr int COMP_DEFAULT_THRESHOLD = 0xFF;
constexpr int COMP_DEFAULT_RATIO = 0x00;
constexpr int COMP_DEFAULT_RELEASE = 0x80;

// log2(1 + i/64) * 256
inline constexpr uint8_t comp_log_table[64] = {
    0,   6,   11,  17,  22,  28,  33,  38,  44,  49,  54,  59,  63,  68,  73,  78,  82,  87,  92,  96,  100, 105,
    109, 113, 118, 122, 126, 130, 134, 138, 142, 146, 150, 154, 157, 161, 165, 169, 172, 176, 179, 183, 186, 190,
    193, 197, 200, 203, 207, 210, 213, 216, 220, 223, 226, 229, 232, 235, 238, 241, 244, 247, 250, 253};

// 2^(i/64) * 4096
inline constexpr uint16_t comp_exp_table[64] = {
    4096, 4141, 4186, 4231, 4277, 4324, 4371, 4419, 4467, 4515, 4565, 4614, 4664, 4715, 4767, 4819,
    4871, 4924, 4978, 5032, 5087, 5142, 5198, 5255, 5312, 5370, 5428, 5487, 5547, 5607, 5668, 5730,
    5793, 5856, 5919, 5984, 6049, 6115, 6182, 6249, 6317, 6386, 6455, 6526, 6597, 6668, 6741, 6814,
    6889, 6964, 7039, 7116, 7194, 7272, 7351, 7431, 7512, 7594, 7677, 7760, 7845, 7930, 8016, 8104};

// How far the gain may rise per block, in 16.16 log2 units. Settings 0x00..0xFF
// run from 6dB per 20ms to 6dB per second (logarithmically spaced, 16 steps).
inline constexpr uint16_t comp_release_table[16] = {1189, 916, 706, 544, 419, 323, 249, 192,
                                                    148,  114, 88,  67,  52,  40,  31,  24};

// The input is limited to +-COMP_INPUT_LIMIT (the master mix of 8 channels can
// exceed 16 bit), and the gain is at most 2^(COMP_MAX_MAKEUP / 256). The ramp
// starts a block at the previous gain, so a loud block can meet the full
// makeup. The product has to fit 32 bits.
constexpr int COMP_INPUT_LIMIT = 65535;
static_assert((int64_t)COMP_INPUT_LIMIT * (4096LL << (COMP_MAX_MAKEUP >> 8)) < (1LL << 31),
              "compressor gain overflows int32");

typedef struct compressor_t {
  int32_t thresholdLog; // 8.8 log2
  int32_t slope;        // 1 - 1/ratio, 0..255
  int32_t makeup;       // 8.8 log2
  int32_t releaseSlew;  // 16.16 log2 per block

  int32_t gainLog; // current gain, 16.16 log2
  int32_t gainLin; // linear gain (4.12) the last block ended with

  // false until the first buffer after a reset, lets the user of the
  // compressor restart it cleanly when it gets switched on
  bool active;

  void reset() {
    gainLog = 0;
    gainLin = 1 << 12;
    active = false;
  }

  // all values are 0..0xFF.
  //   threshold: 0 is -48dB, 0xFF is 0dB
  //   ratio    : 0 is 1:1, 0x80 is 2:1, 0xC0 is 4:1, 0xFF is nearly a limiter
  //   release  : 0 is the fastest, 0xFF the slowest
  void set(uint8_t threshold, uint8_t ratio, uint8_t release) {
    thresholdLog = COMP_FULL_SCALE_LOG - (255 - threshold) * (COMP_THRESHOLD_RANGE >> 8);
    slope = ratio;

    // the gain reduction a full scale input would get, given back as makeup
    int32_t makeupLog = ((COMP_FULL_SCALE_LOG - thresholdLog) * slope) >> 8;
    makeup = (makeupLog > COMP_MAX_MAKEUP) ? COMP_MAX_MAKEUP : makeupLog;

    releaseSlew = comp_release_table[release >> 4];
  }

  // log2(x) in 8.8, x > 0. The Cortex-M0+ has no clz instruction, the
  // compiler's helper is only called once per block
  static int32_t log2_q8(uint32_t x) {
    int clz = __builtin_clz(x);
    uint32_t normalized = x << clz; // top bit is set
    return (31 - clz) * 256 + comp_log_table[(normalized >> 25) & 63];
  }

  // 2^(g / 65536) in 4.12, g <= COMP_MAX_MAKEUP << 8
  static int32_t antilog_q12(int32_t g) {
    int32_t octave = g >> 16;
    int32_t base = comp_exp_table[(g >> 10) & 63];
    if (octave >= 0) {
      return base << octave;
    }
    return (octave < -15) ? 0 : (base >> -octave);
  }

  // Processes an interleaved stereo buffer of fixed point samples in place,
  // size is the number of sample pairs. The output is limited to 16 bit.
  void process_buffer(fixed *buffer, int size) {
    while (size > 0) {
      int n = (size < COMP_BLOCK_SIZE) ? size : COMP_BLOCK_SIZE;

      // detector: peak of both channels
      int32_t peak = 0;
      for (int i = 0; i < 2 * n; i++) {
        int32_t v = fp2i(buffer[i]);
        v = (v < 0) ? -v : v;
        peak = (v > peak) ? v : peak;
      }
      peak = (peak > COMP_INPUT_LIMIT) ? COMP_INPUT_LIMIT : peak;
      int32_t level = (peak > 0) ? log2_q8(peak) : 0;

      int32_t over = level - thresholdLog;
      int32_t reduction = (over > 0) ? (over * slope) >> 8 : 0;
      int32_t target = (makeup - reduction) * 256;

      if (target < gainLog) {
        gainLog = target; // attack
      } else {
        gainLog += releaseSlew;
        gainLog = (gainLog > target) ? target : gainLog;
      }

      // linear ramp from where the last block ended to the new gain, held in
      // 20.12 + 4 bits so the step does not vanish
      int32_t newLin = antilog_q12(gainLog);
      int32_t lin = gainLin << 4;
      int32_t step = (n == COMP_BLOCK_SIZE) ? (newLin - gainLin) : (((newLin - gainLin) * 16) / n);
      gainLin = newLin;

      for (int i = 0; i < 2 * n; i += 2) {
        lin += step;
        int32_t gain = lin >> 4;
        for (int side = 0; side < 2; side++) {
          int32_t v = fp2i(buffer[i + side]);
          v = (v > COMP_INPUT_LIMIT) ? COMP_INPUT_LIMIT : ((v < -COMP_INPUT_LIMIT) ? -COMP_INPUT_LIMIT : v);
          int32_t out = (v * gain) >> 12;
          out = (out > INT16_MAX) ? INT16_MAX : ((out < INT16_MIN) ? INT16_MIN : out);
          buffer[i + side] = i2fp(out);
        }
      }

      buffer += 2 * n;
      size -= n;
    }
  }
} compressor_t;

#endif
