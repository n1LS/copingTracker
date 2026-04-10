/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef _CRITICALERRORMESSAGE_H_
#define _CRITICALERRORMESSAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void critical_error_message(const char *message, int guruId, bool (*externalCallback)(void));

#ifdef __cplusplus
}
#endif

#endif