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

#include "Foundation/Types/Colors.h"
#include "ili9341.h"

#define TEXT_WIDTH 32
#define TEXT_HEIGHT 24
#define CHAR_HEIGHT 10
#define CHAR_WIDTH 10
#define BUFFER_CHARS 24

void chargfx_init();
void chargfx_clear();
void chargfx_draw_screen();
void chargfx_draw_changed();
void chargfx_draw_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void chargfx_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void chargfx_set_foreground(Color color);
void chargfx_set_background(Color color);
void chargfx_set_cursor(uint8_t x, uint8_t y);
uint8_t chargfx_get_cursor_x();
uint8_t chargfx_get_cursor_y();
void chargfx_putc(char c, bool transparent = false);
void chargfx_set_palette_color(int idx, uint16_t rgb565_color);
void chargfx_set_font_index(uint8_t idx);
uint8_t chargfx_get_font_index();
void chargfx_get_screen_storage(uint8_t **outScreen, uint8_t **outColors, bool **outChanged);
uint16_t *chargfx_get_palette();

#endif