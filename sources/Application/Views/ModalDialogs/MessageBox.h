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

#ifndef _MESSAGE_BOX_H_
#define _MESSAGE_BOX_H_

#include "Application/Views/BaseClasses/ModalView.h"
#include <string>

#ifdef MessageBox
#undef MessageBox
#endif
#include <Application/AppWindow.h>

// The buttons are shown in this order, from left to right
enum MessageBoxList { MBL_OK = 0, MBL_YES, MBL_CANCEL, MBL_NO, MBL_STOP, MBL_AUTOSAVE, MBL_PROJECT, MBL_LAST };

enum MessageBoxButtonFlag {
  MBBF_OK = 0x01,
  MBBF_YES = 0x02,
  MBBF_CANCEL = 0x04,
  MBBF_NO = 0x08,
  MBBF_STOP = 0x10,
  MBBF_AUTOSAVE = 0x20,
  MBBF_PROJECT = 0x40
};

class MessageBox : public ModalView {
public:
  // defaultFlag: the button (one of the flags) that is selected when the box opens, without it the last button is
  static MessageBox *Create(View &view, const char *title, const char *message, int btnFlags = MBBF_OK,
                            int defaultFlag = 0);
  static MessageBox *Create(View &view, const char *title, const char *message, const char *message2,
                            int btnFlags = MBBF_OK, int defaultFlag = 0);
  virtual ~MessageBox();
  virtual void Destroy() override;

  virtual void DrawView();
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int currentTick);
  virtual void OnFocus();
  virtual void ButtonDown(uint16_t newButton, uint16_t mask);
  virtual void ButtonUp(uint16_t button, uint16_t mask) override;
  virtual void AnimationUpdate() {};

protected:
  MessageBox(View &view, const char *title, const char *message, int btnFlags = MBBF_OK, int defaultFlag = 0);
  MessageBox(View &view, const char *title, const char *message, const char *message2, int btnFlags = MBBF_OK,
             int defaultFlag = 0);
  etl::string<SCREEN_WIDTH - 2> title_ = "";
  etl::string<SCREEN_WIDTH - 2> line1_ = "";
  etl::string<SCREEN_WIDTH - 2> line2_ = "";
  int button_[MBL_LAST];
  int buttonPosition_[MBL_LAST];
  int buttonLength_[MBL_LAST];
  int buttonY_;
  int buttonCount_;
  int selected_;
  // ENTER was pressed while this dialog was open (see ButtonUp)
  bool enterDown_ = false;

private:
  static bool inUse_;
  static void *storage_;
};
#endif