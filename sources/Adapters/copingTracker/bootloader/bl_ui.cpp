/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#include "bl_ui.h"
#include "Adapters/copingTracker/platform/platform.h"
#include "Adapters/copingTracker/system/input.h"
#include "Application/Views/BaseClasses/View.h"
#include "Foundation/Types/Colors.h"
#include "bl_config.h"
#include "bl_flash_logic.h"
#include "bl_log.h"
#include "bl_menu.h"
#include "bl_path_utils.h"
#include "bl_sd_ops.h"
#include "hardware/structs/watchdog.h"
#include "hardware/watchdog.h"
#include "pico/time.h"
#include <cstring>

// ── App trace reporting ─────────────────────────────────────────────────────

static void report_app_boot_trace() {
  bootlog("BOOTDBG: wd_caused=%d scratch2=0x%08x scratch3=0x%08x", watchdog_caused_reboot() ? 1 : 0,
          watchdog_hw->scratch[2], watchdog_hw->scratch[3]);

  if (watchdog_hw->scratch[2] != kAppBootTraceMagic) {
    return;
  }
  const uint32_t stage = watchdog_hw->scratch[3];
  bootlog("BOOTDBG: app-trace stage=0x%08x", stage);
}

// ── Startup initialisation (shared between normal and auto-boot paths) ──────

static void startup_init(int &selected_file, Uf2FileEntry *uf2_files, int &uf2_count, bool &sd_ready,
                         bool &auto_boot_armed, uint32_t &auto_boot_deadline, char *installed_firmware,
                         char *installed_bin) {
  if (bl_mount_sd()) {
    sd_ready = true;
    if (bl_read_firmware_info(installed_firmware, 64, installed_bin, 80)) {
      auto_boot_armed = true;
      auto_boot_deadline = millis() + 3000;
    }
  } else {
    menu_show_sd_warning();
  }

  if (sd_ready) {
    const int inbox_count = bl_scan_uf2_inbox(uf2_files, kMaxUf2Files);
    if (bl_import_uf2_to_firmwares(uf2_files, inbox_count)) {
      auto_boot_armed = false;
    }

    uf2_count = bl_scan_firmware_bins(uf2_files, kMaxUf2Files);

    selected_file = 0;
    if (installed_bin[0]) {
      for (int i = 0; i < uf2_count; ++i) {
        if (bl_str_equals_ci(uf2_files[i].path, installed_bin)) {
          selected_file = i;
          break;
        }
      }
    }
  }
}

// ── Main loop ─────────────────────────────────────────────────────────────────

void bl_run_ui_loop() {
  int selected_file = 0;
  Uf2FileEntry uf2_files[kMaxUf2Files] = {};
  int uf2_count = 0;
  uint16_t stable_keys = 0;
  uint16_t pending_keys = 0;
  uint32_t pending_since_ms = 0;
  constexpr uint32_t kDebounceMs = 25;
  bool sd_ready = false;
  bool auto_boot_armed = false;
  uint32_t sd_last_check_ms = 0;
  uint32_t auto_boot_deadline = 0;
  char installed_firmware[64] = {0};
  char installed_bin[80] = {0};
  bool display_dirty = true;
  int displayed_auto_boot_seconds = -1;

  report_app_boot_trace();
  startup_init(selected_file, uf2_files, uf2_count, sd_ready, auto_boot_armed, auto_boot_deadline, installed_firmware,
               installed_bin);

  while (true) {
    const uint32_t now_ms = millis();
    const uint16_t raw_keys = scanKeys();
    if (raw_keys != pending_keys) {
      pending_keys = raw_keys;
      pending_since_ms = now_ms;
    }

    uint16_t pressed = 0;
    if (pending_keys != stable_keys && (now_ms - pending_since_ms) >= kDebounceMs) {
      const uint16_t old_keys = stable_keys;
      stable_keys = pending_keys;
      pressed = stable_keys & ~old_keys;
    }
    const uint16_t keys = stable_keys;

    // ── SD card hotplug detection (polled) ───────────────────────────────

    if (sd_ready && (now_ms - sd_last_check_ms) >= kSdPollMs) {
      sd_last_check_ms = now_ms;
      if (!bl_probe_sd_alive()) {
        sd_ready = false;
        menu_show_sd_warning();
      }
    }

    // ── Button dispatch ─────────────────────────────────────────────────

    if (pressed & (BM_UP | BM_DOWN)) {
      if (uf2_count > 0) {
        selected_file = (selected_file + (keys & BM_UP ? -1 : 1) + uf2_count) % uf2_count;
        display_dirty = true;
      }
    }

    // ALT+PLAY: enter firmware update mode (USB recovery)
    if ((pressed & BM_PLAY) && (keys & BM_ALT)) {
      sleep_ms(100);
      platform_bootloader();
    }

    // ALT+ENTER: flash the selected bin without auto-boot
    if ((pressed & BM_ENTER) && (keys & BM_ALT)) {
      if (uf2_count <= 0) {
        menu_show_message("No firmware in /firmwares. Add a UF2 to SD root, "
                          "reboot.",
                          nullptr, LIGHT_YELLOW);
      } else {
        bl_handle_flash_and_boot(&g_sd, uf2_files[selected_file].path);
        display_dirty = true;
      }
    }

    // ENTER (no ALT): flash (if needed) and boot selected firmware
    if ((pressed & BM_ENTER) && !(keys & BM_ALT)) {
      if (uf2_count <= 0) {
        menu_show_message("No firmware in /firmwares. Add a UF2 to SD root, "
                          "reboot.",
                          nullptr, LIGHT_YELLOW);
      } else {
        const char *bin_path = uf2_files[selected_file].path;

        if (installed_bin[0] && bl_str_equals_ci(bin_path, installed_bin)) {
          // Already installed — boot directly.
          bl_handle_boot_installed(&g_sd, bin_path);
        } else {
          bl_handle_flash_and_boot(&g_sd, bin_path);
        }
        display_dirty = true;
      }
    }

    // PLAY (no ALT): boot the app slot directly.
    if ((pressed & BM_PLAY) && !(keys & BM_ALT)) {
      bl_handle_boot_installed(&g_sd, nullptr);
    }

    // EDIT: reboot to firmware update (USB mass storage) mode.
    if (pressed == BM_EDIT) {
      sleep_ms(100);
      platform_bootloader();
    }

    // ── Auto-boot countdown ──────────────────────────────────────────────

    int auto_boot_timeout = -1;
    int auto_boot_seconds = -1;

    if (auto_boot_armed) {
      auto_boot_timeout = static_cast<int32_t>(auto_boot_deadline - now_ms);
      if (auto_boot_timeout < 0) {
        auto_boot_timeout = 0;
      }
      auto_boot_seconds = auto_boot_timeout / 1000 + 1;

      // Any key press aborts auto-boot.
      if (pressed) {
        auto_boot_armed = false;
        display_dirty = true;
      }
    }

    if (auto_boot_timeout == 0 && auto_boot_armed) {
      // Timeout reached — boot installed firmware.
      auto_boot_armed = false;
      bl_handle_boot_installed(&g_sd, nullptr);
    }

    if (auto_boot_seconds != displayed_auto_boot_seconds) {
      display_dirty = true;
    }

    // ── Render ───────────────────────────────────────────────────────────

    if (display_dirty) {
      menu_render_main(uf2_files, uf2_count, selected_file, installed_bin, sd_ready, auto_boot_timeout);
      display_dirty = false;
      displayed_auto_boot_seconds = auto_boot_seconds;
    }

    tight_loop_contents();
  }
}