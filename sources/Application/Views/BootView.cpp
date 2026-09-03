/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "BootView.h"
#ifdef __PICO__
#include "Adapters/copingTracker/system/picoTrackerProjectLoader.h"
#else
#include "Adapters/Host/system/HostProjectLoader.h"
#endif
#include "Application/Model/Project.h"
#include "System/io/Status.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

BootView::BootView(GUIWindow &w, ViewData *viewData) : ScreenView(w, viewData) {
  Reset();
}

void BootView::Reset() {
  lcg_ = 13;

  RandomizeAnimation();
  RandomizeColors();

  animationDone_ = false;
  wrongCount_ = animationSize_;
}

void BootView::ProcessButtonMask(uint16_t mask, bool pressed) {
  if (!pressed) {
    return;
  }

  // Don't respond to button input while a project load is in progress
  if (picoTrackerProjectLoader::IsLoadInProgress()) {
    return;
  }

  Navigate(VT_SONG, vtNone);
}

void BootView::DrawView() {
  Clear();

  for (int n = 0; n < animationSize_; n++) {
    DrawIndex(n);
  }

  // Preserve the build string at the bottom of the screen
  SetColor(Theme::View::inactive);
  SetBackgroundColor(Theme::View::bg);
  DrawString((SCREEN_WIDTH - strlen(VERSION_STRING)) / 2, 22, VERSION_STRING);
}

void BootView::OnFocus() {
  Reset();
}

void BootView::AnimationUpdate() {
  if (waitingForLoad_) {
    waitingForLoad_ = false;
    AppWindow::GetInstance()->DelayedProjectLoad();
  }

  // Run the reveal animation and show loading progress on top
  if (wrongCount_ == 0) {
    animationDone_ = true;
  } else {
    fixCount_ = wrongCount_ / 10;
    if (fixCount_ == 0)
      fixCount_ = 1;

    int fix = fixCount_;

    while (fix--) {
      uint8_t rand = Random();
      RevealRandom();
      RevealColor();
    }

    wrongCount_ -= fixCount_;
  }

  // Show loading progress if a load is in progress, overlaid on the animation
  if (picoTrackerProjectLoader::IsLoadInProgress()) {
    uint32_t idx, total;
    char msg[64];
    picoTrackerProjectLoader::GetProgress(&idx, &total, msg, sizeof(msg));
    // Display progress at the bottom of the screen, overwriting Status
    Status::Set(msg);
  } else {
    // no load in progress, animation done?
    if (animationDone_) {
      // jump to song view
      Navigate(VT_SONG, vtNone);
    }
  }
}

void BootView::RevealColor() {
  int pos = Random() % animationSize_;
  while (animationColors_[pos].byte == defaultColor_.byte) {
    if (++pos >= animationSize_) {
      pos = 0;
    };
  }

  animationColors_[pos] = defaultColor_;
  DrawIndex(pos);
}

void BootView::DrawIndex(int index) {
  uint8_t x, y;

  CoordinatesForIndex(index, &x, &y);
  SetColor(animationColors_[index].fg);
  SetBackgroundColor(animationColors_[index].bg);
  DrawChar(x, y, animationContent_[index]);
}

void BootView::RevealRandom() {
  const color_t defaultColor = {.fg = WHITE, .bg = BLACK};

  int pos = Random() % animationSize_;
  while (animationContent_[pos] == animationTarget_[pos]) {
    pos++;
    if (pos >= animationSize_) {
      pos = 0;
    };
  }

  animationContent_[pos] = animationTarget_[pos];
  DrawIndex(pos);
}

void BootView::RandomizeColors() {
  for (int n = 0; n < animationSize_; n++) {
    do {
      animationColors_[n].byte = Random() & 0xff;
    } while (defaultColor_.byte == animationColors_[n].byte);
  }
}

void BootView::RandomizeAnimation() {
  for (int n = 0; n < animationSize_; n++) {
    animationContent_[n] = Random() & 0xff;
  }
}

uint32_t BootView::Random() {
  lcg_ = lcg_ * 1664525 + 1013904223;
  return lcg_;
}

void BootView::CoordinatesForIndex(int index, uint8_t *x, uint8_t *y) {
  *x = index & 0x1f;
  *y = 10 + (index >> 5);
}

void BootView::SetLoadTrigger() {
  waitingForLoad_ = true;
}
