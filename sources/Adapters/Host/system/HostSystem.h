/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef HOST_SYSTEM_H_
#define HOST_SYSTEM_H_

#include "System/System/System.h"
#include "UIFramework/SimpleBaseClasses/EventManager.h"

class HostSystem : public System {
public:
  static void Boot(int argc, char **argv);
  static void Shutdown();
  static int MainLoop();

public:
  virtual unsigned long GetClock() override;
  virtual void GetBatteryState(BatteryState &state) override;
  virtual void SetDisplayBrightness(unsigned char value) override;
  virtual unsigned int GetMemoryUsage() override;
  virtual void SystemBootloader() override;
  virtual void SystemReboot() override;
  virtual void SystemMassStorage() override;
  virtual void SystemPutChar(int c) override;
  virtual uint32_t GetRandomNumber() override;
  virtual uint32_t Micros() override;
  virtual uint32_t Millis() override;
  virtual void Sleep(uint32_t millis) override;
  virtual SysMutex *GetMutex() override;

private:
  static EventManager *eventManager_;
};

#endif
