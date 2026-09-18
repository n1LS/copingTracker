/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "MemoryPool.h"

etl::vector<int, MAX_FILE_INDEX_SIZE> MemoryPool::fileIndexes_;