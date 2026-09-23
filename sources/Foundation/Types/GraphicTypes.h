/*
* SPDX-License-Identifier: BSD-3-Clause
*
* Copyright (c) 2026 nILS Podewski
*
* This file is part of the copingTracker firmware
*/

#ifndef _GRAPHIC_TYPES_H
#define _GRAPHIC_TYPES_H

#include <stdint.h>

enum Font: uint8_t {
  fRegular,
  fBold,
  fGraphic
};

typedef union {
  uint16_t word;
  struct {
    uint8_t character;
    Font font;
  };
} ScreenCharacter;

#endif