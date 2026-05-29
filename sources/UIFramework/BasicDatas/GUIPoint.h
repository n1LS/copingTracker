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

#ifndef _GUIPOINT_H_
#define _GUIPOINT_H_

// A Simple class to represent a Point/Position

class GUIPoint {
public:
  GUIPoint(long x = 0, long y = 0) {
    x_ = x;
    y_ = y;
  };
  void Add(GUIPoint p) {
    x_ = x_ + p.x_;
    y_ = y_ + p.y_;
  };
  void Sub(GUIPoint p) {
    x_ = x_ - p.x_;
    y_ = y_ - p.y_;
  };
  bool operator==(const GUIPoint &p) const {
    return x_ == p.x_ && y_ == p.y_;
  };
  bool operator!=(const GUIPoint &p) const {
    return x_ != p.x_ || y_ != p.y_;
  };

  GUIPoint operator+(const GUIPoint &p) const {
    return GUIPoint(x_ + p.x_, y_ + p.y_);
  };

  long x_, y_;
};
#endif
