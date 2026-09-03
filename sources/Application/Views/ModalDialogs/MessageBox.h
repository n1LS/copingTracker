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

enum MessageBoxList { MBL_OK = 0, MBL_YES, MBL_CANCEL, MBL_NO, MBL_STOP, MBL_LAST };

enum MessageBoxButtonFlag { MBBF_OK = 0x01, MBBF_YES = 0x02, MBBF_CANCEL = 0x04, MBBF_NO = 0x08, MBBF_STOP = 0x10 };

class MessageBox : public ModalView {
public:
  static MessageBox *Create(View &view, const char *title, const char *message, int btnFlags = MBBF_OK);
  static MessageBox *Create(View &view, const char *title, const char *message, const char *message2,
                            int btnFlags = MBBF_OK);
  virtual ~MessageBox();
  virtual void Destroy() override;

  virtual void DrawView();
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int currentTick);
  virtual void OnFocus();
  virtual void ProcessButtonMask(uint16_t mask, bool pressed);
  virtual void AnimationUpdate() {};

  GUIRect GetFocusRect() override;

protected:
  MessageBox(View &view, const char *title, const char *message, int btnFlags = MBBF_OK);
  MessageBox(View &view, const char *title, const char *message, const char *message2, int btnFlags = MBBF_OK);
  etl::string<SCREEN_WIDTH - 2> title_ = "";
  etl::string<SCREEN_WIDTH - 2> line1_ = "";
  etl::string<SCREEN_WIDTH - 2> line2_ = "";
  int button_[4];
  int buttonPosition_[4];
  int buttonLength_[4];
  int buttonY_;
  int buttonCount_;
  int selected_;

private:
  static bool inUse_;
  static void *storage_;
};
#endif