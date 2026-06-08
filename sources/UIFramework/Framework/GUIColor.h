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

#ifndef _GUICOLOR_H_
#define _GUICOLOR_H_
#include <stdint.h>

// A simple RGB Color representation class

class GUIColor {
public:
  GUIColor(uint16_t r, uint16_t g, uint16_t b) {
    r_ = r;
    g_ = g;
    b_ = b;
  }
  GUIColor(uint16_t r, uint16_t g, uint16_t b, int idx) {
    r_ = r;
    g_ = g;
    b_ = b;
    paletteIndex_ = idx;
  }
  uint16_t r_, g_, b_;
  int paletteIndex_;

  // Equality operator
  bool operator==(const GUIColor &other) const {
    return r_ == other.r_ && g_ == other.g_ && b_ == other.b_;
  }
};
#endif
