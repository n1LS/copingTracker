/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include <cstdint>

// The engine runs at exactly this rate on every target, so it is a build time
// constant rather than something negotiated with the driver:
//
//  - on device, clk_sys is deliberately set to 220.5 MHz because it divides
//    evenly by this rate, giving an exact I2S clock (see platform.cpp)
//  - on host, SDL is opened with allowed_changes = 0, so it either grants this
//    rate or fails to open the device
//
// Anything that describes the rate of *foreign* audio data (the GM bank's
// 22050 Hz source PCM, an imported WAV's own header) is a different concept
// and must not use these.
constexpr uint32_t SAMPLE_RATE_HZ = 44100;
constexpr float SAMPLE_RATE_F = 44100.0f;

// Highest frequency the engine can represent. Filter cutoff mapping is
// expressed relative to this.
constexpr float NYQUIST_HZ = SAMPLE_RATE_F / 2.0f;
