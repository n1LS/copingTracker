/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include <array>
#include <cstdint>

#include "Foundation/Types/Colors.h"

#ifndef _TINT_CHAR_H
#define _TINT_CHAR_H

#pragma pack(1)
struct TintColor {
  uint8_t fg:4;
  uint8_t bg:4;
};
#pragma pack(0)

struct TintChar {
  TintColor fgBg;
  uint8_t character;
};

consteval uint8_t hex_uint4(char c, uint8_t last) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  
  // unknown char? return input value
  return last;
}

/******************************************************************************
 * compile time generator function that create etl::array<TintChar> 
 * 
 * row 1 is fg colors (chars as hex)
 * row 2 is bg colors (chars as hex)
 * row 3 is the text itself
 * 
 * "f       8   f     "
 * "0       1   0     "
 * "This is GRAY. Yay."
 ******************************************************************************/

template <size_t N1, size_t N2, size_t N3>
constexpr auto makeTintString(const char (&fg)[N1], const char (&bg)[N2], const char (&ch)[N3]) {
    static_assert(N1 == N2);
    static_assert(N1 == N3);

    etl::array<TintChar, N1> out{};

    // default colors
    uint8_t currentFg = LIGHT_GRAY;
    uint8_t currentBg = BLACK;

    for (size_t i = 0; i < N1 - 1; i++) {
        currentFg = hex_uint4(fg[i], currentFg);
        currentBg = hex_uint4(bg[i], currentBg);

        out[i] = {
            TintColor(currentFg, currentBg),
            static_cast<uint8_t>(ch[i])
        };
    }

    out[N1 - 1] = { TintColor(0, 0), 0 };

    return out;
}

#endif /* _TINT_CHAR_H */
