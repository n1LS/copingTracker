/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker Boot Manager
 */

#include "../system/input.h"
#include "Adapters/copingTracker/display/ili9341.h"
#include "Adapters/copingTracker/platform/platform.h"
#include "Adapters/copingTracker/sdcard/sdcard.h"
#include "Application/Views/BaseClasses/View.h"
#include "Externals/SdFat/src/SdFat.h"
#include "bootloader_gfx.h"
#include "bootloader_log.h"
#include "bootloader_menu.h"
#include "bsp/board.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/structs/watchdog.h"
#include "hardware/watchdog.h"
#include "path_utils.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "slot_boot.h"
#include <cstring>

#define APP_SLOT_ADDR 0x10000100u
#define FIRMWARE_DIR "/firmwares"
#define FIRMWARE_INFO_FILE "/firmwares/firmware_info.txt"

constexpr uint32_t kSdPollMs = 500;
constexpr uint32_t kMaxUf2Files = 16;
constexpr uint32_t kHeartbeatPeriodMs = 1000;
constexpr uint32_t kAppBootTraceMagic = 0x41505452u; // 'APTR'
constexpr const char *kBootloaderBuildTag = "BLD-2026-06-05-library-v4";

extern int copy_uf2_to_flash(const char *filename, uint32_t target_slot, const char *derived_output_path,
                             bool do_flash);

// Convert "/foo.uf2" -> "/firmwares/foo.bin".
static void uf2_to_firmware_bin_path(const char *uf2_path, char *out, size_t out_size) {
  const char *base = bl_path_basename(uf2_path);
  strlcpy(out, FIRMWARE_DIR, out_size);
  strlcat(out, "/", out_size);
  strlcat(out, base, out_size);
  (void)bl_replace_extension_ci(out, out_size, ".uf2", ".bin");
}

static SdFs g_sd;
bool auto_boot_armed = false;

static void mark_uf2_failed(const char *uf2_path) {
  char failed_path[80];
  strlcpy(failed_path, uf2_path, sizeof(failed_path));
  strlcat(failed_path, ".fail", sizeof(failed_path));

  if (g_sd.exists(failed_path)) {
    g_sd.remove(failed_path);
  }
  (void)g_sd.rename(uf2_path, failed_path);
}

static bool ensure_firmware_dir() {
  if (g_sd.exists(FIRMWARE_DIR)) {
    return true;
  }
  return g_sd.mkdir(FIRMWARE_DIR, true);
}

static bool write_firmware_info(const char *last_uf2, const char *derived_path) {
  if (!ensure_firmware_dir()) {
    return false;
  }

  FsFile info;
  if (!info.open(FIRMWARE_INFO_FILE, O_WRONLY | O_CREAT | O_TRUNC)) {
    return false;
  }

  info.print("last_uf2=");
  info.print(last_uf2 != nullptr ? last_uf2 : "");
  info.print("\n");
  info.print("derived=");
  info.print(derived_path != nullptr ? derived_path : "");
  info.print("\n");
  info.sync();
  info.close();
  return true;
}

static bool read_firmware_info(char *last_uf2_out, size_t uf2_capacity, char *derived_out, size_t derived_capacity) {
  if (last_uf2_out == nullptr || uf2_capacity == 0) {
    return false;
  }

  last_uf2_out[0] = 0;
  if (derived_out && derived_capacity > 0) {
    derived_out[0] = 0;
  }

  FsFile info;
  if (!info.open(FIRMWARE_INFO_FILE, O_RDONLY)) {
    return false;
  }

  char line[64] = {0};
  bool found_uf2 = false;
  while (info.fgets(line, sizeof(line)) > 0) {
    auto strip_nl = [](char *s, size_t cap) {
      const size_t len = std::strlen(s);
      if (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = 0;
      }
    };
    if (std::strncmp(line, "last_uf2=", 9) == 0) {
      std::strncpy(last_uf2_out, line + 9, uf2_capacity - 1);
      last_uf2_out[uf2_capacity - 1] = 0;
      strip_nl(last_uf2_out, uf2_capacity);
      found_uf2 = true;
    } else if (derived_out && derived_capacity > 0 && std::strncmp(line, "derived=", 8) == 0) {
      std::strncpy(derived_out, line + 8, derived_capacity - 1);
      derived_out[derived_capacity - 1] = 0;
      strip_nl(derived_out, derived_capacity);
    }
  }

  info.close();
  return found_uf2;
}

static bool has_uf2_extension(const char *name) {
  return bl_path_has_extension_ci(name, ".uf2");
}

static bool mount_sd_card() {
  if (g_sd.begin(SD_CONFIG)) {
    return true;
  }
  if (!g_sd.card() || g_sd.sdErrorCode() != 0) {
    return false;
  }
  return static_cast<FsVolume *>(&g_sd)->begin(g_sd.card(), true, 0);
}

// Low-level card presence probe: sends CMD13 over SDIO.
// Returns false as soon as the card is physically absent.
// chdir() and other FsVolume calls are cached in RAM and cannot detect removal.
static bool probe_sd_alive() {
  SdCardInterface *card = g_sd.card();
  if (!card) {
    return false;
  }
  uint32_t dummy;
  return card->readOCR(&dummy);
}

static int scan_uf2_files(Uf2FileEntry *entries, int capacity) {
  if (!g_sd.chdir("/")) {
    return 0;
  }

  FsFile cwd;
  if (!cwd.openCwd()) {
    return 0;
  }

  FsFile entry;
  int count = 0;
  char filename[64];

  while (entry.openNext(&cwd, O_RDONLY) && count < capacity) {
    filename[0] = 0;
    entry.getName(filename, sizeof(filename));

    // Ignore hidden files (dot-prefixed) in the UF2 inbox listing.
    if (!entry.isDirectory() && filename[0] != '.' && has_uf2_extension(filename)) {
      strlcpy(entries[count].path, "/", sizeof(entries[count].path));
      strlcat(entries[count].path, filename, sizeof(entries[count].path));
      ++count;
    }

    entry.close();
  }

  cwd.close();
  return count;
}

// Scan /firmwares for *.bin files (the imported library).
static int scan_firmware_bins(Uf2FileEntry *entries, int capacity) {
  if (!g_sd.chdir(FIRMWARE_DIR)) {
    return 0;
  }
  FsFile cwd;
  if (!cwd.openCwd()) {
    g_sd.chdir("/");
    return 0;
  }

  FsFile entry;
  int count = 0;
  char filename[64];
  while (entry.openNext(&cwd, O_RDONLY) && count < capacity) {
    filename[0] = 0;
    entry.getName(filename, sizeof(filename));
    const bool is_bin = bl_path_has_extension_ci(filename, ".bin");
    if (!entry.isDirectory() && is_bin) {
      strlcpy(entries[count].path, FIRMWARE_DIR, sizeof(entries[count].path));
      strlcat(entries[count].path, "/", sizeof(entries[count].path));
      strlcat(entries[count].path, filename, sizeof(entries[count].path));
      ++count;
    }
    entry.close();
  }

  cwd.close();
  g_sd.chdir("/");
  return count;
}

// Import new UF2 files from SD root to /firmwares/*.bin if not already present.
static void import_uf2_to_firmwares(const Uf2FileEntry *inbox, int inbox_count) {
  if (inbox_count <= 0) {
    return;
  }
  if (!ensure_firmware_dir()) {
    return;
  }

  char bin_path[80];
  for (int i = 0; i < inbox_count; ++i) {
    uf2_to_firmware_bin_path(inbox[i].path, bin_path, sizeof(bin_path));

    menu_show_message("Converting", inbox[i].path, WHITE);
    const int rc = copy_uf2_to_flash(inbox[i].path, APP_SLOT_ADDR, bin_path, false);
    if (rc != 0) {
      // on failure, remove the failed .bin (if any) and rename the .uf2 to
      // .uf2.fail to prevent repeated failed import attempts.
      g_sd.remove(bin_path);
      mark_uf2_failed(inbox[i].path);
    } else {
      // on success, remove the .uf2 from the inbox.
      g_sd.remove(inbox[i].path);
    }
  }
}

static void report_app_boot_trace() {
  bootlog("BOOTDBG: wd_caused=%d scratch2=0x%08x scratch3=0x%08x", watchdog_caused_reboot() ? 1 : 0,
          watchdog_hw->scratch[2], watchdog_hw->scratch[3]);

  if (watchdog_hw->scratch[2] != kAppBootTraceMagic) {
    return;
  }
  const uint32_t stage = watchdog_hw->scratch[3];
  bootlog("BOOTDBG: app-trace stage=0x%08x", stage);
}

int main(int argc, char *argv[]) {
  // Initialize microcontroller hardware
  board_init();

  // Do remaining platform init (clocks, display, GPIO, SD card)
  // NOTE: platform_init() initializes full audio/MIDI setup which we don't need
  // For now, we'll use a simplified init below
  platform_init();

  // platform_init() configures the display SPI/GPIO/PWM but does not bring
  // the ILI9341 controller up; the bootloader has no GUI window to do it,
  // so do it here.
  gfx_init();
  // Paint the title bar, static labels and key legend once. Subsequent
  // menu_render_main() calls only repaint the dynamic regions.
  menu_render_static();

  int selected_file = 0;
  Uf2FileEntry uf2_files[kMaxUf2Files] = {};
  // Stable-state debounce: a transition is only accepted after the raw
  // input has held the new value for at least kDebounceMs. This is robust
  // against contact bounce on both press AND release, unlike a time-based
  // lockout that can expire mid-bounce.
  uint16_t stable_keys = 0;
  uint16_t pending_keys = 0;
  uint32_t pending_since_ms = 0;
  constexpr uint32_t kDebounceMs = 25;
  bool sd_ready = false;
  bool sd_warning_active = false;
  uint32_t sd_last_check_ms = 0;
  uint32_t auto_boot_deadline = 0;
  char installed_firmware[64] = {0};
  char installed_bin[80] = {0};
  bool display_dirty = true;
  int displayed_auto_boot_seconds = -1;

  report_app_boot_trace();

  if (mount_sd_card()) {
    sd_ready = true;
    if (read_firmware_info(installed_firmware, sizeof(installed_firmware), installed_bin, sizeof(installed_bin))) {
      auto_boot_armed = true;
      const uint32_t now_ms = millis();
      auto_boot_deadline = now_ms + 3000;
    }
  } else {
    sd_warning_active = true;
    menu_show_sd_warning();
  }

  int uf2_count = 0;
  if (sd_ready) {
    const int inbox_count = scan_uf2_files(uf2_files, kMaxUf2Files);
    import_uf2_to_firmwares(uf2_files, inbox_count);

    uf2_count = scan_firmware_bins(uf2_files, kMaxUf2Files);

    // Preselect the currently installed firmware entry, if present.
    selected_file = 0;
    if (installed_bin[0]) {
      for (int i = 0; i < uf2_count; ++i) {
        if (bl_str_equals_ci(uf2_files[i].path, installed_bin)) {
          selected_file = i;
          break;
        }
      }
    }
    display_dirty = true;
  }

  while (true) {
    const uint32_t now_ms = millis();
    const uint16_t raw_keys = scanKeys();
    if (raw_keys != pending_keys) {
      pending_keys = raw_keys;
      pending_since_ms = now_ms;
    }
    uint16_t pressed = 0;

    if (pending_keys != stable_keys &&
        static_cast<int32_t>(now_ms - pending_since_ms) >= static_cast<int32_t>(kDebounceMs)) {
      const uint16_t prev_stable = stable_keys;
      stable_keys = pending_keys;
      pressed = stable_keys & static_cast<uint16_t>(~prev_stable);
    }
    const uint16_t keys = stable_keys;

    // Periodic SD liveness probe — catches hot-removal while the bootloader is running.
    if (sd_ready && static_cast<int32_t>(now_ms - sd_last_check_ms) >= static_cast<int32_t>(kSdPollMs)) {
      sd_last_check_ms = now_ms;
      if (!probe_sd_alive()) {
        sd_ready = false;
        sd_warning_active = true;
        auto_boot_armed = false;
        menu_show_sd_warning();
      }
    }

    // Abort auto-boot on any key press.
    if (pressed != 0 && auto_boot_armed) {
      menu_render_static();
      auto_boot_armed = false;
      // eat the event, abort press does not trigger any action
      pressed = 0;
      continue;
    }

    // SD warning is active: hold on this screen and reboot on any button.
    if (sd_warning_active) {
      if (pressed != 0) {
        platform_reboot();
      }
      tight_loop_contents();
      continue;
    }

    if (auto_boot_armed && static_cast<int32_t>(now_ms - auto_boot_deadline) >= 0) {
      menu_show_message("Auto-booting app slot...");

      if (!boot_firmware_slot(APP_SLOT_ADDR)) {
        menu_show_message("App-slot boot failed.", "Check flashed firmware image.", LIGHT_RED);
      }
    }

    // up down -> change selection
    constexpr uint8_t UP_OR_DOWN = BM_UP | BM_DOWN;
    if ((pressed & UP_OR_DOWN) && uf2_count > 0) {
      selected_file = (selected_file + (pressed & BM_UP ? -1 : 1) + uf2_count) % uf2_count;
      bootlog("Selected UF2: %s", uf2_files[selected_file].path);
      display_dirty = true;
    }

    // (install &) boot selected firmware
    if (pressed & BM_ENTER) {
      if (uf2_count <= 0) {
        menu_show_message("No firmware in /firmwares. Add a UF2 to SD root, reboot.", nullptr, LIGHT_YELLOW);
      } else {
        const char *bin_path = uf2_files[selected_file].path;
        const char *base = bl_path_basename(bin_path);

        // If the selected firmware is already in the app slot, boot directly.
        if (installed_bin[0] && bl_str_equals_ci(bin_path, installed_bin)) {
          menu_show_message("Booting selected firmware...");
          bootlog("BOOTDBG[%s]: enter->boot(installed) -> boot_firmware_slot(0x%08x)", kBootloaderBuildTag,
                  APP_SLOT_ADDR);
          if (!boot_firmware_slot(APP_SLOT_ADDR)) {
            menu_show_message("Boot failed. Check flashed firmware image.", nullptr, LIGHT_RED);
            bootlog("BOOTDBG: enter->boot(installed) returned failure");
          }
        } else {
          char source_uf2[80];
          strlcpy(source_uf2, "/", sizeof(source_uf2));
          strlcat(source_uf2, base, sizeof(source_uf2));
          (void)bl_replace_extension_ci(source_uf2, sizeof(source_uf2), ".bin", ".uf2");
          // Pre-flash modal so the user sees something happen during
          // the ~1s parse+flash blocking call. Use bare firmware name.
          char display_name[64];
          strlcpy(display_name, base, sizeof(display_name));
          (void)bl_strip_extension_ci(display_name, ".bin");
          menu_show_message("Flashing", display_name);

          const int rc = copy_uf2_to_flash(source_uf2, APP_SLOT_ADDR, bin_path, true);
          if (rc == 0) {
            if (!write_firmware_info(source_uf2, bin_path)) {
              menu_show_message("Warning: could not persist firmware_info metadata.", nullptr, LIGHT_YELLOW);
            }
            menu_show_message("Flash successful. Rebooting...");
            sleep_ms(100);
            platform_reboot();
          } else {
            menu_show_message("Flash failed. See bootloader.log", nullptr, LIGHT_RED);
          }
        }
      }
    }

    if (pressed & BM_PLAY) {
      menu_show_message("Booting app slot...");
      bootlog("BOOTDBG[%s]: handoff(manual) -> boot_firmware_slot(0x%08x)", kBootloaderBuildTag, APP_SLOT_ADDR);
      if (!boot_firmware_slot(APP_SLOT_ADDR)) {
        menu_show_message("App-slot boot failed.", "Check flashed firmware image.", LIGHT_YELLOW);
        bootlog("BOOTDBG: handoff(manual) returned failure");
      }
    }

    // reboot to firmware update
    if (pressed & BM_EDIT) {
      sleep_ms(100);
      platform_bootloader();
    }

    int auto_boot_timeout = -1;
    int auto_boot_seconds = -1;

    if (auto_boot_armed) {
      auto_boot_timeout = static_cast<int32_t>(auto_boot_deadline - now_ms);
      if (auto_boot_timeout < 0) {
        auto_boot_timeout = 0;
      }
      auto_boot_seconds = auto_boot_timeout / 1000 + 1;
    }

    if (auto_boot_seconds != displayed_auto_boot_seconds) {
      display_dirty = true;
    }

    if (display_dirty) {
      menu_render_main(uf2_files, uf2_count, selected_file, installed_bin, sd_ready, auto_boot_timeout);
      display_dirty = false;
      displayed_auto_boot_seconds = auto_boot_seconds;
    }

    tight_loop_contents();
  }

  return 0;
}
