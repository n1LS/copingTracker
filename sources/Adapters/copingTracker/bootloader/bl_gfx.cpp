/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the PatchBay firmware
 */

#include "bl_gfx.h"
#include "bootloader_font.generated.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

/* Character graphics mode */

#define BUFFER_CHARS 15
static Color screen_bg_color = BLACK;
static Color screen_fg_color = WHITE;
static int cursor_x = 0;
static int cursor_y = 0;
uint8_t screen[TEXT_HEIGHT * TEXT_WIDTH] = {0};
uint8_t colors[TEXT_HEIGHT * TEXT_WIDTH] = {0};
uint8_t changed[TEXT_HEIGHT * TEXT_WIDTH] = {0};
uint16_t buffer[CHAR_HEIGHT * CHAR_WIDTH * BUFFER_CHARS] = {0};

// Default VGA/PC terminal palette, swapped bytes
uint16_t palette[16] = {
    0x0000, // BLACK         (0,   0,   0)
    0x00A8, // RED           (170, 0,   0)
    0x4005, // GREEN         (0,   170, 0)
    0x40AD, // YELLOW        (170, 85,  0)
    0x1500, // BLUE          (0,   0,   170)
    0x15A8, // MAGENTA       (170, 0,   170)
    0x5505, // CYAN          (0,   170, 170)
    0x55AD, // LIGHT_GRAY    (170, 170, 170)
    0xAA52, // DARK_GRAY     (85,  85,  85)
    0x00F8, // LIGHT_RED     (255, 85,  85)
    0xE057, // LIGHT_GREEN   (85,  255, 85)
    0xE0FF, // LIGHT_YELLOW  (255, 255, 85)
    0x9F52, // LIGHT_BLUE    (85,  85,  255)
    0x9FFA, // LIGHT_MAGENTA (255, 85,  255)
    0xFF57, // LIGHT_CYAN    (85,  255, 255)
    0xFFFF  // WHITE         (255, 255, 255)
};

void gfx_clear(Color color) {
  int size = TEXT_HEIGHT * TEXT_WIDTH;
  memset(screen, 0, size);
  memset(colors, color, size);
  gfx_set_cursor(0, 0);
  gfx_draw_screen();
}

void gfx_set_foreground(Color color) {
  screen_fg_color = color;
}

void gfx_set_background(Color color) {
  screen_bg_color = color;
}

void gfx_set_cursor(uint8_t x, uint8_t y) {
  cursor_x = x;
  cursor_y = y;
}

uint8_t gfx_get_cursor_x() {
  return cursor_x;
}

uint8_t gfx_get_cursor_y() {
  return cursor_y;
}

void gfx_putc(char c) {
  if (cursor_x < 0 || cursor_x >= TEXT_WIDTH || cursor_y < 0 || cursor_y >= TEXT_HEIGHT) {
    return;
  }

  int idx = cursor_y * TEXT_WIDTH + cursor_x;
  if (c >= 32) {
    c -= 32;
    uint8_t color = ((screen_fg_color & 0xf) << 4) | (screen_bg_color & 0xf);
    if (screen[idx] != c || colors[idx] != color) {
      screen[idx] = c;
      colors[idx] = color;
      changed[idx] = true;
    }
  }

  if (cursor_x + 1 < TEXT_WIDTH) {
    cursor_x++;
  }
}

static inline void gfx_set_window_for_region(uint16_t screen_x, uint16_t screen_y, uint16_t screen_width,
                                             uint16_t screen_height) {
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
}

static inline void gfx_rasterize_char_column(uint8_t char_col, uint8_t row_start, uint8_t row_count, uint16_t *dst) {
  for (int glyph_bit = CHAR_WIDTH - 1; glyph_bit >= 0; glyph_bit--) {
    uint16_t glyph_mask = 1 << (CHAR_WIDTH - 1 - glyph_bit);

    for (int char_row = row_start + row_count - 1; char_row >= row_start; char_row--) {
      int16_t cell_idx = char_row * TEXT_WIDTH + char_col;
      uint8_t glyph_index = screen[cell_idx];
      uint16_t fg_rgb565 = palette[colors[cell_idx] >> 4];
      uint16_t bg_rgb565 = palette[colors[cell_idx] & 0xf];

      const uint8_t *glyph_rows = (const uint8_t *)(font + char_map[glyph_index]);

      // draw the character into the output buffer
      for (int pixel_row = CHAR_HEIGHT - 1; pixel_row >= 0; pixel_row--) {
        uint16_t glyph_row_bits = glyph_rows[pixel_row];
        *dst++ = (glyph_row_bits & glyph_mask) ? fg_rgb565 : bg_rgb565;
      }
    }
  }
}

void gfx_draw_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  int remaining_rows = height;
  while (remaining_rows) {
    int chunk_rows = (remaining_rows > BUFFER_CHARS) ? BUFFER_CHARS : remaining_rows;
    uint8_t chunk_y = y + height - remaining_rows;
    remaining_rows -= chunk_rows;
    gfx_draw_sub_region(x, chunk_y, width, chunk_rows);
  }
}

// NOTE: we make life easier for ourselves by using the LCD controllers
// orientation command to let us treat the x,y coords passed into this function
// as the visual x & y instead of trying to transform them to the LCDs physical
// x,y coords to compensate for the fact that on the picoTracker the screen is
// mounted rotated 90deg clockwise, ie. the "bottom" of the LCD with the flex
// pcb connector is actually on the left instead of its normal orientation of
// being mounted on the bottom of the LCD
void gfx_fill_rect(uint8_t color_index, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
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

inline void gfx_draw_sub_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  assert(height <= BUFFER_CHARS);

  uint16_t screen_x = x * CHAR_WIDTH;
  uint16_t screen_y = (TEXT_HEIGHT - height - y) * CHAR_HEIGHT;
  uint16_t screen_width = width * CHAR_WIDTH;
  uint16_t screen_height = height * CHAR_HEIGHT;

  gfx_set_window_for_region(screen_x, screen_y, screen_width, screen_height);

  ili9341_start_writing();

  for (int char_col = x; char_col < x + width; char_col++) {
    // create one column of screen information
    uint16_t *buffer_idx = buffer;

    gfx_rasterize_char_column(char_col, y, height, buffer_idx);

    ili9341_write_data_continuous((uint8_t *)buffer, CHAR_WIDTH * screen_height * sizeof(int16_t));
  }
  ili9341_stop_writing();
}

void gfx_draw_changed() {
  for (int idx = 0; idx < TEXT_HEIGHT * TEXT_WIDTH; idx++) {
    if (changed[idx]) {
      changed[idx] = false;
      uint16_t y = idx / TEXT_WIDTH;
      uint16_t x = idx - (TEXT_WIDTH * y);

      // Expand height downward from the seed cell
      int height = 1;
      for (int probe_y = y + 1; probe_y < TEXT_HEIGHT; probe_y++) {
        int probe_idx = probe_y * TEXT_WIDTH + x;
        if (changed[probe_idx]) {
          changed[probe_idx] = false;
          height++;
        } else {
          break;
        }
      }

      // Expand width rightward: each new column must have all `height` rows dirty
      int width = 1;
      for (int probe_x = x + 1; probe_x < TEXT_WIDTH; probe_x++) {
        bool col_ok = true;
        for (int probe_y = y; probe_y < y + height; probe_y++) {
          if (!changed[probe_y * TEXT_WIDTH + probe_x]) {
            col_ok = false;
            break;
          }
        }
        if (!col_ok) {
          break;
        }
        for (int probe_y = y; probe_y < y + height; probe_y++) {
          changed[probe_y * TEXT_WIDTH + probe_x] = false;
        }
        width++;
      }

      gfx_draw_region(x, y, width, height);
    }
  }
}

void gfx_draw_screen() {
  // draw the whole screen
  gfx_draw_region(0, 0, TEXT_WIDTH, TEXT_HEIGHT);
}

void gfx_set_palette_color(int idx, uint16_t rgb565_color) {
  palette[idx] = rgb565_color;
}

void gfx_init() {
  ili9341_init();
}
