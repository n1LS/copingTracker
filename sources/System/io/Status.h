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

#ifndef _STATUS_H_
#define _STATUS_H_

#include "Foundation/T_Factory.h"

class Status : public T_Factory<Status> {
public:
  virtual void Print(char *) = 0;
  virtual void PrintMultiLine(char *) = 0;
  static void Set(const char *fmt, ...);
  static void SetMultiLine(const char *fmt, ...);
};

#endif
