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

#ifndef _GUICREATEWINDOWPARAMS_H_
#define _GUICREATEWINDOWPARAMS_H_

#include "GUIRect.h"

// Parameters to specify in order to create new windows

class GUICreateWindowParams {
public:
  const char *title;
};

#endif
