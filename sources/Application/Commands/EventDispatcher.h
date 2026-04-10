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

#ifndef _EVENT_DISPATCHER_H_
#define _EVENT_DISPATCHER_H_

#include "Application/AppWindow.h"
#include "ApplicationCommandDispatcher.h"
#include "Foundation/Observable.h"
#include "Foundation/T_Singleton.h"
#include "System/Timer/Timer.h"

class EventDispatcher : public T_Singleton<EventDispatcher>, public CommandExecuter, public I_Observer {
public:
  EventDispatcher();
  ~EventDispatcher();
  void SetWindow(GUIWindow *window);
  virtual void Execute(FourCC id, float value);
  unsigned int OnTimerTick();
  int GetEventMask() {
    return eventMask_;
  };
  virtual void Update(Observable &o, I_ObservableData *d);

private:
  GUIWindow *window_;
  static int keyRepeat_;
  static int keyDelay_;
  unsigned int eventMask_;
  unsigned int repeatMask_;
  I_Timer *timer_;
};

#endif
