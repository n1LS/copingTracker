/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "MemoryPool.h"

MemoryPool::Scratch MemoryPool::scratch_;
etl::vector_ext<int> MemoryPool::fileIndexes_(scratch_.fileIndexStorage, MAX_FILE_INDEX_SIZE);
