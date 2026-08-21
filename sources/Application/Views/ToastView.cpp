/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2025 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "ToastView.h"
#include "Application/AppWindow.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include <string.h>

#define TOAST_MAX_LINE_WIDTH (SCREEN_WIDTH - 5)
#define TOAST_MAX_LINES 8

ToastView *ToastView::instance_ = nullptr;

ToastView::ToastView(GUIWindow &w, ViewData *viewData) : View(w, viewData) {
  // Initialize lines array
  for (int i = 0; i < TOAST_MAX_LINES; i++) {
    lines_[i][0] = 0;
  }
};

ToastView::~ToastView() {
}
ToastView *ToastView::GetInstance() {
  return instance_;
}

void ToastView::Init(GUIWindow &w, ViewData *viewData) {
  if (!instance_) {
    __attribute__((section(".DATA_RAM"))) static char toastStorage[sizeof(ToastView)];
    instance_ = new (toastStorage) ToastView(w, viewData);
  }
}

// splits the message into multiple lines fitting into the toast width and pads
// with spaces on both sides
void ToastView::WrapText(const char *message) {
  lineCount_ = 0;

  constexpr int prefixWidth = 3;
  constexpr int lineWidth = prefixWidth + TOAST_MAX_LINE_WIDTH;
  constexpr int lineSize = lineWidth + 1; // + '\0'

  auto addEmptyLine = [&]() {
    memset(lines_[lineCount_], ' ', lineWidth);
    lines_[lineCount_][lineWidth] = '\0';
    lineCount_++;
  };

  addEmptyLine();

  const int msgLen = strlen(message);
  int pos = 0;

  while (pos < msgLen && lineCount_ < maxLines - 1) {
    int len = msgLen - pos;

    if (len > TOAST_MAX_LINE_WIDTH)
      len = TOAST_MAX_LINE_WIDTH;

    // Prefer breaking at a space.
    if (len == TOAST_MAX_LINE_WIDTH) {
      for (int i = len - 1; i >= len / 2; i--) {
        if (message[pos + i] == ' ') {
          len = i;
          break;
        }
      }
    }

    // Start with the entire line padded with spaces.
    memset(lines_[lineCount_], ' ', lineWidth);

    // Copy the actual text after the 3-character prefix.
    memcpy(lines_[lineCount_] + prefixWidth, message + pos, len);

    lines_[lineCount_][lineWidth] = '\0';
    lineCount_++;

    pos += len;

    while (pos < msgLen && message[pos] == ' ') {
      pos++;
    }
  }

  addEmptyLine();
}

void ToastView::UpdateTimer() {
  if (!visible_)
    return;

  uint32_t now = System::GetInstance()->Millis();

  // check if we should start animating out
  if (now >= dismissTime_ && animationStartTime_ == 0) {
    animationStartTime_ = now;
    ((AppWindow &)w_).SetDirty();
  }

  // update animation offset
  if (animationStartTime_ > 0) {
    int newOffset = (now - animationStartTime_) / 50; // one row per 50ms

    if (newOffset >= 3 + lineCount_) {
      // animation complete, hide the toast
      visible_ = false;
      animationOffset_ = 0;
      animationStartTime_ = 0;
      ((AppWindow &)w_).SetDirty();
    } else if (newOffset != animationOffset_) {
      // animation in progress, update offset
      animationOffset_ = newOffset;
      ((AppWindow &)w_).SetDirty();
    }
  }
}

void ToastView::Show(const char *text, const ToastType *type, uint32_t msTime) {
  type_ = *type;
  visible_ = true;
  animationOffset_ = 0;
  animationStartTime_ = 0;
  dismissTime_ = System::GetInstance()->Millis() + msTime;
  WrapText(text);
}

void ToastView::Draw(GUIWindow &w) {
  if (!visible_)
    return;

  // SetColor(Theme::View::fg);
  SetColor(LIGHT_BLUE);
  SetBackgroundColor(DARK_GRAY);

  int y = std::max(0, (int)(SCREEN_HEIGHT - lineCount_ - 1 + animationOffset_));
  int iconY = y + 2;

  // top margin line
  SetColor(Theme::Dialog::bg);
  DrawChar(0, y, CHAR(char_filledHalfBorder_topLeft_s), true);
  DrawChar(SCREEN_WIDTH - 1, y, CHAR(char_filledHalfBorder_topRight_s), true);

  for (int x = 1; x < SCREEN_WIDTH - 1; x++) {
    DrawChar(x, y, CHAR(char_block_bottom_s), true);
  }

  // border left and right
  for (int i = 0; i < lineCount_ && y + i < SCREEN_HEIGHT; i++) {
    DrawChar(0, y + 1 + i, CHAR(char_block_left_s), true);
    DrawChar(SCREEN_WIDTH - 1, y + 1 + i, CHAR(char_block_right_s), true);
  }

  // messages
  SetColor(Theme::Dialog::fg);
  SetBackgroundColor(Theme::Dialog::bg);

  for (int i = 0; i < lineCount_ && y < SCREEN_HEIGHT; i++) {
    DrawString(1, y + 1 + i, lines_[i]);
  }

  // add the icon
  SetColor(type_.color);
  SetBackgroundColor(Theme::Dialog::bg);

  if (iconY < SCREEN_HEIGHT) {
    DrawString(2, iconY, type_.symbol);
  }
}