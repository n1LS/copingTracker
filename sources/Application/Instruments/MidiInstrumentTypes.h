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
} midi_voice_t;