/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#include "slot_boot.h"
#include "Adapters/copingTracker/bootloader/bootloader_log.h"
#include "hardware/regs/m0plus.h"
#include "hardware/structs/nvic.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/systick.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include <cstdint>
#include <cstdio>

// This function jumps to the application entry point
// It must update the vector table and stack pointer before jumping
void _Noreturn launch_application_from(void *app_location) {
  // https://vanhunteradams.com/Pico/Bootloader/Bootloader.html
  uint32_t *new_vector_table = (uint32_t *)app_location;
  volatile uint32_t *vtor = (uint32_t *)(PPB_BASE + M0PLUS_VTOR_OFFSET);
  *vtor = (uint32_t)new_vector_table;
  asm volatile("msr msp, %0\n"
               "bx %1\n"
               :
               : "r"(new_vector_table[0]), "r"(new_vector_table[1])
               :);

  while (true) {
    tight_loop_contents();
  }
}

bool boot_firmware_slot(uint32_t slot_base_address) {
  launch_application_from((void *)(XIP_BASE + 0x100));
  return true;
}
