/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#include "bl_uf2_parser.h"
#include "Adapters/copingTracker/sdcard/sdcard.h"
#include "Externals/SdFat/src/SdFat.h"
#include "bl_config.h"
#include "bl_flash_writer.h"
#include "bl_log.h"
#include "hardware/flash.h"
#include <cstdint>
#include <cstdio>
#include <cstring>

// UF2 format magic numbers (local to this module).
constexpr uint32_t kUf2MagicStart0 = 0x0A324655u;
constexpr uint32_t kUf2MagicStart1 = 0x9E5D5157u;
constexpr uint32_t kUf2MagicEnd = 0x0AB16F30u;
static FsFile g_file;
static FsFile g_derived;
static uint8_t g_uf2_block[kUf2BlockSize];
static uint8_t g_flash_page[FLASH_PAGE_SIZE];
static uint8_t g_fill_chunk[kFillChunkSize];

uint32_t read_le32(const uint8_t *p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8u) | (static_cast<uint32_t>(p[2]) << 16u) |
         (static_cast<uint32_t>(p[3]) << 24u);
}

bool is_valid_uf2_block(const uint8_t *block) {
  const uint32_t magic0 = read_le32(block + 0);
  const uint32_t magic1 = read_le32(block + 4);
  const uint32_t magic_end = read_le32(block + 508);
  return magic0 == kUf2MagicStart0 && magic1 == kUf2MagicStart1 && magic_end == kUf2MagicEnd;
}

bool is_sd_ready(SdFs *sd) {
  if (!sd) {
    return false;
  }
  // Assume SD is already mounted by the caller; just verify it's valid
  return sd->card() != nullptr;
}

bool validate_uf2_targets_for_app_slot(uint32_t min_addr, uint32_t max_addr, uint32_t target_slot) {
  const uint32_t slot_start = target_slot;
  const uint32_t slot_end = target_slot + kAppSlotSize;

  if (max_addr > slot_end) {
    bootlog("UF2: image address range [0x%08x .. 0x%08x) is outside app slot "
            "[0x%08x .. 0x%08x)\n",
            min_addr, max_addr, slot_start, slot_end);
    bootlog("UF2: this UF2 is not linked for this app slot.\n");
    return false;
  }

  bootlog("UF2: preflight OK for app slot. image range [0x%08x .. 0x%08x)\n", min_addr, max_addr);
  return true;
}

int flash_derived_bin_to_slot(SdFs *sd, const char *bin_path) {
  if (!is_sd_ready(sd)) {
    bootlog("BIN: SD not ready\n");
    return -1;
  }

  FsFile bin_file;
  if (!bin_file.open(sd, bin_path, O_RDONLY)) {
    bootlog("BIN: failed to open %s\n", bin_path);
    return -1;
  }

  const uint32_t bin_size = static_cast<uint32_t>(bin_file.fileSize());
  if (bin_size == 0 || bin_size > kAppSlotSize) {
    bootlog("BIN: invalid size %u for %s\n", bin_size, bin_path);
    bin_file.close();
    return -1;
  }

  if (erase_firmware_range(kXipBase, bin_size) != 0) {
    bootlog("BIN: erase failed for slot 0x%08x size %u\n", kXipBase, bin_size);
    bin_file.close();
    return -1;
  }

  uint32_t offset = 0;
  bootlog("BIN: flashing %u bytes from %s starting at 0x%08x\n", bin_size, bin_path, kXipBase);
  while (offset < bin_size) {
    std::memset(g_flash_page, 0xFF, sizeof(g_flash_page));

    const uint32_t remaining = bin_size - offset;
    const uint32_t to_read = (remaining < FLASH_PAGE_SIZE) ? remaining : FLASH_PAGE_SIZE;
    const int read_bytes = bin_file.read(g_flash_page, static_cast<size_t>(to_read));
    if (read_bytes != static_cast<int>(to_read)) {
      bootlog("BIN: short read at offset %u (expected %u, got %d)\n", offset, to_read, read_bytes);
      bin_file.close();
      return -1;
    }

    const uint32_t absolute = kXipBase + offset;
    bootlog("BIN: flashing chunk at offset=%u absolute=0x%08x to_read=%u\n", offset, absolute, to_read);
    if (write_firmware_chunk(absolute, g_flash_page, to_read) != 0) {
      bootlog("BIN: write failed at 0x%08x\n", absolute);
      bin_file.close();
      return -1;
    }

    if (verify_firmware_chunk(absolute, g_flash_page, to_read) != 0) {
      bootlog("BIN: verify failed at 0x%08x\n", absolute);
      bin_file.close();
      return -1;
    }

    offset += to_read;
  }

  bin_file.close();
  bootlog("BIN: flashed %u bytes from %s to app slot 0x%08x\n", bin_size, bin_path, kXipBase);
  return 0;
}

// Parse UF2 from SD, validate slot range, write derived .bin, and
// optionally flash app slot.
int convert_uf2_to_bin(SdFs *sd, const char *filename, uint32_t target_slot, const char *derived_output_path) {
  if (!is_sd_ready(sd)) {
    bootlog("UF2: SD not ready\n");
    return -1;
  }

  bootlog("UF2: converting %s -> %s\n", filename, derived_output_path);

  if (!g_file.open(sd, filename, O_RDONLY)) {
    bootlog("UF2: failed to open %s\n", filename);
    return -1;
  }

  uint32_t min_addr = 0xFFFFFFFFu;
  uint32_t max_addr = 0u;
  uint32_t block_count = 0;

  while (g_file.read(g_uf2_block, kUf2BlockSize) == static_cast<int>(kUf2BlockSize)) {
    if (!is_valid_uf2_block(g_uf2_block)) {
      continue;
    }

    const uint32_t target_addr = read_le32(g_uf2_block + 12);
    const uint32_t payload_size = read_le32(g_uf2_block + 16);

    if (payload_size == 0 || payload_size > 476u) {
      bootlog("UF2: found a broken block payload size");
      continue;
    }

    if (target_addr < min_addr) {
      min_addr = target_addr;
    }
    if (target_addr + payload_size > max_addr) {
      max_addr = target_addr + payload_size;
    }
    ++block_count;
  }

  if (block_count == 0 || min_addr == 0xFFFFFFFFu || max_addr <= min_addr) {
    bootlog("UF2: no valid UF2 blocks\n");
    g_file.close();
    return -1;
  }

  const uint32_t image_size = max_addr - min_addr;

  // The derived file starts at 0x10000000 because it contains boot2.
  const uint32_t derived_size = max_addr - kBoot2Address;

  if (image_size > kAppSlotSize + kBoot2Size) {
    bootlog("UF2: image too large (%u bytes)\n", image_size);
    g_file.close();
    return -1;
  }

  if (derived_size == 0 || derived_size > kAppSlotSize + kBoot2Size) {
    bootlog("UF2: invalid derived size (%u bytes)\n", derived_size);
    g_file.close();
    return -1;
  }

  if (!validate_uf2_targets_for_app_slot(min_addr, max_addr, target_slot)) {
    g_file.close();
    return -1;
  }

  // create target
  if (!g_derived.open(sd, derived_output_path, O_WRONLY | O_CREAT | O_TRUNC)) {
    bootlog("UF2: failed to create derived artifact %s\n", derived_output_path);
    g_file.close();
    return -1;
  }

  /*
   * The derived image starts at 0x10000000.
   *
   * Copy the currently installed boot2 into the first 256 bytes.
   * This is important because erasing the first flash sector will
   * also erase boot2.
   */
  const uint8_t *current_boot2 = reinterpret_cast<const uint8_t *>(kBoot2Address);
  bootlog("UF2: copying current boot2 from 0x%08x\n", kBoot2Address);
  if (g_derived.write(current_boot2, kBoot2Size) != kBoot2Size) {
    bootlog("UF2: failed writing current boot2 to derived artifact\n");
    g_derived.close();
    g_file.close();
    return -1;
  }

  g_file.rewind();
  uint32_t current_file_pos = kBoot2Size;

  while (g_file.read(g_uf2_block, kUf2BlockSize) == static_cast<int>(kUf2BlockSize)) {
    if (!is_valid_uf2_block(g_uf2_block)) {
      continue;
    }

    const uint32_t target_addr = read_le32(g_uf2_block + 12);
    const uint32_t payload_size = read_le32(g_uf2_block + 16);

    if (payload_size == 0 || payload_size > 476u) {
      continue;
    }

    // Skip blocks that target boot2 range (first 256 bytes).
    const uint32_t boot2_end = kBoot2Address + kBoot2Size;
    if (target_addr < boot2_end) {
      const uint32_t overlap = boot2_end - target_addr;
      if (overlap >= payload_size) {
        continue; // Entire block is in boot2 range; skip it.
      }
      // Partial overlap: write only the part after boot2.
      const uint32_t write_offset = overlap;
      const uint32_t write_size = payload_size - overlap;
      const uint32_t file_offset = kBoot2Size;

      if (current_file_pos != file_offset) {
        g_derived.seekSet(file_offset);
        current_file_pos = file_offset;
      }

      if (g_derived.write(g_uf2_block + 32 + write_offset, write_size) != write_size) {
        bootlog("UF2: write failed at 0x%08x\n", target_addr);
        g_derived.close();
        g_file.close();
        return -1;
      }
      current_file_pos += write_size;
    } else {
      // Block is entirely in app range.
      uint32_t file_offset = target_addr - kBoot2Address;

      if (current_file_pos != file_offset) {
        g_derived.seekSet(file_offset);
        current_file_pos = file_offset;
      }

      if (g_derived.write(g_uf2_block + 32, payload_size) != payload_size) {
        bootlog("UF2: write failed at 0x%08x\n", target_addr);
        g_derived.close();
        g_file.close();
        return -1;
      }
      current_file_pos += payload_size;
    }
  }

  g_derived.sync();
  g_derived.close();
  g_file.close();

  bootlog("UF2: imported %u blocks from %s\n", block_count, filename);
  bootlog("UF2: derived artifact created at %s\n", derived_output_path);

  return 0;
}