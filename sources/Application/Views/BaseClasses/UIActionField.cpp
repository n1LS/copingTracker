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

#include "UIActionField.h"
#include "Application/AppWindow.h"
#include "ViewUtils.h"
#include <string.h>

UIActionField::UIActionField(const char *name, unsigned int fourcc, GUIPoint &position) : UIField(position) {
  name_ = name;
  fourcc_ = fourcc;
}

UIActionField::~UIActionField() {};

void UIActionField::Draw(GUIWindow &w, int offset) {
  GUIPoint position(x_, y_ + offset);

  // enforce max field length
  char buffer[MAX_FIELD_WIDTH + 1];
  snprintf(buffer, sizeof(buffer), "(%.*s)", MAX_FIELD_WIDTH, name_);
  strncpy(buffer, name_, MAX_FIELD_WIDTH);
  buffer[MAX_FIELD_WIDTH] = '\0';

  ((AppWindow &)w).SetBackgroundColor(focus_ ? cHighlight1 : cBackground);
  ((AppWindow &)w).SetColor(focus_ ? cBackground : cNormal);
  w.DrawString(buffer, position);

  // add button ends
  if (focus_) {
    // draw highlight button ends
    GUIPoint pos(x_ - 1, y_);
    ((AppWindow &)w).SwapColors();
    w.DrawChar(GLYPH(char_button_border_left_s), pos);
    pos.x_ += strlen(buffer) + 1;
    w.DrawChar(GLYPH(char_button_border_right_s), pos);
  }
}

void UIActionField::OnClick() {
  SetChanged();
  NotifyObservers((I_ObservableData *)fourcc_);
}

const char *UIActionField::GetString() {
  return name_;
}
