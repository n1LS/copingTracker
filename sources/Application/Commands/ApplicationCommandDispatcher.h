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

#ifndef _APPLICATION_COMMAND_DISPATCHER_H_
#define _APPLICATION_COMMAND_DISPATCHER_H_

#include "Application/Model/Project.h"
#include "Foundation/T_Singleton.h"

class CommandExecuter {
public:
  CommandExecuter() {};
  virtual ~CommandExecuter() {};
  virtual void Execute(Token id, float value) = 0;
};

class ApplicationCommandDispatcher : public T_Singleton<ApplicationCommandDispatcher>, public CommandExecuter {
public:
  ApplicationCommandDispatcher();
  ~ApplicationCommandDispatcher();
  void Init(Project *project);
  void Close();
  virtual void Execute(Token id, float value);
  void OnTempoTap();
  void OnQueueRow();
  void OnNudgeDown();
  void OnNudgeUp();

private:
  Project *project_;
};

#endif
