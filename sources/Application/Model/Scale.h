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

struct Scale {
  const char *name;
  bool steps[12];
};

// Source of scales in original release:
// https://upload.wikimedia.org/wikipedia/commons/thumb/3/35/PitchConstellations.svg/1280px-PitchConstellations.svg.png

inline const Scale scales[] = {
    // common
    {"Chromatic", {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
    {"Ionian", {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1}},
    {"Aeolian", {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0}},
    {"Major Penta", {1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0}},
    {"Minor Penta", {1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0}},
    {"Blues", {1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0}},
    {"Dorian", {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0}},
    {"Mixolydian", {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0}},
    {"Phrygian", {1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0}},
    {"Lydian", {1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1}},
    {"Harm. Minor", {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1}},
    {"Melodic Minor", {1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1}},

    // fairly common
    {"Whole Tone", {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}},
    {"Octatonic", {1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1}},
    {"Phryg. Dom.", {1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0}},
    {"Locrian", {1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0}},
    {"Bebop Dom.", {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1}},
    {"Harm. Major", {1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1}},
    {"Dbl.Harmonic", {1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1}},
    {"Altered", {1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0}},

    // other
    {"Acoustic", {1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0}},
    {"Adonal Mal.", {1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0}},
    {"Algerian", {1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1}},
    {"Augmented", {1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1}},
    {"Enigmatic", {1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1}},
    {"Flamenco", {1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1}},
    {"Gypsy", {1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0}},
    {"Half Dimin.", {1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0}},
    {"Hirajoshi", {1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0}},
    {"Hung. Gypsy", {1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1}},
    {"Hung. Minor", {1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1}},
    {"Insen", {1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0}},
    {"Istrian", {1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0}},
    {"Iwato", {1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0}},
    {"Lyd. Aug.", {1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1}},
    {"Major Bebop", {1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1}},
    {"MajorLocrian", {1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0}},
    {"Mel. Min. Asc", {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1}},
    {"Neap. Major", {1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}},
    {"Neap. Minor", {1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1}},
    {"Persian", {1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1}},
    {"Prometheus", {1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0}},
    {"Tritone", {1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0}},
    {"Ukrainian", {1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0}},
};

constexpr int numScales = sizeof(scales) / sizeof(scales[0]);
extern const std::array<const char *, numScales> scaleNames;

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
