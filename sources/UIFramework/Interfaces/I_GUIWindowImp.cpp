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

#include "I_GUIWindowImp.h"

// Sets the GUI Window associted to the current imp

void I_GUIWindowImp::SetWindow(GUIWindow &window) {
  _window = &window;
}
