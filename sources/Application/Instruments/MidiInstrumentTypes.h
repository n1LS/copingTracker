/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include "Externals/etl/include/etl/array.h"

#define MAX_MIDI_CHORD_NOTES 4

typedef struct midi_voice_t {
  etl::array<uint8_t, MAX_MIDI_CHORD_NOTES + 1> lastNotes_;
  uint8_t lastVolume_;
  bool first_;
  uint8_t _padding[1];
} midi_voice_t;

static_assert(sizeof(midi_voice_t) % 4 == 0, "Check sizeof(chiptune_voice_t) in error message - it must be a multiple of 4");
