/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "HostProjectLoader.h"
#include "Application/Instruments/SamplePool.h"
#include "System/FileSystem/FileSystem.h"
#include <cstring>

namespace picoTrackerProjectLoader {
static bool loadInProgress = false;
static bool loadComplete = false;

bool StartLoad(const char *path) {
  // Unlike the Pico (which offloads sample loading to core1 and reports
  // completion asynchronously via IsLoadComplete()), the Host has no
  // equivalent second-core constraint, so we can load synchronously here.
  // Without actually calling SamplePool::Load(), the sample pool's name
  // list stayed empty on Host, so persisted SampleInstrument sample_
  // (CHAR_LIST) selections could never resolve by name on restore and were
  // silently dropped (reset to NO_SAMPLE).
  loadInProgress = true;

  FileSystem::GetInstance()->chdir("/");
  SamplePool::GetInstance()->Load(path);

  loadComplete = true;
  return true;
}

bool IsLoadInProgress() {
  return loadInProgress;
}

bool IsLoadComplete() {
  return loadComplete;
}

void AcknowledgeLoadComplete() {
  loadInProgress = false;
  loadComplete = false;
}

void GetProgress(uint32_t *index, uint32_t *total, char *messageBuf, size_t bufSize) {
  if (index)
    *index = 0;
  if (total)
    *total = 0;
  if (messageBuf && bufSize > 0)
    messageBuf[0] = '\0';
}
} // namespace picoTrackerProjectLoader
