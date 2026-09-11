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

#ifndef _THEME_CONSTANTS_H_
#define _THEME_CONSTANTS_H_

#include "Adapters/copingTracker/display/font.generated.h"

// Define default color values to be used across the application
namespace ThemeConstants {
// Color constants
const uint32_t DEFAULT_COLOR0 = 0x000000;
const uint32_t DEFAULT_COLOR1 = 0xBB3B2A;
const uint32_t DEFAULT_COLOR2 = 0x25BC24;
const uint32_t DEFAULT_COLOR3 = 0xC88200;
const uint32_t DEFAULT_COLOR4 = 0x003259;
const uint32_t DEFAULT_COLOR5 = 0xBF4182;
const uint32_t DEFAULT_COLOR6 = 0x2DB1BE;
const uint32_t DEFAULT_COLOR7 = 0x808080;
const uint32_t DEFAULT_COLOR8 = 0x303030;
const uint32_t DEFAULT_COLOR9 = 0xFC391F;
const uint32_t DEFAULT_COLOR10 = 0x31E71F;
const uint32_t DEFAULT_COLOR11 = 0xFFC023;
const uint32_t DEFAULT_COLOR12 = 0x40649E;
const uint32_t DEFAULT_COLOR13 = 0xFF64B4;
const uint32_t DEFAULT_COLOR14 = 0x14F0F0;
const uint32_t DEFAULT_COLOR15 = 0xDEDEDE;

// Font constants
const int DEFAULT_UIFONT = 0x0;
inline const int THEME_FONT_COUNT = FONT_COUNT;
inline const char *THEME_FONT_NAMES[THEME_FONT_COUNT] = {"Regular", "Bold", "Block"};
inline const char *DEFAULT_THEME_NAME = "Default";
} // namespace ThemeConstants

#endif // _THEME_CONSTANTS_H_
