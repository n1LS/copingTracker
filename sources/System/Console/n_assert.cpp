/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "System/Console/Trace.h"
#include <assert.h>
#include <string.h>

void __NAssert(const char *exp, const char *file, unsigned line) {
  const char *filestr = file;
  if (strlen(file) > 20) {
    filestr = file + strlen(file) - 20;
  }
  Trace::Error("Assertion failed: %s", exp);
  Trace::Error("  >> file [%s]", filestr);
  Trace::Error("  >> line %d", line);
  assert(0);
}
