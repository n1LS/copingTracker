/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _CORE0_LOAD_H_
#define _CORE0_LOAD_H_

#include <cstdint>

// How busy core 0 (the UI core) is, on the device (CORE0_LOAD_METER, set in sources/CMakeLists.txt, not for the host).
//
// The main loop of core 0 never sleeps, it polls USB, MIDI, the buttons and the event queue all the time, so
// the load is the share of the time the loop spends handling something: an event from the queue (redraws,
// clock ticks) or a button change. Update() is called once at the top of every pass of the main loop and
// attributes the time since the last pass to busy or idle, whether the last pass called MarkWork(). Every
// WINDOW_US the share is published as the current average. Core 0 writes and reads it, there is nothing to
// synchronise. The core 1 (audio) load is MixerService::GetRenderLoad().
//
// Without CORE0_LOAD_METER (the host build) everything here compiles to nothing.
class Core0Load {
public:
#ifdef CORE0_LOAD_METER
  // the pass of the main loop that is running handled something
  static void MarkWork() {
    worked_ = true;
  }

  // called at the top of every pass of the main loop
  static void Update(uint32_t nowUs) {
    if (!started_) {
      started_ = true;
      lastUs_ = nowUs;
      return;
    }

    // unsigned, so it survives the micros counter wrapping
    const uint32_t passUs = nowUs - lastUs_;
    lastUs_ = nowUs;

    windowUs_ += passUs;
    if (worked_) {
      busyUs_ += passUs;
    }
    worked_ = false;

    if (windowUs_ >= WINDOW_US) {
      const uint64_t percent = (static_cast<uint64_t>(busyUs_) * 100) / windowUs_;
      percent_ = percent > 99 ? 99 : static_cast<uint32_t>(percent);
      windowUs_ = 0;
      busyUs_ = 0;
    }
  }

  // the share of time core 0 was busy in the last window, 0..99
  static uint32_t Percent() {
    return percent_;
  }

private:
  static constexpr uint32_t WINDOW_US = 250000;

  static inline bool started_ = false;
  static inline bool worked_ = false;
  static inline uint32_t lastUs_ = 0;
  static inline uint32_t windowUs_ = 0;
  static inline uint32_t busyUs_ = 0;
  static inline uint32_t percent_ = 0;
#else
  static void MarkWork() {
  }
  static void Update(uint32_t) {
  }
  static uint32_t Percent() {
    return 0;
  }
#endif
};

#endif
