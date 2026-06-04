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

#include "UIField.h"

UIField::UIField(const GUIPoint &position) {
  x_ = position.x_;
  y_ = position.y_;
  focus_ = false;
}

UIField::~UIField() {};

GUIPoint UIField::GetPosition() {
  GUIPoint point(x_, y_);
  return point;
}

void UIField::SetPosition(const GUIPoint &p) {
  x_ = p.x_;
  y_ = p.y_;
}

void UIField::ClearFocus() {
  focus_ = false;
}

void UIField::SetFocus() {
  focus_ = true;
}

bool UIField::HasFocus() {
  return focus_;
}

bool UIField::IsStatic() {
  return false;
}
