/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "System/mirrorUI/mirrorUIProtocol.h"
#include <cstdint>

#ifndef PICO_REMOTE_UI_H_
#define PICO_REMOTE_UI_H_

#define ITF_NUM_CDC_0 0
#define USB_TIMEOUT_US 500000

void sendToUSBCDC(char buf[], int length);

#endif