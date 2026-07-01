/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef _PICOTRACKERSYSTEM_H_
#define _PICOTRACKERSYSTEM_H_

#include <map>

#include "System/System/System.h"
#include "UIFramework/SimpleBaseClasses/EventManager.h"

#define USEREVENT_TIMER 0
#define USEREVENT_EXPOSE 1

class picoTrackerSystem : public System {
public:
  static void Boot(int argc, char **argv);
  static void Shutdown();
  static int MainLoop();

public: // System implementation
  virtual unsigned long GetClock();
  virtual void GetBatteryState(BatteryState &state);
  virtual void SetDisplayBrightness(unsigned char value);
  virtual void Sleep(int millisec);
  virtual unsigned int GetMemoryUsage();
  virtual void PowerDown() {};
  virtual void SystemBootloader();
  virtual void SystemReboot();
  virtual void SystemMassStorage();
  virtual void SystemPutChar(int c);
  virtual uint32_t GetRandomNumber();
  virtual uint32_t Micros();
  virtual uint32_t Millis();
  virtual SysMutex *GetMutex() override;

private:
  static bool invert_;
  static unsigned int lastBeatCount_;

  static EventManager *eventManager_;
};
#endif
