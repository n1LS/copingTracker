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
 *  EventManager.cpp
 *  lgpt
 *
 *  Created by Marc Nostromo on 23/03/12.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

#include "EventManager.h"
#include "Application/Commands/NodeList.h"
#include "Application/Model/Config.h"
#include <cstring>

EventManager *EventManager::instance_ = NULL;

bool EventManager::Init() {
  instance_ = this;
  return true;
}
