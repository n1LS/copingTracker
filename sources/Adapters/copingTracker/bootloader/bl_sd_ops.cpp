/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#include "bl_sd_ops.h"
#include "bl_config.h"
#include "bl_log.h"
#include "bl_path_utils.h"
#include "bl_uf2_parser.h"
#include "bl_menu.h"
#include "Foundation/Types/Colors.h"
#include "Adapters/copingTracker/sdcard/sdcard.h"
#include <cstring>

// ── Global SD instance ──────────────────────────────────────────────────────

SdFs g_sd;

// ── Mount / probe ───────────────────────────────────────────────────────────

bool bl_mount_sd() {
  if (g_sd.begin(SD_CONFIG)) {
    return true;
  }
  if (!g_sd.card() || g_sd.sdErrorCode() != 0) {
    return false;
  }
  return static_cast<FsVolume *>(&g_sd)->begin(g_sd.card(), true, 0);
}

bool bl_probe_sd_alive() {
  SdCardInterface *card = g_sd.card();
  if (!card) {
    return false;
  }
  uint32_t dummy;
  return card->readOCR(&dummy);
}

// ── Utilities ───────────────────────────────────────────────────────────────

static bool has_uf2_extension(const char *name) {
  return bl_path_has_extension_ci(name, ".uf2");
}

// Convert "/foo.uf2" -> "/firmwares/foo.bin"
static void uf2_to_firmware_bin_path(const char *uf2_path, char *out,
                                     size_t out_size) {
  const char *base = bl_path_basename(uf2_path);
  bl_copy_str(out, out_size, kFirmwareDir);
  bl_append_str(out, out_size, "/");
  bl_append_str(out, out_size, base);
  (void)bl_replace_extension_ci(out, out_size, ".uf2", ".bin");
}

static void mark_uf2_failed(const char *uf2_path) {
  char failed_path[80];
  bl_copy_str(failed_path, sizeof(failed_path), uf2_path);
  bl_append_str(failed_path, sizeof(failed_path), ".fail");

  if (g_sd.exists(failed_path)) {
    g_sd.remove(failed_path);
  }
  (void)g_sd.rename(uf2_path, failed_path);
}

// ── Directory scanning ──────────────────────────────────────────────────────

int bl_scan_uf2_inbox(Uf2FileEntry *entries, int capacity) {
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

    if (!entry.isDirectory() && filename[0] != '.' &&
        has_uf2_extension(filename)) {
      bl_copy_str(entries[count].path, sizeof(entries[count].path), "/");
      bl_append_str(entries[count].path, sizeof(entries[count].path), filename);
      ++count;
    }

    entry.close();
  }

  cwd.close();
  return count;
}

int bl_scan_firmware_bins(Uf2FileEntry *entries, int capacity) {
  if (!g_sd.chdir(kFirmwareDir)) {
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
      bl_copy_str(entries[count].path, sizeof(entries[count].path),
                  kFirmwareDir);
      bl_append_str(entries[count].path, sizeof(entries[count].path), "/");
      bl_append_str(entries[count].path, sizeof(entries[count].path), filename);
      ++count;
    }
    entry.close();
  }

  cwd.close();
  g_sd.chdir("/");
  return count;
}

// ── Firmware directory & metadata ───────────────────────────────────────────

bool bl_ensure_firmware_dir() {
  if (g_sd.exists(kFirmwareDir)) {
    return true;
  }
  return g_sd.mkdir(kFirmwareDir, true);
}

bool bl_write_firmware_info(const char *last_uf2, const char *derived_path) {
  if (!bl_ensure_firmware_dir()) {
    return false;
  }

  FsFile info;
  if (!info.open(kFirmwareInfoFile, O_WRONLY | O_CREAT | O_TRUNC)) {
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

bool bl_read_firmware_info(char *last_uf2_out, size_t uf2_capacity,
                           char *derived_out, size_t derived_capacity) {
  if (last_uf2_out == nullptr || uf2_capacity == 0) {
    return false;
  }

  last_uf2_out[0] = 0;
  if (derived_out && derived_capacity > 0) {
    derived_out[0] = 0;
  }

  FsFile info;
  if (!info.open(kFirmwareInfoFile, O_RDONLY)) {
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
    } else if (derived_out && derived_capacity > 0 &&
               std::strncmp(line, "derived=", 8) == 0) {
      std::strncpy(derived_out, line + 8, derived_capacity - 1);
      derived_out[derived_capacity - 1] = 0;
      strip_nl(derived_out, derived_capacity);
    }
  }

  info.close();
  return found_uf2;
}

// ── UF2 import ──────────────────────────────────────────────────────────────

bool bl_import_uf2_to_firmwares(const Uf2FileEntry *inbox, int inbox_count) {
  if (inbox_count <= 0) {
    return false;
  }

  if (!bl_ensure_firmware_dir()) {
    return false;
  }

  char bin_path[80];
  bool any_imported = false;
  for (int i = 0; i < inbox_count; ++i) {
    uf2_to_firmware_bin_path(inbox[i].path, bin_path, sizeof(bin_path));

    // Skip if the binary already exists in the library.
    if (g_sd.exists(bin_path)) {
      continue;
    }

    menu_show_message("Importing", bl_path_basename(inbox[i].path));

    const int rc = convert_uf2_to_bin(&g_sd, inbox[i].path, kXipBase, bin_path);
    if (rc != 0) {
      // Conversion failed; rename the UF2 so it won't be retried.
      mark_uf2_failed(inbox[i].path);
    } else {
      // On success, remove the source UF2.
      g_sd.remove(inbox[i].path);
      any_imported = true;
    }
    menu_show_message("Done.", nullptr, WHITE);
  }

  return any_imported;
}