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

#include "UIStaticField.h"
#include "Application/AppWindow.h"
#include "ViewUtils.h"
#include <string.h>

UIStaticField::UIStaticField(const GUIPoint &position, const char *string)
    : UIField(position), color_(Theme::View::fg) {
  string_ = string;
}

void UIStaticField::Draw(GUIWindow &w, int offset) {
  GUIPoint position = GetPosition();
  position.y_ += offset;

  ((AppWindow &)w).SetColor(color_);
  ((AppWindow &)w).SetBackgroundColor(Theme::View::bg);
  w.DrawString(position.x_, position.y_, string_);
}

void UIStaticField::ProcessArrow(uint16_t mask) {
}

bool UIStaticField::IsStatic() {
  return true;
}
