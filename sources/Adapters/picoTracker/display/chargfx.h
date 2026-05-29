/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef _TEXT_CHARGFX_H
#define _TEXT_CHARGFX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ili9341.h"

#define TEXT_WIDTH 32
#define TEXT_HEIGHT 24
#define CHAR_HEIGHT 10
#define CHAR_WIDTH 10
#define BUFFER_CHARS 12

// ARNE-16 palette converted to RGB565 --
// https://lospec.com/palette-list/arne-16
typedef enum {
  CHARGFX_BLACK,
  CHARGFX_RED,
  CHARGFX_GREEN,
  CHARGFX_YELLOW,
  CHARGFX_BLUE,
  CHARGFX_MAGENTA,
  CHARGFX_CYAN,
  CHARGFX_LIGHT_GRAY,
  CHARGFX_DARK_GRAY,
  CHARGFX_LIGHT_RED,
  CHARGFX_LIGHT_GREEN,
  CHARGFX_LIGHT_YELLOW,
  CHARGFX_LIGHT_BLUE,
  CHARGFX_LIGHT_MAGENTA,
  CHARGFX_LIGHT_CYAN,
  CHARGFX_WHITE
} chargfx_color_t;

void chargfx_init();
void chargfx_clear(chargfx_color_t color);
void chargfx_draw_screen();
void chargfx_draw_changed();
void chargfx_draw_changed_simple();
void chargfx_draw_sub_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void chargfx_draw_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void chargfx_fill_rect(uint8_t color_index, uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void chargfx_set_foreground(chargfx_color_t color);
void chargfx_set_background(chargfx_color_t color);
void chargfx_set_cursor(uint8_t x, uint8_t y);
uint8_t chargfx_get_cursor_x();
uint8_t chargfx_get_cursor_y();
void chargfx_putc(char c);
void chargfx_set_palette_color(int idx, uint16_t rgb565_color);
void chargfx_set_font_index(uint8_t idx);

#ifdef __cplusplus
}
#endif
#endif
