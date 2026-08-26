/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#ifndef PATCHBAY_BL_SD_OPS_H
#define PATCHBAY_BL_SD_OPS_H

#include "Externals/SdFat/src/SdFat.h"
#include "bl_menu.h"
#include <cstddef>
#include <cstdint>

// The single SdFs instance shared across the bootloader.
extern SdFs g_sd;

// ── Mount / probe ───────────────────────────────────────────────────────────

// Mount the SD card. Returns true on success.
bool bl_mount_sd();

// Low-level card presence probe via CMD13. Returns false if card is gone.
bool bl_probe_sd_alive();

// ── Directory scanning ──────────────────────────────────────────────────────

// Scan SD root for .uf2 files (the "inbox"). Returns the entry count.
int bl_scan_uf2_inbox(Uf2FileEntry *entries, int capacity);

// Scan /firmwares for .bin files (the imported library). Returns entry count.
int bl_scan_firmware_bins(Uf2FileEntry *entries, int capacity);

// ── Firmware directory & metadata ───────────────────────────────────────────

// Create /firmwares if it does not exist.
bool bl_ensure_firmware_dir();

// Persist the last-flashed UF2 info to /firmwares/firmware_info.txt.
bool bl_write_firmware_info(const char *last_uf2, const char *derived_path);

// Read persisted firmware info. Returns true if last_uf2 was found.
bool bl_read_firmware_info(char *last_uf2_out, size_t uf2_capacity, char *derived_out, size_t derived_capacity);

// ── UF2 import ──────────────────────────────────────────────────────────────

// Import new UF2 files from SD root to /firmwares/*.bin.  Skips files whose
// derived bin already exists.  Returns true if at least one file was imported.
bool bl_import_uf2_to_firmwares(const Uf2FileEntry *inbox, int inbox_count);

#endif // PATCHBAY_BL_SD_OPS_H
