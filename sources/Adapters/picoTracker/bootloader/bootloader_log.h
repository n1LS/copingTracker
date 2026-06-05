/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker Boot Manager
 */

#ifndef BOOTLOADER_LOG_H
#define BOOTLOADER_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

void bootlog(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif