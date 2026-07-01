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

/*
 *  EventManager.h
 *  lgpt
 *
 *  Created by Marc Nostromo on 23/03/12.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "Externals/etl/include/etl/string.h"
#include "config/StringLimits.h"

#define PICO_CLOCK_INTERVAL 33 // ~30Hz
#define PICO_CLOCK_HZ (1000 / PICO_CLOCK_INTERVAL)

class EventManager {
public:
  EventManager() {};

  static EventManager *instance_;

  virtual ~EventManager() {};
  virtual bool Init();
  virtual int MainLoop() = 0;
  virtual void SetVirtualButtonMask(uint16_t buttonMask, bool pressed) = 0;
};
