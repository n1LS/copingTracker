/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "HostMutex.h"

HostMutex::HostMutex() {
}

HostMutex::~HostMutex() {
}

bool HostMutex::Lock() {
  mutex_.lock();
  return true;
}

void HostMutex::Unlock() {
  mutex_.unlock();
}
