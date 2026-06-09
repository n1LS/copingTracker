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

#ifndef _UPDATE_DATA_H_
#define _UPDATE_DATA_H_

// Increment/decrement a byte field by offset, clamping or wrapping within
// [0, limit]. 0xFF is treated as uninitialized and reset to 0 unless limit
// itself is 0xFF (i.e. the field uses the full uint8 range).
inline void updateDataValue(unsigned char *c, int offset, unsigned char limit, bool wrap) {
  int v = *c;
  if ((v == 0xFF) && (limit != 0xFF))
    v = 0;
  v += offset;
  if (v < 0)
    v = wrap ? (limit + 1 + v) : 0;
  if (v > limit)
    v = wrap ? v - (limit + 1) : limit;
  *c = (unsigned char)v;
}

#endif
