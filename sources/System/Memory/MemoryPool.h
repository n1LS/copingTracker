/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _MEMORY_POOL_H
#define _MEMORY_POOL_H

#include "Externals/SRC/common.h" // for SRC_MAX_RATIO
#include "Externals/etl/include/etl/vector.h"
#include "System/FileSystem/FileSystem.h"
#include <cstdint>

// A single piece of RAM shared by several buffers that are never needed at
// the same time. Only one "view" (file system writes, persistency document
// (XML) parsing, or sample-import scratch) is ever alive at once
//
// The file-index listing vector (FileList()) is intentionally NOT part of
// that union and keeps its own dedicated storage
class MemoryPool {
public:
  // File indexes produced while browsing/listing directories (file picker
  // views, persistency scans, etc). One listing operation at a time.
  static etl::ivector<int> &FileList() {
    return fileIndexes_;
  }

  // File system write/read buffer
  static uint8_t *fileBuffer() {
    return scratch_.fileBuffer;
  }

  // PersistencyDocument buffer
  static char *persistencyStack() {
    return scratch_.Persistency.stack;
  }
  static char *persistencyAttrName() {
    return scratch_.Persistency.attrname;
  }
  static char *persistencyAttrVal() {
    return scratch_.Persistency.attrval;
  }
  static char *persistencyAttrContent() {
    return scratch_.Persistency.content;
  }
  static const int persistencyAttrContentSize = 129;
  static const int persistencyAttrNameSize = 64;
  static const int persistencyAttrSize = 64;
  static const int persistencyStackSize = 1024;

  // Shared scratch buffers used by SamplePool while importing a WAV file
  // (with optional resampling) during sample import.
  static constexpr int32_t kImportChunkSize = 512;
  static constexpr int32_t kImportInputSamples = kImportChunkSize / static_cast<int32_t>(sizeof(int16_t));
  static constexpr int32_t kImportMaxOutputSamples = (kImportInputSamples * SRC_MAX_RATIO) + 8;

  static float *GetImportResampleIn() {
    return scratch_.import.resample.resampleIn;
  }
  static float *GetImportResampleOut() {
    return scratch_.import.resample.resampleOut;
  }
  static int16_t *GetImportResampleOutInt16() {
    return scratch_.import.resample.resampleOutInt16;
  }
  // Straight (non-resampled) copy buffer. Mutually exclusive in time with
  // the resample buffers above (a single import either resamples or does a
  // straight copy), so it is nested in its own union with them.
  static uint8_t *GetImportRawCopyBuffer() {
    return scratch_.import.rawCopyBuffer;
  }

private:
  union Scratch {
    // Backing storage for file system writes
    uint8_t fileBuffer[512];

    // Backing storage for the persistency document
    struct Persistency {
      char stack[persistencyStackSize];
      char attrname[persistencyAttrNameSize];
      char attrval[persistencyAttrSize];
      char content[persistencyAttrContentSize]; // 128 + \0
    } Persistency;

    // Backing storage for sample-import scratch buffers.
    union Import {
      struct Resample {
        float resampleIn[kImportInputSamples];
        float resampleOut[kImportMaxOutputSamples];
        int16_t resampleOutInt16[kImportMaxOutputSamples];
      } resample;
      uint8_t rawCopyBuffer[kImportChunkSize];
    } import;
  };

  static Scratch scratch_;
  // Dedicated storage for the file-index listing vector.
  static etl::vector<int, MAX_FILE_INDEX_SIZE> fileIndexes_;
};

#endif
