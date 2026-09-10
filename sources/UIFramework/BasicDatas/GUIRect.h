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

#ifndef _GUIRECT_H_
#define _GUIRECT_H_

#include "GUIPoint.h"

// A Simple class to represent rectangles. Note that the Constructor
// is using top,left,bottom,right and NOT x,y coordinates

struct GUIRect {
  GUIRect(int x = 0, int y = 0, int w = 0, int h = 0);
  GUIRect(GUIPoint &, GUIPoint &);

  // Returns true if the specified point is contained in the rectangle.
  // Include completely the rectangle's border

  bool Contains(const GUIPoint &);

  // Moves the rectangle top-left position keeping its width/height

  void SetPosition(GUIPoint &);

  // Returns the top-left corner of the rectangle

  GUIPoint GetPosition();

  // Returns a rectangle resulting of the intersection of the two rects

  GUIRect Intersect(GUIRect);

  // Make sure the top/left/right/bottom are in correct order

  void Normalize();

  // Translate the rectangle from the given offset

  void Translate(GUIPoint &);

  // Accessor to the rectangle coordinates and size

  inline int Top() const {
    return y_;
  }

  inline int Left() const {
    return x_;
  }

  inline int Bottom() const {
    return y_ + h_;
  }

  inline int Right() const {
    return x_ + w_;
  }

  inline int Width() const {
    return w_;
  }

  inline int Height() const {
    return h_;
  }

  int x_;
  int y_;
  int w_;
  int h_;
};

#endif
