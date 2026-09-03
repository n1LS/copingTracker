/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include <stdint.h>
#include <string.h>

namespace picoTrackerProjectLoader {
bool StartLoad(const char *path);
bool IsLoadInProgress();
bool IsLoadComplete();
void AcknowledgeLoadComplete();
void GetProgress(uint32_t *index, uint32_t *total, char *messageBuf, size_t bufSize);
} // namespace picoTrackerProjectLoader
