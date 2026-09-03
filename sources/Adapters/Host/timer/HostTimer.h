/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef HOST_TIMER_H_
#define HOST_TIMER_H_

#include "System/Timer/Timer.h"
#include <SDL2/SDL.h>

class HostTimer : public I_Timer {
public:
  HostTimer();
  virtual ~HostTimer();

  virtual void SetPeriod(float msec) override;
  virtual bool Start() override;
  virtual void Stop() override;
  virtual float GetPeriod() override;

  static Uint32 OnTimerTick(Uint32 interval, void *param);

private:
  float period_;
  SDL_TimerID timer_id_;
  bool running_;
};

class HostTimerService : public TimerService {
public:
  virtual I_Timer *CreateTimer() override;
  virtual void TriggerCallback(int msec, timerCallback cb) override;

private:
  static HostTimer timerInstance_;
};

#endif
