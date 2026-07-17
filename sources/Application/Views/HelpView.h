/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _HELP_VIEW_H_
#define _HELP_VIEW_H_

#include "ScreenView.h"
#include "ViewData.h"

class HelpView : public ScreenView {
public:
  HelpView(GUIWindow &w, ViewData *viewData);
  ~HelpView();
  virtual void ProcessButtonMask(uint16_t mask, bool pressed) override;
  virtual void DrawView() override;
  virtual void OnFocus();

private:
  void setTab(int index);
  void scrollBy(int delta);
  void drawTabs();
  void drawContent();
  int selectedTab_;
  int offset_;
  int tabOffset_;
  const uint8_t *data_;
  int numLines_; 
  unsigned int dataSize_; 
  bool navDown_;
};
#endif
