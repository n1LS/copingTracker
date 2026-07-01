/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef _PICOTRACKEREVENTMANAGER_
#define _PICOTRACKEREVENTMANAGER_

#include "Foundation/T_Singleton.h"
#include "SerialDebugUI.h"
#include "UIFramework/SimpleBaseClasses/EventManager.h"
#include <string>

#include "pico/stdlib.h"

class picoTrackerEventManager : public T_Singleton<picoTrackerEventManager>, public EventManager {
public:
  picoTrackerEventManager();
  ~picoTrackerEventManager();
  virtual bool Init();
  virtual int MainLoop();
  virtual void SetVirtualButtonMask(uint16_t buttonMask, bool pressed) override;

protected:
  static void ProcessInputEvent();

private:
  static repeating_timer_t timer_;

  static bool finished_;
  static bool redrawing_;
  static uint16_t buttonMask_;
  static uint16_t debounceMask_;
  static unsigned int lastDebounceTime_;
  static unsigned int keyRepeat_;
  static unsigned int keyDelay_;
  static unsigned int keyKill_;
  static bool isRepeating_;
  static unsigned long time_;

  static SerialDebugUI serialDebugUI_;

  static uint16_t virtualButtonMask_;
};
#endif
