/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "HostProjectLoader.h"
#include <cstring>

namespace picoTrackerProjectLoader {
static bool loadInProgress = false;
static bool loadComplete = false;

bool StartLoad(const char *path) {
  loadInProgress = true;
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
