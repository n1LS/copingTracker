/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _PICOTRACKER_PROJECT_LOADER_H_
#define _PICOTRACKER_PROJECT_LOADER_H_

#include "Application/Persistency/PersistenceConstants.h"
#include <cstddef>
#include <cstdint>

enum class ProjectLoadResult { PENDING, DONE };

class picoTrackerProjectLoader {
public:
  // Called from core0 only. Returns false (and does nothing) if a load is
  // already in progress (re-entrancy guard).
  static bool StartLoad(const char *projectName);

  // Called from core0 only, once per frame while a load is in flight.
  static bool IsLoadInProgress();

  // Called from core0 only. Returns true exactly once the load has
  // finished; core0 must then call AcknowledgeLoadComplete() and resume
  // AudioThread via picoTrackerAudioDriver::ResumeAudioThread().
  static bool IsLoadComplete();

  // Snapshot of progress for display; safe to call from core0 at any time.
  // Copies out under the guard mutex — cheap, called every frame.
  static void GetProgress(uint32_t *index, uint32_t *total, char *messageBuf, size_t bufSize);

  // Core1 calls this to update progress (wrapped by SamplePool::updateStatus override).
  // Safe to call from core1; guarded by mutex.
  static void UpdateProgress(uint32_t index, uint32_t total, const char *message);

  // Core0 calls this once after observing IsLoadComplete() == true, before
  // starting a new load. Clears the in-progress/complete flags.
  static void AcknowledgeLoadComplete();

private:
  static void LoadThreadEntry(); // runs on core1
};

#endif
