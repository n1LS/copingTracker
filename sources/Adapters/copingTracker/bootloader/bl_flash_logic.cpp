/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#include "bl_flash_logic.h"
#include "Adapters/copingTracker/platform/platform.h"
#include "Foundation/Types/Colors.h"
#include "bl_config.h"
#include "bl_log.h"
#include "bl_menu.h"
#include "bl_path_utils.h"
#include "bl_sd_ops.h"
#include "bl_slot_boot.h"
#include "bl_uf2_parser.h"
#include "pico/time.h"
#include <cstring>

bool bl_handle_flash_and_boot(SdFs *sd, const char *bin_path) {
  if (bin_path == nullptr) {
    menu_show_message("No firmware selected.", nullptr, LIGHT_YELLOW);
    return false;
  }

  const char *base = bl_path_basename(bin_path);

  // Derive the original source UF2 name for metadata persistence.
  char source_uf2[80];
  bl_copy_str(source_uf2, sizeof(source_uf2), "/");
  bl_append_str(source_uf2, sizeof(source_uf2), base);
  (void)bl_replace_extension_ci(source_uf2, sizeof(source_uf2), ".bin", ".uf2");

  // Show modal message so user sees activity during ~1s flash.
  char display_name[64];
  bl_copy_str(display_name, sizeof(display_name), base);
  (void)bl_strip_extension_ci(display_name, ".bin");
  menu_show_message("Flashing", display_name);

  const int rc = flash_derived_bin_to_slot(sd, bin_path);
  if (rc != 0) {
    menu_show_message("Flash failed. See bootloader.log", nullptr, LIGHT_RED);
    bootlog("BOOTDBG: flash_derived_bin_to_slot returned %d for %s", rc, bin_path);
    return false;
  }

  if (!bl_write_firmware_info(source_uf2, bin_path)) {
    menu_show_message("Warning: could not persist firmware_info metadata.", nullptr, LIGHT_YELLOW);
  }

  menu_show_message("Flash successful. Rebooting...");
  sleep_ms(100);
  platform_reboot();
  return true; // unreachable, but keeps the compiler happy
}

bool bl_handle_boot_installed(SdFs *sd, const char *installed_bin_path) {
  (void)sd;

  if (installed_bin_path && installed_bin_path[0]) {
    menu_show_message("Booting selected firmware...");
    bootlog("BOOTDBG[%s]: enter->boot(installed) -> boot_firmware_slot(0x%08x)", kBootloaderBuildTag, kXipBase);
    if (!boot_firmware_slot(kXipBase)) {
      menu_show_message("Boot failed. Check flashed firmware image.", nullptr, LIGHT_RED);
      bootlog("BOOTDBG: enter->boot(installed) returned failure");
      return false;
    }
  } else {
    menu_show_message("Booting app slot...");
    bootlog("BOOTDBG[%s]: handoff(manual) -> boot_firmware_slot(0x%08x)", kBootloaderBuildTag, kXipBase);
    if (!boot_firmware_slot(kXipBase)) {
      menu_show_message("App-slot boot failed.", "Check flashed firmware image.", LIGHT_YELLOW);
      bootlog("BOOTDBG: handoff(manual) returned failure");
      return false;
    }
  }
  return true;
}
