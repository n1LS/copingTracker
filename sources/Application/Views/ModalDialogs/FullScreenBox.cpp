/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "FullScreenBox.h"
#include <Application/AppWindow.h>
#include <new>

static bool inUse = false;
alignas(FullScreenBox) static unsigned char FullScreenBoxStorage[sizeof(FullScreenBox)];
static void *storage = FullScreenBoxStorage;

FullScreenBox *FullScreenBox::Create(View &view, const char *title, const char *message, int btnFlags) {
  if (inUse) {
    auto *existing = reinterpret_cast<FullScreenBox *>(storage);
    existing->~FullScreenBox();
    inUse = false;
  }
  inUse = true;
  return new (storage) FullScreenBox(view, title, message, btnFlags);
}

FullScreenBox *FullScreenBox::Create(View &view, const char *title, const char *message, const char *message2,
                                     int btnFlags) {
  if (inUse) {
    auto *existing = reinterpret_cast<FullScreenBox *>(storage);
    existing->~FullScreenBox();
    inUse = false;
  }
  inUse = true;
  return new (storage) FullScreenBox(view, title, message, message2, btnFlags);
}

FullScreenBox::FullScreenBox(View &view, const char *title, const char *message, int btnFlags)
    : MessageBox(view, title, message, btnFlags) {
}

FullScreenBox::FullScreenBox(View &view, const char *title, const char *message, const char *message2, int btnFlags)
    : MessageBox(view, title, message, message2, btnFlags) {
}

FullScreenBox::~FullScreenBox() {};

void FullScreenBox::Destroy() {
  this->~FullScreenBox();
  inUse = false;
}

void FullScreenBox::DrawView() {
  // message size
  int line1_width = line1_.size();
  // set window size full screen
  SetWindow(SCREEN_WIDTH, SCREEN_HEIGHT);

  // draw text
  SetColor(Theme::View::error);
  SetBackgroundColor(Theme::View::bg);

  int x1 = ((SCREEN_WIDTH - line1_width) / 2) - 2;
  int y1 = (SCREEN_HEIGHT / 2) - 4;

  DrawString(x1, y1, line1_.c_str());

  if (line2_.size() > 0) {
    int line2_width = line2_.size();
    int x2 = ((SCREEN_WIDTH - line2_width) / 2) - 2;
    int y2 = y1 + 2;
    DrawString(x2, y2, line2_.c_str());
  }
}
