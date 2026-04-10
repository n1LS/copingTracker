/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "picoTrackerMutex.h"

picoTrackerMutex::picoTrackerMutex() {
  mutex_init(&mutex_);
}

inline bool picoTrackerMutex::Lock() {
  mutex_enter_blocking(&mutex_);
  return true;
}

inline void picoTrackerMutex::Unlock() {
  mutex_exit(&mutex_);
}
