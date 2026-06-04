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

#include "Scale.h"
#include "System/Console/n_assert.h"

// Source of scales in original release:
// https://upload.wikimedia.org/wikipedia/commons/thumb/3/35/PitchConstellations.svg/1280px-PitchConstellations.svg.png

// todo: shorten names to 16 chars
const char *scaleNames[numScales] = {
    "None (Chromatic)",
    "Acoustic",
    "Adonal malakh",
    "Aeolian mode (minor)",
    "Algerian",
    "Altered",
    "Augmented",
    "Bebop dominant",
    "Blues",
    "Dorian",
    "Double harmonic",
    "Enigmatic",
    "Flamenco",
    "Gypsy",
    "Half diminished",
    "Harmonic major",
    "Harmonic minor",
    "Hirajoshi",
    "Hungarian gypsy",
    "Hungarian minor",
    "Insen",
    "Ionian mode (major)",
    "Istrian",
    "Iwato",
    "Locrian",
    "Lydian augmented",
    "Lydian",
    "Major bebop",
    "Major locrian",
    "Major pentatonic",
    "Melodic minor",
    "Melodic minor (asc)",
    "Minor pentatonic",
    "Mixolydian",
    "Neapolitan major",
    "Neapolitan minor",
    "Octatonic",
    "Persian",
    "Phrygian dominant",
    "Phrygian",
    "Prometheus",
    "Tritone",
    "Ukranian",
    "Whole tone"};

const bool scaleSteps[numScales][12] = {
    // 0  1  2  3  4  5  6  7  8  9  10 11
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},  // None (Chromatic)
                                           // 0     2     4     6  7     9
    {1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0},  // Acoustic
                                           // 0     2     4  5     7  8     10
    {1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0},  // Adonal malakh
                                           // 0     2  3     5     7  8            10
    {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0},  // Aeolian mode (minor)
                                           // 0     2      3                  6      7    8                   11
    {1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1},  // Algerian
                                           // 0  1            3     4           6            8            10
    {1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0},  // Altered
                                           // 0        3     4                   7     8                    11
    {1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1},  // Augmented
                                           // 0     2           4      5            7            9     10    11
    {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1},  // Bebop dominant
                                           // 0        3           5     6      7                  10
    {1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0},  // Blues
                                           // 0     2  3            5            7           9     10
    {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0},  // Dorian
                                           // 0  1                    4     5            7    8                   11
    {1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1},  // Double harmonic
                                           // 0  1                    4           6            8            10    11
    {1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1},  // Enigmatic
                                           // 0  1                    4    5             7     8                  11
    {1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1},  // Flamenco
                                           // 0     2      3                   6     7     8           10
    {1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0},  // Gypsy
                                           // 0     2      3           5      6           8            10
    {1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0},  // Half diminished
                                           // 0     2            4      5            7     8                  11
    {1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1},  // Harmonic major
                                           // 0     2     3             5           7      8                  11
    {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1},  // Harmonic minor
                                           // 0     2     3                          7     8
    {1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0},  // Hirajoshi
                                           // 0     2  3                   6     7      8                  11
    {1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1},  // Hungarian gypsy
                                           // 0            2    3                   6     7      8                  11
    {1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1},  // Hungarian minor
                                           // 0    1                           5           7                   10
    {1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0},  // Insen
                                           // 0            2            4     5            7           9            11
    {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1},  // Ionian mode (major)
                                           // 0     1            3     4           6      7
    {1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0},  // Istrian
                                           // 0    1                          5      6                         10
    {1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0},  // Iwato
                                           // 0    1            3             5     6      7                 10
    {1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0},  // Locrian
                                           // 0           2            4            6            8      9           11
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1},  // Lydian augmented
                                           // 0           2            4            6      7            9           11
    {1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1},  // Lydian
                                           // 0           2            4     5             7    8     9            11
    {1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1},  // Major bebop
                                           // 0           2            4     5      6           8            10
    {1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0},  // Major locrian
                                           // 0           2            4                   7            9
    {1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0},  // Major pentatonic
                                           // 0           2      3            5           7     8      9    10    11
    {1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1},  // Melodic minor
                                           // 0     2  3     5     7     9     11
    {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1},  // Melodic minor (asc)
                                           // 0        3            5           7                   10
    {1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0},  // Minor pentatonic
                                           // 0     2           4      5           7            9     10
    {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0},  // Mixolydian
                                           // 0  1            3            5            7            9            11
    {1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},  // Neapolitan major
                                           // 0  1            3            5            7      8                  11
    {1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1},  // Neapolitan minor
                                           // 0     2     3            5      6            8     9           11
    {1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1},  // Octatonic
                                           // 0  1                   4     5     6            8                   11
    {1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1},  // Persian
                                           // 0  1                   4     5            7     8            10
    {1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0},  // Phrygian dominant
                                           // 0  1            3            5            7     8            10
    {1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0},  // Phrygian
                                           // 0     2            4            6                    9    10
    {1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0},  // Prometheus
                                           // 0  1                   4            6     7                   10
    {1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0},  // Tritone
                                           // 0     2     3                   6     7            9     10
    {1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0},  // Ukranian
                                           // 0     2            4            6            8            10
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}}; // Whole tone

// Return the offset from the root note in semitones for the given scale and
// "scale number", taking into account the scale root
uint8_t getSemitonesOffset(uint8_t scale, uint8_t number, uint8_t root) {

  // check for valid ranges of scale, number and root
  if (scale >= numScales || number >= 12 || root >= 12) {
    NAssert(0);
    return 0;
  }

  // Find the nth note in the scale (where n is the number parameter)
  uint8_t i = 0;
  uint8_t foundNotes = 0;

  // Find the nth note in the scale
  while (foundNotes < number) {
    i++;
    // Adjust for the root note by shifting the scale pattern
    // For root = 0 (C), this simplifies to just checking scaleSteps[scale][i %
    // 12]
    if (scaleSteps[scale][(i + 12 - root) % 12]) {
      foundNotes++;
    }
  }

  return i;
}
