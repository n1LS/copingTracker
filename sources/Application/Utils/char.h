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

#ifndef _CHAR_H_
#define _CHAR_H_

#include "Foundation/Types/Types.h"
#include <string.h>

extern const char hexChars[16];
extern const char *noteNames[12];

inline void hex2char(const unsigned char c, char *s) {
  char *dest__ = s;
  *dest__++ = hexChars[(c & 0xF0) >> 4];
  *dest__++ = hexChars[(c & 0x0F) >> 0];
  *dest__ = 0;
}

inline void hexshort2char(const uint16_t c, char *s) {
  char *dest__ = s;
  *dest__++ = hexChars[(c & 0xF000) >> 12];
  *dest__++ = hexChars[(c & 0x0F00) >> 8];
  *dest__++ = hexChars[(c & 0x00F0) >> 4];
  *dest__++ = hexChars[(c & 0x000F) >> 0];
  *dest__ = 0;
}

#define c2h__(c) (c < 'A' ? c - '0' : c - 'A' + 10)

inline void char2hex(const char *s, unsigned char *c) {
  const char *src = s;
  unsigned char b1 = (c2h__(src[0])) << 4;
  unsigned char b2 = c2h__(src[1]);
  *c = b1 + b2;
}

inline void note2char(unsigned char d, char *s) {
  int oct = d / 12;
  int note = d % 12;
  strcpy(s, noteNames[note]);
  s[2] = hexChars[oct];
}

inline void oct2visualizer(unsigned char d, char *s) {
  int oct = d / 12 - 2;
  if (oct < 0) {
    s[0] = '-';
    oct = -oct;
  } else {
    s[0] = ' ';
  }
  s[1] = '0' + oct;
  s[2] = '\0'; // sloppy, can we make the array shorter?
  s[3] = '\0'; // sloppy, can we make the array shorter?
}

#endif
