/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#ifndef PATCHBAY_BL_SLOT_BOOT_H
#define PATCHBAY_BL_SLOT_BOOT_H

#include <cstdint>

void boot_firmware_slot(uint32_t slot_base_address);

#endif
