/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _MEMORY_POOL_H
#define _MEMORY_POOL_H

#include "Externals/etl/include/etl/vector.h"
#include "System/FileSystem/FileSystem.h"

class MemoryPool {
 public:
    static etl::vector<int, MAX_FILE_INDEX_SIZE> &Get() {
        return fileIndexes_;
    }

 private:
    static etl::vector<int, MAX_FILE_INDEX_SIZE> fileIndexes_;
};

#endif