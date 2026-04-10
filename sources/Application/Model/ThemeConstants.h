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

#include "Adapters/picoTracker/display/font.h"

// Define default color values to be used across the application
namespace ThemeConstants {
// Color constants
const uint32_t DEFAULT_BACKGROUND = 0x000000;
const uint32_t DEFAULT_FOREGROUND = 0xD8D8D8;
const uint32_t DEFAULT_HICOLOR1 = 0x005080;
const uint32_t DEFAULT_HICOLOR2 = 0x307090;
const uint32_t DEFAULT_CONSOLECOLOR = 0x000000;
const uint32_t DEFAULT_CURSORCOLOR = 0x000000;
const uint32_t DEFAULT_INFOCOLOR = 0x00E470;
const uint32_t DEFAULT_WARNCOLOR = 0xE8EC00;
const uint32_t DEFAULT_ERRORCOLOR = 0xF85050;
const uint32_t DEFAULT_ACCENT = 0xF08400;
const uint32_t DEFAULT_ACCENT_ALT = 0xF8B800;
const uint32_t DEFAULT_EMPHASIS = 0x787878;
// const uint32_t DEFAULT_RESERVED1 = 0x0000FF;
// const uint32_t DEFAULT_RESERVED2 = 0x555555;
// const uint32_t DEFAULT_RESERVED3 = 0x777777;
// const uint32_t DEFAULT_RESERVED4 = 0xFFFF00;

// Font constants
const int DEFAULT_UIFONT = 0x0;
inline const int THEME_FONT_COUNT = FONT_COUNT;
inline const char *THEME_FONT_NAMES[THEME_FONT_COUNT] = {"Regular", "Bold", "Block"};

inline const char *DEFAULT_THEME_NAME = "Default";
} // namespace ThemeConstants

#endif // _THEME_CONSTANTS_H_
