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

#include "Adapters/copingTracker/display/font.h"

// Define default color values to be used across the application
namespace ThemeConstants {
// Color constants
const uint32_t DEFAULT_COLOR0 = 0x000000;
const uint32_t DEFAULT_COLOR1 = 0x800000;
const uint32_t DEFAULT_COLOR2 = 0x008000;
const uint32_t DEFAULT_COLOR3 = 0x808000;
const uint32_t DEFAULT_COLOR4 = 0x000080;
const uint32_t DEFAULT_COLOR5 = 0x800080;
const uint32_t DEFAULT_COLOR6 = 0x008080;
const uint32_t DEFAULT_COLOR7 = 0x808080;
const uint32_t DEFAULT_COLOR8 = 0xc6c6c6;
const uint32_t DEFAULT_COLOR9 = 0xFF0000;
const uint32_t DEFAULT_COLOR10 = 0x00FF00;
const uint32_t DEFAULT_COLOR11 = 0xFFFF00;
const uint32_t DEFAULT_COLOR12 = 0x0000FF;
const uint32_t DEFAULT_COLOR13 = 0xFF00FF;
const uint32_t DEFAULT_COLOR14 = 0x00FFFF;
const uint32_t DEFAULT_COLOR15 = 0xFFFFFF;

// Font constants
const int DEFAULT_UIFONT = 0x0;
inline const int THEME_FONT_COUNT = FONT_COUNT;
inline const char *THEME_FONT_NAMES[THEME_FONT_COUNT] = {"Regular", "Bold", "Block"};
inline const char *DEFAULT_THEME_NAME = "Default";
} // namespace ThemeConstants

#endif // _THEME_CONSTANTS_H_
