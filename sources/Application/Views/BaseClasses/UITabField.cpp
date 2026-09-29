/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "UITabField.h"
#include "Application/AppWindow.h"

namespace {
// The screen buffer indexes characters with an unsigned column, so drawing
// outside [0, SCREEN_WIDTH) would wrap around and corrupt memory. These
// helpers additionally clip to [loX, hiX) so the tab strip never bleeds into
// the label area (to the left) or past the reserved indicator column (to the
// right) while it is scrolled.
void DrawCharClipped(AppWindow &w, int x, int y, char c, int loX, int hiX) {
  if (x >= loX && x < hiX && x >= 0 && x < SCREEN_WIDTH) {
    w.DrawChar(x, y, c);
  }
}

void DrawStringClipped(AppWindow &w, int x, int y, const char *str, int loX, int hiX) {
  for (const char *c = str; *c; c++, x++) {
    DrawCharClipped(w, x, y, *c, loX, hiX);
  }
}
} // namespace

UITabField::UITabField(const char *label, const GUIPoint &position, Variable &variable, const char *tabs[], int count)
    : UIIntVarField(position, variable, "", 0, count - 1, 1, count - 1), src_(variable) {
  count_ = count;
  tabOffset_ = 0;
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

  int tabAreaStart = position.x_ + (int)strlen(label_);

  // Total width consumed if every tab were drawn (each tab advances by
  // len+1, with one extra trailing char after the last tab).
  int contentWidth = 1;
  for (int t = 0; t < count_; t++) {
    contentWidth += 1 + (int)strlen(tabs_[t]);
  }

  int availableWidth = SCREEN_WIDTH - tabAreaStart;
  bool needsScroll = contentWidth > availableWidth;

  // Reserve the last screen column for the scroll indicator when scrolling
  // is needed.
  int maxRight = needsScroll ? SCREEN_WIDTH - 1 : SCREEN_WIDTH;

  if (needsScroll) {
    // Keep the focused tab within the visible window, shifting tabOffset_
    // just enough to bring it back into view.
    int tabX = tabOffset_;
    for (int t = 0; t < index; t++) {
      tabX += 1 + (int)strlen(tabs_[t]);
    }
    int tabWidth = (int)strlen(tabs_[index]) + 2;

    int rightEdge = tabAreaStart + tabX + tabWidth;
    if (rightEdge > maxRight) {
      tabOffset_ -= (rightEdge - maxRight);
    }

    int leftEdge = tabAreaStart + tabX;
    if (leftEdge < tabAreaStart) {
      tabOffset_ += (tabAreaStart - leftEdge);
    }
  } else {
    tabOffset_ = 0;
  }

  int x = tabAreaStart + tabOffset_;

  const char *space = " ";
  const char *noSpace = "";
  const char **nextSpace = &space;

  for (int t = 0; t < count_; t++) {
    int len = (int)strlen(tabs_[t]);
    int tabStart = x;
    int tabEnd = x + 1 + len;
    // True only for the single leftmost visible tab when it is scrolled such
    // that its start is hidden behind the label but part of it still shows.
    bool cutOnLeft = needsScroll && tabStart < tabAreaStart && tabEnd > tabAreaStart;

    if (index == t) {
      Color bg = focus_ ? Theme::PanelInput::bg(true) : Theme::View::Tab::bg(true);
      Color fg = focus_ ? Theme::PanelInput::fg(true) : Theme::View::Tab::fg(true);

      w.SetBackgroundColor(backgroundColor_);
      w.SetColor(bg);

      DrawCharClipped(w, x, position.y_, CHAR(char_button_border_left_s), tabAreaStart, maxRight);
      DrawCharClipped(w, x + 1 + len, position.y_, CHAR(char_button_border_right_s), tabAreaStart, maxRight);

      w.SetBackgroundColor(bg);
      w.SetColor(fg);
      DrawStringClipped(w, x + 1, position.y_, tabs_[t], tabAreaStart, maxRight);

      if (cutOnLeft) {
        DrawCharClipped(w, tabAreaStart, position.y_, CHAR(char_indicator_ellipsis_s), tabAreaStart, maxRight);
      }

      nextSpace = &noSpace;
      focusWidth_ = len + 2;
      focusPosition_ = x;
    } else {
      w.SetBackgroundColor(backgroundColor_);
      w.SetColor(Theme::View::Tab::fg(false));
      DrawStringClipped(w, x, position.y_, *nextSpace, tabAreaStart, maxRight);
      DrawStringClipped(w, x + 1, position.y_, tabs_[t], tabAreaStart, maxRight);

      if (cutOnLeft) {
        DrawCharClipped(w, tabAreaStart, position.y_, CHAR(char_indicator_ellipsis_s), tabAreaStart, maxRight);
      }

      nextSpace = &space;
    }

    x += 1 + len;
  }

  // Fill anything left between the last drawn tab and the reserved indicator
  // column (or the screen edge, when not scrolling) so no stale background
  // pixels remain from a previous, wider draw. If the last tab is selected
  // and fully visible, its right border character already occupies the
  // first column of this range, so don't overwrite it.
  bool lastTabRightBorderDrawn = (index == count_ - 1) && x > tabAreaStart && x < maxRight;
  int fillStart = lastTabRightBorderDrawn ? x + 1 : x;

  w.SetColor(Theme::View::fg);
  w.SetBackgroundColor(backgroundColor_);
  for (int fillX = fillStart; fillX < maxRight; fillX++) {
    DrawCharClipped(w, fillX, position.y_, ' ', tabAreaStart, maxRight);
  }

  if (needsScroll) {
    // x now points just past the last drawn tab: if it still reaches beyond
    // the screen, there is more content hidden to the right.
    char indicator;
    if (x >= SCREEN_WIDTH) {
      indicator = (tabOffset_ < 0) ? CHAR(char_indicator_leftRight_s) : CHAR(char_indicator_rightNoLeft_s);
    } else {
      indicator = CHAR(char_indicator_leftNoRight_s);
    }

    w.SetColor(Theme::View::fg);
    w.SetBackgroundColor(backgroundColor_);
    w.DrawChar(SCREEN_WIDTH - 1, position.y_, indicator);
  }
}

int UITabField::GetFocusOffset() {
  return focusPosition_;
}

void Update(Observable &o, I_ObservableData *d) {
}
