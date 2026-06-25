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
#include "gpio.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "ili9341.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "System/Profiler/Profiler.h"

/* Character graphics mode */

#define SWAP_BYTES(color) ((uint16_t)(color >> 8) | (uint16_t)(color << 8))

static Color screen_bg_color = BLACK;
static Color screen_fg_color = WHITE;

static int cursor_x = 0;
static int cursor_y = 0;

static uint8_t screen[TEXT_HEIGHT * TEXT_WIDTH] = {0};
static uint8_t colors[TEXT_HEIGHT * TEXT_WIDTH] = {0};
static uint16_t buffer1[CHAR_HEIGHT * CHAR_WIDTH * BUFFER_CHARS] = {0};
static uint16_t buffer2[CHAR_HEIGHT * CHAR_WIDTH * BUFFER_CHARS] = {0};

uint16_t *buffer = buffer1;
uint16_t *buffer_dma = buffer2;

static uint8_t ui_font_index = 0;

// Using a bit array in order to save memory, there is a slight performance
// hit in doing so vs a bool array
static bool changed[TEXT_HEIGHT * TEXT_WIDTH] = {0};

// Default palette, can be redefined
static uint16_t palette[16] = {
    0x0000, // 0: black       (#000000)
    0x0080, // 1: dark red    (#800000)
    0x0004, // 2: dark green  (#008000)
    0x0084, // 3: dark yellow (#808000)
    0x1000, // 4: dark blue   (#000080)
    0x1080, // 5: dark magenta(#800080)
    0x1004, // 6: dark cyan   (#008080)
    0x1084, // 7: gray        (#808080)
    0x38C6, // 8: light gray  (#C6C6C6)
    0x00F8, // 9: red         (#FF0000)
    0xE007, // 10: green      (#00FF00)
    0xE0FF, // 11: yellow     (#FFFF00)
    0x1F00, // 12: blue       (#0000FF)
    0x1FF8, // 13: magenta    (#FF00FF)
    0xFF07, // 14: cyan       (#00FFFF)
    0xFFFF  // 15: white      (#FFFFFF)
};

uint16_t *chargfx_get_palette() {
  return palette;
}

void chargfx_clear() {
  int size = TEXT_WIDTH * TEXT_HEIGHT;
  memset(screen, 0, size);
  memset(colors, screen_bg_color, size);
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

// NOTE: we make life easier for ourselves by using the LCD controllers
// orientation command to let us treat the x,y coords passed into this function
// as the visual x & y instead of trying to transform them to the LCDs physical
// x,y coords to compensate for the fact that on the picoTracker the screen is
// mounted rotated 90deg clockwise, ie. the "bottom" of the LCD with the flex
// pcb connector is actually on the left instead of its normal orientation of
// being mounted on the bottom of the LCD
void chargfx_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  // Get the RGB565 color from the current foreground palette index
  uint16_t color = palette[screen_fg_color];

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
  ili9341_transmit32(ILI9341_CASET, display_x, display_x + display_w - 1);
  ili9341_transmit32(ILI9341_PASET, display_y, display_y + display_h - 1);

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

inline void chargfx_draw_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  assert(height <= BUFFER_CHARS);

  uint16_t screen_x = x * CHAR_WIDTH;
  uint16_t screen_y = (TEXT_HEIGHT - height - y) * CHAR_HEIGHT;
  uint16_t screen_width = width * CHAR_WIDTH;
  uint16_t screen_height = height * CHAR_HEIGHT;

  ili9341_transmit32(ILI9341_CASET, screen_y, screen_y + screen_height - 1);
  ili9341_transmit32(ILI9341_PASET, screen_x, screen_x + screen_width - 1);

  ili9341_set_command(ILI9341_RAMWR);
  ili9341_start_writing();

  const font_t *font = fonts[ui_font_index];

  bool haveDmaInFlight = false;

  for (int page = x; page < x + width; page++) {
    uint16_t *buffer_idx = buffer;

    for (int col = y + height - 1; col >= y; col--) {
      int idx = col * TEXT_WIDTH + page;

      uint8_t character = screen[idx];
      uint16_t fg = palette[colors[idx] >> 4];
      uint16_t bg = palette[colors[idx] & 0x0f];

      const uint16_t *glyph = (*font)[character];

      for (int glyphY = 0; glyphY < CHAR_HEIGHT; glyphY++) {
        uint16_t pix = glyph[glyphY];

        for (int glyphX = CHAR_WIDTH - 1; glyphX >= 0; glyphX--) {
          uint16_t mask = 1u << glyphX;
          *buffer_idx++ = (pix & mask) ? bg : fg;
        }
      }
    }

    if (haveDmaInFlight) {
      while (dma_channel_is_busy(DISPLAY_DMA_CH)) {
      }
      while (spi_is_busy(DISPLAY_SPI)) {
      }
    }

    dma_channel_set_read_addr(DISPLAY_DMA_CH, buffer, false);

    dma_channel_set_trans_count(DISPLAY_DMA_CH, CHAR_WIDTH * screen_height * sizeof(uint16_t), true);

    haveDmaInFlight = true;

    uint16_t *tmp = buffer;
    buffer = buffer_dma;
    buffer_dma = tmp;
  }

  if (haveDmaInFlight) {
    while (dma_channel_is_busy(DISPLAY_DMA_CH)) {
    }
    while (spi_is_busy(DISPLAY_SPI)) {
    }
  }

  ili9341_stop_writing();
}

void chargfx_draw_changed() {
  PROFILE_FUNCTION();

  for (uint8_t y = 0; y < TEXT_HEIGHT; y++) {
    int start = -1;

    for (uint8_t x = 0; x < TEXT_WIDTH; x++) {
      int idx = y * TEXT_WIDTH + x;

      if (changed[idx]) {
        if (start < 0) {
          start = x;
        }

        changed[idx] = false;
      } else if (start >= 0) {
        chargfx_draw_region(start, y, x - start, 1);

        start = -1;
      }
    }

    if (start >= 0) {
      chargfx_draw_region(start, y, TEXT_WIDTH - start, 1);
    }
  }
}

void chargfx_draw_screen() {
  // draw the whole screen
  for (int y = 0; y < TEXT_HEIGHT; y++) {
    chargfx_draw_region(0, y, TEXT_WIDTH, 1);
  }
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
