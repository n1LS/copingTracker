/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef HOST_SAMPLE_POOL_H_
#define HOST_SAMPLE_POOL_H_

#include "Application/Instruments/SamplePool.h"
#include <memory>
#include <vector>

class HostSamplePool : public SamplePool {
public:
  HostSamplePool();
  virtual ~HostSamplePool();

  virtual void Reset() override;
  virtual bool CheckSampleFits(int sampleSize) override;
  virtual uint32_t GetAvailableSampleStorageSpace() override;
  virtual void updateStatus(uint32_t current, uint32_t total, const char *message) override;

protected:
  virtual bool loadSample(const char *name) override;
  virtual bool unloadSample(uint32_t index) override;

private:
  static constexpr uint32_t MAX_POOL_SIZE = 256 * 1024 * 1024;
  std::vector<std::vector<uint8_t>> sampleBuffers_;
  uint32_t totalUsed_;
};

#endif
