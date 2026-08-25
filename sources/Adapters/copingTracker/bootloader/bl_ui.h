/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#ifndef PATCHBAY_BL_UI_H
#define PATCHBAY_BL_UI_H

#include "bl_menu.h"
#include <cstdint>

// Runs the main interactive menu loop.  This function never returns; it either
// reboots the device or boots into the application firmware.
void bl_run_ui_loop() __attribute__((noreturn));

#endif // PATCHBAY_BL_UI_H
