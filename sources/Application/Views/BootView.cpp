/*
* SPDX-License-Identifier: BSD-3-Clause
*
* Copyright (c) 2026 nILS Podewski
*
* This file is part of the copingTracker firmware
*/

#include "BootView.h"

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

  if (mask & BM_ENTER) {
    Navigate(VT_SONG);
  } else {
    Reset();
  }
}

void BootView::DrawView() {
  Clear();

  for (int n = 0; n < animationSize_; n++) {
    DrawIndex(n);
  }
}

void BootView::OnFocus() {
  Reset();
}

void BootView::AnimationUpdate() {
  if (wrongCount_ == 0) {
    animationDone_ = true;
    return;
  }
  
  fixCount_ = wrongCount_ / 10;
  if (fixCount_ == 0) fixCount_ = 1;
  
  int fix = fixCount_;
  
  while (fix--) {
    uint8_t rand = Random();
    RevealRandom();
    RevealColor();
  }

  wrongCount_ -= fixCount_;
}

void BootView::RevealColor() {

  int pos = Random() % animationSize_;
  while (animationColors_[pos].byte == defaultColor_.byte) {
    pos++;
    if (pos >= animationSize_) {
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
  const color_t defaultColor = { fg: WHITE, bg: BLACK };
  
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

void BootView::RevealRing() {
  const int width = 32;
  const int height = animationSize_ / width;

  // roll dice for direction, right or left, top or bottom
  bool left = Random() & 13;
  bool top = Random() & 25;

  const int xStart = left ? 0 : width - 1;
  const int xEnd = left ? width : -1;
  const int xStep = left ? 1 : -1;

  const int yStart = top ? 0 : height - 1;
  const int yEnd = top ? height : -1;
  const int yStep = top ? 1 : -1;

  int selected = -1;

  for (int x = xStart; x != xEnd && selected < 0; x += xStep) {
    for (int y = yStart; y != yEnd; y += yStep) {
      const int index = y * width + x;

      if (animationContent_[index] != animationTarget_[index]) {
        selected = index;
        animationContent_[index] = animationTarget_[index];
        return;
      }
    }
  }

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
  lcg_ = lcg_ * 1664525	+ 1013904223;
  return lcg_;
}

void BootView::CoordinatesForIndex(int index, uint8_t *x, uint8_t *y) {
  *x = index & 0x1f;
  *y = 10 + (index >> 5);
}