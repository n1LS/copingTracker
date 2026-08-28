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

#include "Status.h"
#include "Application/AppWindow.h"
#include <System/Console/nanoprintf.h>
#include <System/System/System.h>
#include <string.h>

void Status::Set(const char *fmt, ...) {
  auto status = Status::GetInstance();
  if (!status)
    return;

  char buffer[128];

  va_list args;
  va_start(args, fmt);
  npf_vsnprintf(buffer, sizeof(buffer), fmt, args);
  status->Print(buffer);
  va_end(args);

  extern AppWindow *instance;
  if (instance) {
    instance->Flush();
  }
}