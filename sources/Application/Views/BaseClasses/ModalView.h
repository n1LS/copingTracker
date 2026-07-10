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

#ifndef _MODAL_VIEW_H_
#define _MODAL_VIEW_H_

#include "View.h"
#include <stdint.h>

class ModalView : public View {
public:
  ModalView(View &);
  virtual ~ModalView();

  bool IsFinished();
  int GetReturnCode();
  uint32_t GetInstanceId() const;

  void EndModal(int returnCode);
  virtual void Destroy();

protected:
  void SetWindow(int width, int height);
  virtual void ClearTextRect(int x, int y, int w, int h);
  virtual void DrawString(int x, int y, const char *text);
  virtual void DrawChar(int x, int y, char c, bool transparent = false);

  // Override GetAnchor to account for modal window position
  virtual GUIPoint GetAnchor();

  int left_;
  int top_;

private:
  static uint32_t nextInstanceId_;
  uint32_t instanceId_;
  bool finished_;
  int returnCode_;
};
#endif
