/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "HostSamplePool.h"
#include "Application/Instruments/WavFile.h"
#include "System/Console/Trace.h"
#include "System/FileSystem/FileSystem.h"
#include <cstring>

HostSamplePool::HostSamplePool() : SamplePool(), totalUsed_(0) {
  sampleBuffers_.resize(MAX_SAMPLES);
}

HostSamplePool::~HostSamplePool() {
  Reset();
}

void HostSamplePool::Reset() {
  for (int i = 0; i < MAX_SAMPLES; i++) {
    sampleBuffers_[i].clear();
  }
  totalUsed_ = 0;
  count_ = 0;
}

bool HostSamplePool::CheckSampleFits(int sampleSize) {
  return (totalUsed_ + sampleSize) < MAX_POOL_SIZE;
}

uint32_t HostSamplePool::GetAvailableSampleStorageSpace() {
  return MAX_POOL_SIZE - totalUsed_;
}

void HostSamplePool::updateStatus(uint32_t current, uint32_t total, const char *message) {
  if (message) {
    Trace::Debug("SamplePool: [%u/%u] %s", current, total, message);
  }
}

bool HostSamplePool::loadSample(const char *name) {
  if (!name || count_ >= MAX_SAMPLES)
    return false;

  WavFile *wav = &wav_[count_];
  auto res = wav->Open(name);
  if (!res) {
    Trace::Error("Failed to open sample: %s", name);
    return false;
  }

  uint32_t sampleSize = wav->GetDiskSize(-1);
  if (!CheckSampleFits(sampleSize)) {
    Trace::Error("Sample too large: %s (%u bytes)", name, sampleSize);
    wav->Close();
    return false;
  }

  sampleBuffers_[count_].resize(sampleSize);
  wav->SetSampleBuffer((int16_t *)sampleBuffers_[count_].data());
  totalUsed_ += sampleSize;

  strncpy(nameStore_[count_], name, MAX_INSTRUMENT_FILENAME_LENGTH);
  nameStore_[count_][MAX_INSTRUMENT_FILENAME_LENGTH] = 0;
  names_[count_] = nameStore_[count_];

  count_++;
  return true;
}

bool HostSamplePool::unloadSample(uint32_t index) {
  if (index >= count_)
    return false;

  uint32_t sampleSize = wav_[index].GetDiskSize(-1);
  sampleBuffers_[index].clear();
  totalUsed_ -= sampleSize;

  return true;
}
