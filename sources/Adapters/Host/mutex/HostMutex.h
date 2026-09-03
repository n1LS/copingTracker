/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef HOST_MUTEX_H_
#define HOST_MUTEX_H_

#include "System/Process/SysMutex.h"
#include <mutex>

class HostMutex : public SysMutex {
public:
  HostMutex();
  virtual ~HostMutex();

  virtual bool Lock() override;
  virtual void Unlock() override;

private:
  std::mutex mutex_;
};

#endif
