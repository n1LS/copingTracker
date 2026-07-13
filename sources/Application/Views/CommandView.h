/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "ModalView.h"

#ifndef COMMAND_VIEW_H
#define COMMAND_VIEW_H

class CommandView : public ModalView {

public:
  static CommandView *Create(View &view, FourCC command);
  virtual void Destroy() override;
  virtual ~CommandView();

  virtual void ProcessButtonMask(uint16_t mask, bool pressed);
  virtual void DrawView();
  virtual void OnFocus();

  virtual void AnimationUpdate() {};
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int currentTick);

  void SetCommand(FourCC command);
  FourCC GetCommmand();

  void Reset();

protected:
  CommandView(View &view, FourCC command);

private:
  void ProcessSelection(uint16_t mask);

  static bool inUse_;
  static void *storage_;

  int index_ = 0;
};

#endif