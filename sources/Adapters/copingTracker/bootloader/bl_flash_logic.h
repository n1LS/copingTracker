/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#ifndef PATCHBAY_BL_FLASH_LOGIC_H
#define PATCHBAY_BL_FLASH_LOGIC_H

#include "Externals/SdFat/src/SdFat.h"

// Flash the selected .bin file to the app slot and persist metadata.
// On success, reboots the device. On failure, returns false.
bool bl_handle_flash_and_boot(SdFs *sd, const char *bin_path);

// Boot the installed firmware without flashing.
// Returns false if the slot is not bootable.
bool bl_handle_boot_installed(SdFs *sd, const char *installed_bin_path);

#endif // PATCHBAY_BL_FLASH_LOGIC_H
