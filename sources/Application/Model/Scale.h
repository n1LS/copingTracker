/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef SCALE_VIEW_H
#define SCALE_VIEW_H

#include "Application/Utils/char.h"
#include <cstdint>

const int numScales = 44;
extern const char *const scaleNames[numScales];
extern const bool scaleSteps[numScales][12];

// Function that calculates semitone offset based on scale, position in scale,
// and root note
uint8_t getSemitonesOffset(uint8_t scale, uint8_t number, uint8_t root);

// As above, but walking down the scale. The returned offset is the distance
// *below* the root, so callers subtract it.
uint8_t getSemitonesOffsetDown(uint8_t scale, uint8_t number, uint8_t root);

// Whether a note (any MIDI note, also below 0 or above 127) is one of the notes of the scale around the root note
bool isInScale(int note, int scale, int root);

// Moves a note that is not in the scale to a note of the scale: upwards with a direction above 0, downwards with a
// direction below 0 and to the nearest with 0 (downwards if both are as far). A note that is in the scale stays.
int snapToScale(int note, int scale, int root, int direction);

// The note a note of a phrase becomes: first the transposition of the chain step is added, with scaleAware the result
// is mapped into the scale (in the direction of that transposition, the nearest without one), and last the
// transposition of the project is added, it moves everything and is not mapped. The stored note is not changed.
int transposeNote(int note, int chainTransposition, bool scaleAware, int scale, int root, int projectTransposition);

#endif
