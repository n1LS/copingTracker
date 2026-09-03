/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _BOOTVIEW_H
#define _BOOTVIEW_H

#include "Application/AppWindow.h"
#include "ScreenView.h"
#include "ViewData.h"

class BootView : public ScreenView {
public:
  BootView(GUIWindow &w, ViewData *data);
  void Reset();
  virtual void ProcessButtonMask(uint16_t mask, bool pressed);
  virtual void DrawView();
  virtual void OnFocus();
  virtual void AnimationUpdate();
  bool IsAnimationDone() const {
    return animationDone_;
  }

  void SetLoadTrigger();

private:
  uint32_t Random();
  void RandomizeColors();
  void RandomizeAnimation();

  void CoordinatesForIndex(int index, uint8_t *x, uint8_t *y);
  void DrawIndex(int index);

  void RevealColor();
  void RevealRandom();
  void RevealRing();

  constexpr static const color_t defaultColor_ = {{.fg = WHITE, .bg = BLACK}};
  static const int animationSize_ = SCREEN_WIDTH * 3;
  color_t animationColors_[animationSize_];
  char animationContent_[animationSize_];
  constexpr static const char *animationTarget_ = "           PROPS " char_logo_1 "            "
                                                  "           NORTH " char_logo_2 "            "
                                                  "           AUDIO " char_logo_3 "            ";
  uint32_t lcg_ = 13;
  uint32_t wrongCount_;
  uint32_t fixCount_ = 1;
  bool animationDone_ = false;
  bool waitingForLoad_ = false;
};

#endif
