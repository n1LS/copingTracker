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

#include "UISwatchField.h"
#include "Application/AppWindow.h"

UISwatchField::UISwatchField(const GUIPoint &position, const Color color) : UIField(position) {
  color_ = color;
}

void UISwatchField::Draw(GUIWindow &w, int offset) {
  GUIPoint position = GetPosition() + GUIPoint(1, offset);

  w.SetBackgroundColor(color_);
  w.DrawString(position.x_, position.y_, "   ");
}

void UISwatchField::ProcessArrow(uint16_t mask) {};

bool UISwatchField::IsStatic() {
  return true;
}

int UISwatchField::GetFocusOffset() {
  return 0;
}

int UISwatchField::GetFocusWidth() {
  return 0;
}
