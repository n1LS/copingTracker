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

template <size_t... I> constexpr auto makeScaleNames(std::index_sequence<I...>) {
  return std::array<const char *, sizeof...(I)>{scales[I].name...};
}

extern const std::array<const char *, numScales> scaleNames = makeScaleNames(std::make_index_sequence<numScales>{});

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
    // For root = 0 (C), this simplifies to just checking scales[scale].steps[i %
    // 12]
    if (scales[scale].steps[(i + 12 - root) % 12]) {
      foundNotes++;
    }
  }

  return i;
}

// Mirror of getSemitonesOffset walking downwards: returns how many semitones
// *below* the root the nth scale note sits, so a caller subtracts the result.
uint8_t getSemitonesOffsetDown(uint8_t scale, uint8_t number, uint8_t root) {

  // check for valid ranges of scale, number and root
  if (scale >= numScales || number >= 12 || root >= 12) {
    NAssert(0);
    return 0;
  }

  uint8_t i = 0;
  uint8_t foundNotes = 0;

  // Walk down one semitone at a time, counting the notes that are in the
  // scale. Every scale contains its own root, so this always terminates.
  while (foundNotes < number) {
    i++;
    if (scales[scale].steps[(12 - ((i + root) % 12)) % 12]) {
      foundNotes++;
    }
  }

  return i;
}

bool isInScale(int note, int scale, int root) {
  if (scale < 0 || scale >= numScales || root < 0 || root >= 12) {
    return true;
  }
  return scales[scale].steps[((note - root) % 12 + 12) % 12];
}

int snapToScale(int note, int scale, int root, int direction) {
  if (isInScale(note, scale, root)) {
    return note;
  }
  // every scale contains its root, so a note is found within an octave
  if (direction > 0) {
    for (int i = 1; i < 12; i++) {
      if (isInScale(note + i, scale, root)) {
        return note + i;
      }
    }
  } else if (direction < 0) {
    for (int i = 1; i < 12; i++) {
      if (isInScale(note - i, scale, root)) {
        return note - i;
      }
    }
  } else {
    for (int i = 1; i <= 6; i++) {
      if (isInScale(note - i, scale, root)) {
        return note - i;
      }
      if (isInScale(note + i, scale, root)) {
        return note + i;
      }
    }
  }
  return note;
}

int transposeNote(int note, int chainTransposition, bool scaleAware, int scale, int root, int projectTransposition) {
  int transposed = note + chainTransposition;
  if (scaleAware) {
    transposed = snapToScale(transposed, scale, root, chainTransposition);
  }
  // the project moves everything, after the notes were put into the scale
  return transposed + projectTransposition;
}
