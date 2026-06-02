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

  GUIPoint position = GetPosition();
  position.y_ += offset;

  position.x_ += 1;

  ((AppWindow &)w).SetBackgroundColor(color_);
  w.DrawString("   ", position);
  ((AppWindow &)w).SetColor(cccccNormal);
}

void UISwatchField::ProcessArrow(unsigned short mask) {};

bool UISwatchField::IsStatic() {
  return true;
}
