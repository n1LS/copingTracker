/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "chargfx.h"
#include "font.h"
#include "hardware/spi.h"
#include "ili9341.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

/* Character graphics mode */

#define SWAP_BYTES(color) ((uint16_t)(color >> 8) | (uint16_t)(color << 8))

static Color screen_bg_color = BLACK;
static Color screen_fg_color = WHITE;

static int cursor_x = 0;
static int cursor_y = 0;

static uint8_t screen[TEXT_HEIGHT * TEXT_WIDTH] = {0};
static uint8_t colors[TEXT_HEIGHT * TEXT_WIDTH] = {0};
static uint16_t buffer[CHAR_HEIGHT * CHAR_WIDTH * BUFFER_CHARS] = {0};

static uint8_t ui_font_index = 0;

// Using a bit array in order to save memory, there is a slight performance
// hit in doing so vs a bool array
static bool changed[TEXT_HEIGHT * TEXT_WIDTH ] = {0};

// Default palette, can be redefined
static uint16_t palette[16] = {};

uint16_t *chargfx_get_palette() {
  return palette;
}

void chargfx_clear(Color color) {
  int size = TEXT_WIDTH * TEXT_HEIGHT;
  memset(screen, 0, size);
  memset(colors, color, size);
  chargfx_set_cursor(0, 0);
  chargfx_draw_screen();
}

void chargfx_set_foreground(Color color) {
  screen_fg_color = color;
}

void chargfx_set_background(Color color) {
  screen_bg_color = color;
}

void chargfx_set_cursor(uint8_t x, uint8_t y) {
  cursor_x = x;
  cursor_y = y;
}

void chargfx_set_font_index(uint8_t idx) {
  ui_font_index = idx;
}

uint8_t chargfx_get_font_index() {
  return ui_font_index;
}

uint8_t chargfx_get_cursor_x() {
  return cursor_x;
}

uint8_t chargfx_get_cursor_y() {
  return cursor_y;
}

void chargfx_putc(char c, bool transparent) {
  int idx = cursor_y * TEXT_WIDTH + cursor_x;
  // todo: use a color_t
  uint8_t color;

  if (transparent) {
    // use already printed bg color instead of the currently set color
    color = (screen_fg_color << 4) | (colors[idx] & 0x0f);
  } else {
    color = (screen_fg_color << 4) | screen_bg_color;
  }

  if (screen[idx] != c || colors[idx] != color) {
    screen[idx] = c;
    colors[idx] = color;
    changed[idx] = true;
  }
}

void chargfx_draw_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  int remainder = height;
  while (remainder) {
    int sub_height = (remainder > BUFFER_CHARS) ? BUFFER_CHARS : remainder;
    int sub_y = y + height - remainder;
    remainder -= sub_height;
    chargfx_draw_sub_region(x, sub_y, width, sub_height);
  }
}

// NOTE: we make life easier for ourselves by using the LCD controllers
// orientation command to let us treat the x,y coords passed into this function
// as the visual x & y instead of trying to transform them to the LCDs physical
// x,y coords to compensate for the fact that on the picoTracker the screen is
// mounted rotated 90deg clockwise, ie. the "bottom" of the LCD with the flex
// pcb connector is actually on the left instead of its normal orientation of
// being mounted on the bottom of the LCD
void chargfx_fill_rect(uint8_t color_index, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  // Get the RGB565 color from the current foreground palette index
  uint16_t color = palette[color_index];

  // Clip the rectangle to the screen dimensions
  if (x >= ILI9341_TFTHEIGHT || y >= ILI9341_TFTWIDTH) {
    return;
  }
  if (x + width > ILI9341_TFTHEIGHT) {
    width = ILI9341_TFTHEIGHT - x;
  }
  if (y + height > ILI9341_TFTWIDTH) {
    height = ILI9341_TFTWIDTH - y;
  }

  // display_x is from right hand edge and since the picoTracker LCD is mounted
  // rotated 90deg clockwise, the LCDs "physical height" is actually visually
  // speaking the width
  uint16_t display_x = ILI9341_TFTHEIGHT - x - width;
#ifdef LCD_ST7789
  uint16_t display_y = y;
#else
  uint16_t display_y = ILI9341_TFTWIDTH - y - height;
#endif
  uint16_t display_w = width;
  uint16_t display_h = height;

  // Set rotation for rectangle drawing
  ili9341_set_command(ILI9341_MADCTL);
  ili9341_command_param(0x28); // 90-degree clockwise rotation

  // Set display window
  ili9341_set_command(ILI9341_CASET);
  ili9341_command_param16(display_x);
  ili9341_command_param16(display_x + display_w - 1);

  ili9341_set_command(ILI9341_PASET);
  ili9341_command_param16(display_y);
  ili9341_command_param16(display_y + display_h - 1);

  ili9341_set_command(ILI9341_RAMWR);
  ili9341_start_writing();

  // just use the char cell buffer for our line buffer as its more than big
  // enough
  for (uint16_t i = 0; i < display_w; i++) {
    buffer[i] = color;
  }

  // Write the buffer for each column
  for (uint16_t i = 0; i < display_h; i++) {
    ili9341_write_data_continuous((uint8_t *)buffer, display_w * sizeof(uint16_t));
  }

  ili9341_stop_writing();

  // Restore original rotation
  ili9341_set_command(ILI9341_MADCTL);
  ili9341_command_param(LCD_MADCTL_DEFAULT);
}

inline void chargfx_draw_sub_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  assert(height <= BUFFER_CHARS);

  uint16_t screen_x = x * CHAR_WIDTH;
  uint16_t screen_y = (TEXT_HEIGHT - height - y) * CHAR_HEIGHT;
  uint16_t screen_width = width * CHAR_WIDTH;
  uint16_t screen_height = height * CHAR_HEIGHT;

  // column address set
  ili9341_set_command(ILI9341_CASET);
  ili9341_command_param16(screen_y);
  ili9341_command_param16(screen_y + screen_height - 1);

  // page address set
  ili9341_set_command(ILI9341_PASET);
  ili9341_command_param16(screen_x);
  ili9341_command_param16(screen_x + screen_width - 1);
  // start writing
  ili9341_set_command(ILI9341_RAMWR);

  ili9341_start_writing();

  const font_t *font = fonts[ui_font_index];

  for (int page = x; page < x + width; page++) {
    // create one column of screen information
    uint16_t *buffer_idx = buffer;

    for (int bit = CHAR_WIDTH - 1; bit >= 0; bit--) {
      uint16_t mask = 1 << (CHAR_WIDTH - 1 - bit);
      for (int col = y + height - 1; col >= y; col--) {
        int16_t idx = col * TEXT_WIDTH + page;
        uint8_t character = screen[idx];
        uint16_t fg_color = palette[colors[idx] >> 4];
        uint16_t bg_color = palette[colors[idx] & 0xf];

        const uint16_t *pixel_data = (*font)[character];

        // draw the character into the buffer
        for (int j = CHAR_HEIGHT - 1; j >= 0; j--) {
          uint16_t pix = pixel_data[j];
          *buffer_idx++ = (pix & mask) ? fg_color : bg_color;
        }
      }
    }
    ili9341_write_data_continuous((uint8_t *)buffer, CHAR_WIDTH * screen_height * sizeof(int16_t));
  }
  ili9341_stop_writing();
}

void chargfx_draw_changed() {
  for (int idx = 0; idx < TEXT_HEIGHT * TEXT_WIDTH; idx++) {
    if (changed[idx]) {
      changed[idx] = false;
      // check adjacent in order to find bigger rectangle
      uint16_t y = idx / TEXT_WIDTH;
      uint16_t x = idx - (TEXT_WIDTH * y);

      int height = 1;
      // first pass tests the height
      for (int probe_y = y + 1; probe_y < TEXT_HEIGHT; probe_y++) {
        int probe_idx = probe_y * TEXT_WIDTH + x;
        if (changed[probe_idx]) {
          changed[probe_idx] = false;
          height++;
          continue;
        }
        break;
      }

      int16_t width = 1;
      // having the height, we can test every subsequent column
      for (int probe_x = x + 1; probe_x < TEXT_WIDTH; probe_x++) {
        for (int probe_y = y; probe_y < y + height; probe_y++) {
          // if we don't get to max height, then abort
          int probe_idx = probe_y * TEXT_WIDTH + probe_x;
          if (!changed[probe_idx]) {
            // undo last column
            for (int undo_y = y; undo_y < probe_y; undo_y++) {
              changed[undo_y * TEXT_WIDTH + probe_x] = true;
            }
            goto end;
          }
          changed[probe_idx] = false;
        }
        width++;
      }
    end:
      chargfx_draw_region(x, y, width, height);
    }
  }
}

void chargfx_draw_screen() {
  // draw the whole screen
  chargfx_draw_region(0, 0, TEXT_WIDTH, TEXT_HEIGHT);
}

void chargfx_set_palette_color(int idx, uint16_t rgb565_color) {
  palette[idx] = SWAP_BYTES(rgb565_color);
}

void chargfx_init() {
  ili9341_init();
}

void chargfx_get_screen_storage(uint8_t **outScreen, uint8_t **outColors, bool **outChanged) {
  *outScreen = screen;
  *outColors = colors;
  *outChanged = changed;
}
