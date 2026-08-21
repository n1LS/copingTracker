/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#ifndef PATCHBAY_BL_FLASH_WRITER_H
#define PATCHBAY_BL_FLASH_WRITER_H

#include <cstdint>

int write_firmware_chunk(uint32_t absolute_address, const uint8_t *data, uint32_t length);
int verify_firmware_chunk(uint32_t absolute_address, const uint8_t *data, uint32_t length);
int erase_firmware_range(uint32_t slot_address, uint32_t image_size);

#endif
