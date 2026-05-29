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

GUIRect::GUIRect(long x0, long y0, long x1, long y1) : _topLeft(x0, y0), _bottomRight(x1, y1) {
}

GUIRect::GUIRect(GUIPoint &topLeft, GUIPoint &bottomRight) : _topLeft(topLeft), _bottomRight(bottomRight) {
}

// Returns true if the point is contained inside the specified rectangle

bool GUIRect::Contains(GUIPoint &p) {
  return p.x_ >= _topLeft.x_ && p.x_ <= _bottomRight.x_ && p.y_ >= _topLeft.y_ && p.y_ <= _bottomRight.y_;
}

// Returns the topLeft corner of the rectangle

GUIPoint GUIRect::GetPosition() {
  return _topLeft;
}

// Moves the rectangle to the specified topLeft point. The rectangle keeps
// the same size

void GUIRect::SetPosition(GUIPoint &point) {
  long w = _bottomRight.x_ - _topLeft.x_;
  long h = _bottomRight.y_ - _topLeft.y_;
  _topLeft = point;
  _bottomRight = point;
  _bottomRight.Add(GUIPoint(w, h));
}

// Translate the rectangle

void GUIRect::Translate(GUIPoint &p) {
  _topLeft.Add(p);
  _bottomRight.Add(p);
}

GUIRect GUIRect::Intersect(GUIRect &other) {
  this->Normalize();
  other.Normalize();

  GUIPoint topLeft = _topLeft;
  if (other.Left() > topLeft.x_) {
    topLeft.x_ = other.Left();
  }
  if (other.Top() > topLeft.y_) {
    topLeft.y_ = other.Top();
  }

  GUIPoint bottomRight = _bottomRight;
  if (other.Right() < bottomRight.x_) {
    bottomRight.x_ = other.Right();
  }
  if (other.Bottom() < bottomRight.y_) {
    bottomRight.y_ = other.Bottom();
  }
  return GUIRect(topLeft, bottomRight);
}

void GUIRect::Normalize() {
  if (_topLeft.x_ > _bottomRight.x_) {
    int x = _topLeft.x_;
    _topLeft.x_ = _bottomRight.x_;
    _bottomRight.x_ = x;
  }
  if (_topLeft.y_ > _bottomRight.y_) {
    int y = _topLeft.y_;
    _topLeft.y_ = _bottomRight.y_;
    _bottomRight.y_ = y;
  }
}
