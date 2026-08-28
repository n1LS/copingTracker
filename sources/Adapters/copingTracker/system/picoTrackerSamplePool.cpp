/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "picoTrackerSamplePool.h"
#include "Adapters/copingTracker/system/picoTrackerProjectLoader.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#include "pico/platform.h"
#include <cstring>

#define MB 1024 * 1024

#define VERBOSE_FLASH_DEBUG 0

// Direction-agnostic lockout check: returns true if the *other* core
// relative to the caller is registered as a lockout victim.
static inline bool other_core_is_lockout_victim() {
  return multicore_lockout_victim_is_initialized(1 - get_core_num());
}

// Maximum sample storage per project (8MB limit for now)
#define SAMPLE_STORAGE_START_MB 8

// Define where sample storage begins in flash
// Use all flash available after binary for samples
extern char __flash_binary_end;
#define FLASH_TARGET_OFFSET                                                                                            \
  ((((uintptr_t)&__flash_binary_end - 0x10000000u) / FLASH_SECTOR_SIZE) + 1) * FLASH_SECTOR_SIZE

// Total flash size depends on hardware:
// - Raspberry Pi Pico: 2MB
// - picoTracker custom hardware: up to 16MB
// We'll detect actual size at runtime if needed

uint32_t picoTrackerSamplePool::flashEraseOffset_ = FLASH_TARGET_OFFSET;
uint32_t picoTrackerSamplePool::flashWriteOffset_ = FLASH_TARGET_OFFSET;
// Initial default value - will be properly set in the constructor based on
// actual flash size
uint32_t picoTrackerSamplePool::flashLimit_ = 0;
// From the SDK, values are not defined in the header file
#define FLASH_RUID_DUMMY_BYTES 4
#define FLASH_RUID_DATA_BYTES 8
#define FLASH_RUID_TOTAL_BYTES (1 + FLASH_RUID_DUMMY_BYTES + FLASH_RUID_DATA_BYTES)

uint32_t storage_get_flash_capacity() {
  uint8_t txbuf[FLASH_RUID_TOTAL_BYTES] = {0x9f};
  uint8_t rxbuf[FLASH_RUID_TOTAL_BYTES] = {0};
  flash_do_cmd(txbuf, rxbuf, FLASH_RUID_TOTAL_BYTES);

  return 1 << rxbuf[3];
}

picoTrackerSamplePool::picoTrackerSamplePool() : SamplePool() {
  // Detect the actual flash size at runtime
  uint32_t totalFlashSize = storage_get_flash_capacity();

  // Calculate the maximum usable flash for samples
  // This is either the 8MB limit or the actual available flash, whichever is
  // smaller
  uint32_t maxUsableFlash = SAMPLE_STORAGE_START_MB * MB;

  flashLimit_ = totalFlashSize;

  // Set the flash offset to maximum usable flash back from the top of the flash
  // or immediately after the firmware
  flashWriteOffset_ = flashEraseOffset_ =
      flashLimit_ < SAMPLE_STORAGE_START_MB * MB ? FLASH_TARGET_OFFSET : SAMPLE_STORAGE_START_MB * MB;

  Trace::Debug("Total flash size: %u bytes", totalFlashSize);
  Trace::Debug("Flash target offset: %u bytes", FLASH_TARGET_OFFSET);
  Trace::Debug("Max usable flash: %u bytes", maxUsableFlash);
  Trace::Debug("Flash limit set to: %u bytes", flashLimit_);
  Trace::Debug("Flash write offset set to: %u bytes", flashWriteOffset_);
}

void picoTrackerSamplePool::Reset() {
  count_ = 0;
  for (int i = 0; i < MAX_SAMPLES; i++) {
    wav_[i].Close();
    nameStore_[i][0] = '\0';
  };

  // Reset flash erase and write pointers when we close project
  flashEraseOffset_ = FLASH_TARGET_OFFSET;
  flashWriteOffset_ = FLASH_TARGET_OFFSET;
}

void picoTrackerSamplePool::updateStatus(uint32_t current, uint32_t total, const char *message) {
  // If running on core1 (during a project load), route progress into the
  // shared loader state so core0 can display it safely without race on
  // AppWindow's screen buffer. Otherwise, fall back to the base class
  // behavior (write directly to Status for runtime single-sample imports
  // on core0).
  if (get_core_num() == 1) {
    picoTrackerProjectLoader::UpdateProgress(current, total, message);
  } else {
    SamplePool::updateStatus(current, total, message);
  }
}

bool picoTrackerSamplePool::loadSample(const char *name) {
  Trace::Log("SAMPLEPOOL", "Loading sample into flash: %s", name);

  if (count_ == MAX_SAMPLES)
    return false;

  // Open and stat the WAV file — this is SD I/O and must happen fully
  // outside any lockout/interrupt-disabled window.
  auto res = wav_[count_].Open(name);
  if (!res) {
    Trace::Error("Failed to load sample:%s", name);
    return false;
  }
  strncpy(nameStore_[count_], name, MAX_INSTRUMENT_FILENAME_LENGTH);
  nameStore_[count_][MAX_INSTRUMENT_FILENAME_LENGTH] = '\0';
  count_++;

  updateStatus(importIndex, importCount, "Copying to flash" char_indicator_ellipsis_s);

  if (!LoadInFlash(&wav_[count_ - 1])) {
    Trace::Error("Failed loading sample into flash: %s", name);
    count_--;
    nameStore_[count_][0] = '\0';
    wav_[count_].Close();
    return false;
  }

  wav_[count_ - 1].Close();
  return true;
}

bool picoTrackerSamplePool::LoadInFlash(WavFile *wave) {

  uint32_t FlashBaseBufferSize = wave->GetDiskSize(-1);

  // Size actually occupied in flash
  uint32_t FlashPageBufferSize =
      ((FlashBaseBufferSize / FLASH_PAGE_SIZE) + ((FlashBaseBufferSize % FLASH_PAGE_SIZE) != 0)) * FLASH_PAGE_SIZE;

  if (flashWriteOffset_ + FlashPageBufferSize > flashLimit_) {
    return false;
  }

  // Set wave base
  wave->SetSampleBuffer((int16_t *)(XIP_BASE + flashWriteOffset_));

  // Determine if we need to pause the other core during flash operations.
  // The other core must be a registered lockout victim.
  bool needLockout = other_core_is_lockout_victim();

  // --- Erase: The other core must be paused (XIP is inaccessible during
  // flash erase on this chip). No SD I/O involved here.
  if (FlashPageBufferSize > (flashEraseOffset_ - flashWriteOffset_)) {
    uint32_t additionalData = FlashPageBufferSize - flashEraseOffset_ + flashWriteOffset_;
    uint32_t sectorsToErase =
        ((additionalData / FLASH_SECTOR_SIZE) + ((additionalData % FLASH_SECTOR_SIZE) != 0)) * FLASH_SECTOR_SIZE;

    if (needLockout)
      multicore_lockout_start_blocking();
    uint32_t irqs = save_and_disable_interrupts();
    flash_range_erase(flashEraseOffset_, sectorsToErase);
    restore_interrupts(irqs);
    if (needLockout)
      multicore_lockout_end_blocking();

    flashEraseOffset_ += sectorsToErase;
  }

  // Read samples from SD and program into flash, one page at a time.
  uint32_t br = 0;
  uint8_t readBuffer[BUFFER_SIZE];

  wave->Rewind();
  wave->Read(&readBuffer, BUFFER_SIZE, &br);
  while (br > 0) {
    uint32_t writeSize = br;
    // Adjust to page size
    writeSize = ((writeSize / FLASH_PAGE_SIZE) + ((writeSize % FLASH_PAGE_SIZE) != 0)) * FLASH_PAGE_SIZE;

    // --- Program: Only this brief operation requires the other core to be paused.
    // SD read happens fully outside this window.
    if (needLockout)
      multicore_lockout_start_blocking();
    uint32_t irqs = save_and_disable_interrupts();
    flash_range_program(flashWriteOffset_, (uint8_t *)readBuffer, writeSize);
    restore_interrupts(irqs);
    if (needLockout)
      multicore_lockout_end_blocking();

    flashWriteOffset_ += writeSize;
    // Next SD read happens outside any lockout/interrupt-disabled window.
    wave->Read(&readBuffer, BUFFER_SIZE, &br);
  }

  return true;
}

bool picoTrackerSamplePool::unloadSample(uint32_t index) {
  return false;
}

bool picoTrackerSamplePool::CheckSampleFits(int sampleSize) {
  // Calculate flash storage needed (round up to flash page size)
  uint32_t flashNeeded = ((sampleSize / FLASH_PAGE_SIZE) + ((sampleSize % FLASH_PAGE_SIZE) != 0)) * FLASH_PAGE_SIZE;

  // Check if there's enough space available
  uint32_t availableFlash = flashLimit_ - flashWriteOffset_;

  return flashNeeded <= availableFlash;
}
