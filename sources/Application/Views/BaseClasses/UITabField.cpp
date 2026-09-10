/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "UITabField.h"
#include "Application/AppWindow.h"

UITabField::UITabField(const char *label, const GUIPoint &position, Variable &variable, const char *tabs[], int count)
    : UIIntVarField(position, variable, "", 0, count - 1, 1, count - 1), src_(variable) {
  count_ = count;
  label_ = label;

  src_ = variable;

  for (int n = 0; n < count; n++) {
    tabs_[n] = tabs[n];
  }
}

void UITabField::Draw(GUIWindow &window, int offset) {
  AppWindow &w = ((AppWindow &)window);
  GUIPoint position = GetPosition();

  int index = src_.GetInt();

  w.SetColor(Theme::View::fg);
  w.SetBackgroundColor(backgroundColor_);
  w.DrawString(position.x_, position.y_, label_);

  int x = position.x_ + (int)strlen(label_);

  const char *space = " ";
  const char *noSpace = "";
  const char **nextSpace = &space;

  for (int t = 0; t < count_; t++) {
    int len = (int)strlen(tabs_[t]);

    if (index == t) {
      w.SetBackgroundColor(backgroundColor_);
      w.SetColor(Theme::View::Tab::bg(true));
      w.DrawChar(x, position.y_, CHAR(char_button_border_left_s));
      w.DrawChar(x + 1 + len, position.y_, CHAR(char_button_border_right_s));

      w.SetBackgroundColor(Theme::View::Tab::bg(true));
      w.SetColor(Theme::View::Tab::fg(true));
      w.DrawString(x + 1, position.y_, tabs_[t]);

      nextSpace = &noSpace;
      focusWidth_ = len + 2;
      focusPosition_ = x;
    } else {
      w.SetBackgroundColor(backgroundColor_);
      w.SetColor(Theme::View::Tab::fg(false));
      w.DrawString(x, position.y_, *nextSpace);
      w.DrawString(x + 1, position.y_, tabs_[t]);
      nextSpace = &space;
    }

    x += 1 + len;
  }

  // last space
  w.SetColor(RED);
  w.DrawString(x, position.y_, *nextSpace);
}

int UITabField::GetFocusOffset() {
  return focusPosition_;
}

void Update(Observable &o, I_ObservableData *d) {
}
