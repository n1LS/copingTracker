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

#include "Application/Instruments/InstrumentBank.h"
#include "Foundation/Types/Types.h"
#include "System/Console/nanoprintf.h"
#include <string.h>

extern const char hexChars[16];
extern const char *noteNames[12];

static inline unsigned char hexNibble(char c) {
  unsigned char v = c - '0';
  if (v < 10)
    return v;

  v = (c | 0x20) - 'a'; // lowercase normalize trick
  return (v < 6) ? (v + 10) : 0;
}

static inline char hexChar(unsigned char v) {
  return hexChars[v & 0x0F];
}

inline void noteToString(unsigned char d, char (&out)[4]) {
  int oct = d / 12;
  int note = d % 12;
  strcpy(out, noteNames[note]);
  out[2] = hexChars[oct];
  out[3] = 0;
}

inline uint8_t hexStringToByte(char *s) {
  return (hexNibble(s[0]) << 4) + hexNibble(s[1]);
}

static inline void byteToHexString(uint8_t c, char *s) {
  s[0] = hexChar(c >> 4);
  s[1] = hexChar(c);
  s[2] = 0;
}
static inline void wordToHexString(uint16_t c, char *s) {
  s[0] = hexChar(c >> 12);
  s[1] = hexChar(c >> 8);
  s[2] = hexChar(c >> 4);
  s[3] = hexChar(c);
  s[4] = 0;
}

#endif
