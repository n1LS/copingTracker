/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _ZERO_CROSSING_H_
#define _ZERO_CROSSING_H_

#include <cstdint>
#include <cstdlib>

// Zero crossings of a sample: the waveform draws them, and a slice point can be moved from one to the next.
//
// A zero crossing is between two neighbouring samples that are on different sides of zero, zero itself counts as
// the positive side. -5, 0, 5 is one crossing (between -5 and 0), 5, 0, 5 has none. Of the two samples the one
// closer to zero is the position of the crossing, the later one if both are as close.
namespace ZeroCrossing {

// whether the signal crosses zero between two neighbouring samples
inline bool Crosses(int16_t before, int16_t after) {
  return (before < 0) != (after < 0);
}

// whether the later one of the two samples of a crossing is its position (it is at least as close to zero)
inline bool LaterIsPosition(int16_t before, int16_t after) {
  return std::abs(static_cast<int32_t>(after)) <= std::abs(static_cast<int32_t>(before));
}

// Finds the closest zero crossing after (direction > 0) or before (direction < 0) a sample, in `count` frames of
// `channels` interleaved channels, only the first (left) channel counts. The crossing at the sample itself does not
// count. Returns false if there is none.
inline bool Find(const int16_t *samples, uint32_t count, uint32_t channels, uint32_t from, int direction,
                 uint32_t &found) {
  if (samples == nullptr || count < 2 || channels == 0) {
    return false;
  }

  // The position of the crossing between sample i - 1 and i is i - 1 or i, so it never decreases with i: the first
  // crossing in the direction of the search that is on the right side of `from` is the closest one.
  if (direction > 0) {
    for (uint32_t i = from + 1; i < count; ++i) {
      const int16_t before = samples[(i - 1) * channels];
      const int16_t after = samples[i * channels];
      if (Crosses(before, after)) {
        const uint32_t position = LaterIsPosition(before, after) ? i : i - 1;
        if (position > from) {
          found = position;
          return true;
        }
      }
    }
  } else {
    for (uint32_t i = (from < count ? from : count - 1); i >= 1; --i) {
      const int16_t before = samples[(i - 1) * channels];
      const int16_t after = samples[i * channels];
      if (Crosses(before, after)) {
        const uint32_t position = LaterIsPosition(before, after) ? i : i - 1;
        if (position < from) {
          found = position;
          return true;
        }
      }
    }
  }
  return false;
}

} // namespace ZeroCrossing

#endif
