/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "bl_config.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <algorithm>
#include <cstdint>

static bool is_in_slot_range(uint32_t absolute_address, uint32_t length) {
  if (length == 0) {
    return false;
  }

  const uint32_t write_end = absolute_address + length;
  if (write_end < absolute_address) {
    return false;
  }

  const uint32_t slot_start = kXipBase;
  const uint32_t slot_end = kXipBase + kAppSlotSize;

  if (absolute_address < slot_start || write_end > slot_end) {
    return false;
  }

  // Also guard against accidentally reaching the bootloader's own flash region.
  if (absolute_address >= kBootloaderBase) {
    return false;
  }

  return true;
}

int erase_firmware_range(uint32_t slot_address, uint32_t image_size) {
  if (image_size == 0 || image_size > kAppSlotSize) {
    return -1;
  }

  if (slot_address < kXipBase || slot_address >= (kXipBase + kAppSlotSize)) {
    return -1;
  }

  if (!is_in_slot_range(slot_address, image_size)) {
    return -1;
  }

  const uint32_t erase_start = slot_address & ~(FLASH_SECTOR_SIZE - 1u);
  const uint32_t erase_end = (slot_address + image_size + (FLASH_SECTOR_SIZE - 1u)) & ~(FLASH_SECTOR_SIZE - 1u);
  const uint32_t erase_size = erase_end - erase_start;

  const uint32_t flash_offset = erase_start - kXipBase;
  uint32_t irq_state = save_and_disable_interrupts();
  flash_range_erase(flash_offset, erase_size);
  restore_interrupts(irq_state);

  return 0;
}

int write_firmware_chunk(uint32_t absolute_address, const uint8_t *data, uint32_t length) {
  if (data == nullptr || length == 0) {
    return -1;
  }

  if ((length % FLASH_PAGE_SIZE) != 0) {
    return -1;
  }

  if ((absolute_address % FLASH_PAGE_SIZE) != 0) {
    return -1;
  }

  if (!is_in_slot_range(absolute_address, length)) {
    return -1;
  }

  const uint32_t flash_offset = absolute_address - kXipBase;
  uint32_t irq_state = save_and_disable_interrupts();
  flash_range_program(flash_offset, data, length);
  restore_interrupts(irq_state);
  return 0;
}

int verify_firmware_chunk(uint32_t absolute_address, const uint8_t *data, uint32_t length) {
  if (data == nullptr || length == 0) {
    return -1;
  }

  if (!is_in_slot_range(absolute_address, length)) {
    return -1;
  }

  const uint8_t *flash_ptr = reinterpret_cast<const uint8_t *>(absolute_address);
  for (uint32_t i = 0; i < length; ++i) {
    if (flash_ptr[i] != data[i]) {
      return -1;
    }
  }

  return 0;
}
