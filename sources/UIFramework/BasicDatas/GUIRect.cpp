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

#include "GUIRect.h"

// Constructor: specifies top,lef,bottom and right coordinates

GUIRect::GUIRect(int x, int y, int w, int h) : x_(x), y_(y), w_(w), h_(h) {
}
GUIRect::GUIRect(GUIPoint &topLeft, GUIPoint &bottomRight)
    : x_(topLeft.x_), y_(topLeft.y_), w_(bottomRight.x_ - topLeft.x_), h_(bottomRight.y_ - topLeft.y_) {
}

// Returns true if the point is contained inside the specified rectangle

bool GUIRect::Contains(const GUIPoint &p) {
  return p.x_ >= x_ && p.y_ >= y_ && p.y_ < y_ + h_ && p.x_ < x_ + w_;
}

// Returns the topLeft corner of the rectangle

GUIPoint GUIRect::GetPosition() {
  return GUIPoint(x_, y_);
}

// Moves the rectangle to the specified topLeft point. The rectangle keeps
// the same size

void GUIRect::SetPosition(GUIPoint &point) {
  x_ = point.x_;
  y_ = point.y_;
}

// Translate the rectangle

void GUIRect::Translate(GUIPoint &p) {
  x_ += p.x_;
  y_ += p.y_;
}
