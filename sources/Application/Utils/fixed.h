/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

// fixed point arithmatic

#ifndef _FIXED_H
#define _FIXED_H

#define USE_FIXED_POINT

#ifdef USE_FIXED_POINT

#define FIXED_SHIFT 15
#define FIXED_SCALE (1 << FIXED_SHIFT)

#define i2fp(a) ((a) << FIXED_SHIFT)
#define fp2i(a) ((a) >> FIXED_SHIFT)

#define fp_add(a, b) ((a) + (b))
#define fp_sub(a, b) ((a) - (b))

#define fp_mul(x, y) ((fixed)(((long long)(x) * (long long)(y)) >> FIXED_SHIFT))

#define fp_div(x, y) ((((x) << 2) / ((y) >> 8)) << 10)
#define fp_mulint(x, y) fp_mul(x, y)

#define FP_ONE (1 << FIXED_SHIFT)
#define FPONE FP_ONE

typedef signed int fixed;

// fp_mul_coef(x, coef): equivalent to fp_mul(x, coef) but restricted to
// cases where 'coef' is a bounded fixed-point coefficient with
// |coef| <= FP_ONE (e.g. interpolation weights, volume/pan/filter
// coefficients). Cortex-M0+ (ARMv6-M, used by the RP2040) has no hardware
// 32x32->64 multiply, so fp_mul's 64-bit product compiles to a
// multi-instruction software routine; splitting 'x' into 16-bit halves lets
// the whole computation use single-cycle 32-bit MULS instead. Bit-exact
// with fp_mul(x, coef) for all int32 x and |coef| <= FP_ONE.
inline fixed fp_mul_coef(fixed x, fixed coef) {
  fixed xHi = x >> 16;
  fixed xLo = x & 0xFFFF;
  return (xHi * coef << 1) + ((xLo * coef) >> FIXED_SHIFT);
}

inline fixed fl2fp(float val) {
  return (fixed)(val * FIXED_SCALE);
}

inline float fp2fl(fixed val) {
  return ((float)val) / FIXED_SCALE;
}

#else // uses float

typedef float fixed;
#define fp2i(a) (int)a
#define i2fp(a) (fixed) a
#define fl2fp(a) a
#define fp2fl(a) a
#define fp_add(a, b) a + b
#define fp_sub(a, b) a - b
#define fp_mul(a, b) a *b
#define fp_div(a, b) a / b
#define fp_mul_coef(a, b) ((a) * (b))

#define FP_ONE 1.0
#define FPONE FP_ONE

#endif

#endif /*_FIXED_H*/
