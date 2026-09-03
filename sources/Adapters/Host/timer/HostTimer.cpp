/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "HostTimer.h"

HostTimer::HostTimer() : period_(0.0f), timer_id_(0), running_(false) {
}

HostTimer::~HostTimer() {
  if (running_) {
    Stop();
  }
}

void HostTimer::SetPeriod(float msec) {
  period_ = msec;
}

bool HostTimer::Start() {
  if (running_ || period_ <= 0.0f) {
    return false;
  }
  timer_id_ = SDL_AddTimer((Uint32)period_, OnTimerTick, this);
  if (timer_id_ == 0) {
    return false;
  }
  running_ = true;
  return true;
}

void HostTimer::Stop() {
  if (running_ && timer_id_ != 0) {
    SDL_RemoveTimer(timer_id_);
    timer_id_ = 0;
    running_ = false;
  }
}

float HostTimer::GetPeriod() {
  return period_;
}

Uint32 HostTimer::OnTimerTick(Uint32 interval, void *param) {
  HostTimer *self = (HostTimer *)param;
  self->SetChanged();
  self->NotifyObservers();
  return interval;
}

HostTimer HostTimerService::timerInstance_;

I_Timer *HostTimerService::CreateTimer() {
  timerInstance_.Stop();
  timerInstance_.SetPeriod(-1.0f);
  return &timerInstance_;
}

void HostTimerService::TriggerCallback(int msec, timerCallback cb) {
  SDL_AddTimer(
      msec,
      [](Uint32 interval, void *param) -> Uint32 {
        timerCallback callback = (timerCallback)param;
        callback();
        return 0;
      },
      (void *)cb);
}
