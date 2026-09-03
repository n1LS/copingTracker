/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include "Foundation/Types/Colors.h"
#include <cstdint>

#define CHARGFX_TEXT_WIDTH 32
#define CHARGFX_TEXT_HEIGHT 24
#define CHARGFX_CHAR_WIDTH 10
#define CHARGFX_CHAR_HEIGHT 10
#define CHARGFX_SCREEN_WIDTH (CHARGFX_TEXT_WIDTH * CHARGFX_CHAR_WIDTH)
#define CHARGFX_SCREEN_HEIGHT (CHARGFX_TEXT_HEIGHT * CHARGFX_CHAR_HEIGHT)

void chargfx_init();
void chargfx_clear();
void chargfx_set_foreground(Color color);
void chargfx_set_background(Color color);
Color chargfx_get_foreground();
Color chargfx_get_background();
void chargfx_set_cursor(uint8_t x, uint8_t y);
uint8_t chargfx_get_cursor_x();
uint8_t chargfx_get_cursor_y();
void chargfx_putc(char c, bool transparent = false);
void chargfx_set_palette_color(int idx, uint16_t rgb565_color);
void chargfx_set_font_index(uint8_t idx);
uint8_t chargfx_get_font_index();
void chargfx_get_screen_storage(uint8_t **outScreen, uint8_t **outColors, bool **outChanged);
uint16_t *chargfx_get_palette();
uint32_t *chargfx_get_pixel_buffer();
void chargfx_draw_screen();
void chargfx_draw_changed();
void chargfx_draw_focus_rect(uint8_t x, uint8_t y, uint8_t width);
void chargfx_draw_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void chargfx_draw_highlight_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void chargfx_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
