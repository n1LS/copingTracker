/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the PatchBay Boot Manager
 */

#ifndef PATCHBAY_BL_CONFIG_H
#define PATCHBAY_BL_CONFIG_H

#include <cstdint>

// ── Flash layout ────────────────────────────────────────────────────────────

// Base of the XIP-mapped flash address space.
constexpr uint32_t kXipBase = 0x10000000u;

// Boot2 region occupies the first 256 bytes of flash.
constexpr uint32_t kBoot2Size = 256u;
constexpr uint32_t kBoot2Address = kXipBase;

// Application slot: starts right after boot2 (kXipBase + kBoot2Size).
constexpr uint32_t kAppSlotBase = kXipBase + kBoot2Size; // 0x10000100
constexpr uint32_t kAppSlotSize = 0x00FEFF00u;

// Bootloader region (top 64 KB of the 16 MB flash — never written by firmware
// update code).
constexpr uint32_t kBootloaderBase = 0x10FF0000u;

// ── SD card paths ───────────────────────────────────────────────────────────

constexpr const char *kFirmwareDir = "/firmwares";
constexpr const char *kFirmwareInfoFile = "/firmwares/firmware_info.txt";
constexpr const char *kBootlogPath = "/copingTracker/boot.log";

// ── Timing & limits ─────────────────────────────────────────────────────────

constexpr uint32_t kSdPollMs = 500;
constexpr uint32_t kMaxUf2Files = 16;
constexpr uint32_t kHeartbeatPeriodMs = 1000;
constexpr uint32_t kBootlogBufferSize = 256;

// ── Misc ────────────────────────────────────────────────────────────────────

constexpr uint32_t kAppBootTraceMagic = 0x41505452u; // 'APTR'
constexpr const char *kBootloaderBuildTag = "BLD-2026-06-05-library-v4";

// UF2 block constants.
constexpr uint32_t kUf2BlockSize = 512u;
constexpr uint32_t kUf2MaxPayload = 476u;

constexpr uint32_t kFillChunkSize = 256u;

#endif // PATCHBAY_BL_CONFIG_H
