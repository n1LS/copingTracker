/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "chargfx_host.h"
#include "font.generated.h"
#include <algorithm>
#include <cstring>

static Color screen_bg_color = BLACK;
static Color screen_fg_color = WHITE;
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t ui_font_index = 0;

static uint8_t screen[CHARGFX_TEXT_HEIGHT * CHARGFX_TEXT_WIDTH] = {0};
static uint8_t colors[CHARGFX_TEXT_HEIGHT * CHARGFX_TEXT_WIDTH] = {0};
static bool changed[CHARGFX_TEXT_HEIGHT * CHARGFX_TEXT_WIDTH] = {0};
static uint32_t pixel_buffer[CHARGFX_SCREEN_HEIGHT * CHARGFX_SCREEN_WIDTH] = {0};

static uint16_t palette[16] = {0x0000, 0x0080, 0x0004, 0x0084, 0x1000, 0x1080, 0x1004, 0x1084,
                               0x38C6, 0x00F8, 0xE007, 0xE0FF, 0x1F00, 0x1FF8, 0xFF07, 0xFFFF};

static uint32_t RGB565toRGB888(uint16_t rgb565) {
  uint8_t r = (rgb565 >> 11) << 3;
  uint8_t g = ((rgb565 >> 5) & 0x3F) << 2;
  uint8_t b = (rgb565 & 0x1F) << 3;
  return (r << 16) | (g << 8) | b;
}

void chargfx_init() {
  memset(screen, 0, sizeof(screen));
  memset(colors, 0, sizeof(colors));
  memset(changed, true, sizeof(changed));
  memset(pixel_buffer, 0, sizeof(pixel_buffer));
}

void chargfx_clear() {
  int size = CHARGFX_TEXT_WIDTH * CHARGFX_TEXT_HEIGHT;
  memset(screen, 0, size);
  memset(colors, screen_bg_color, size);
  memset(changed, true, sizeof(changed));
  chargfx_set_cursor(0, 0);
}

void chargfx_set_foreground(Color color) {
  screen_fg_color = color;
}

void chargfx_set_background(Color color) {
  screen_bg_color = color;
}

Color chargfx_get_foreground() {
  return screen_fg_color;
}

Color chargfx_get_background() {
  return screen_bg_color;
}

void chargfx_set_cursor(uint8_t x, uint8_t y) {
  cursor_x = std::min((int)x, (int)(CHARGFX_TEXT_WIDTH - 1));
  cursor_y = std::min((int)y, (int)(CHARGFX_TEXT_HEIGHT - 1));
}

uint8_t chargfx_get_cursor_x() {
  return cursor_x;
}

uint8_t chargfx_get_cursor_y() {
  return cursor_y;
}

void chargfx_set_font_index(uint8_t idx) {
  ui_font_index = idx;
}

uint8_t chargfx_get_font_index() {
  return ui_font_index;
}

void chargfx_putc(char c, bool transparent) {
  int idx = cursor_y * CHARGFX_TEXT_WIDTH + cursor_x;
  uint8_t color;

  if (transparent) {
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

void chargfx_set_palette_color(int idx, uint16_t rgb565_color) {
  if (idx >= 0 && idx < 16) {
    palette[idx] = rgb565_color;
  }
}

uint16_t *chargfx_get_palette() {
  return palette;
}

uint32_t *chargfx_get_pixel_buffer() {
  return pixel_buffer;
}

void chargfx_get_screen_storage(uint8_t **outScreen, uint8_t **outColors, bool **outChanged) {
  if (outScreen)
    *outScreen = screen;
  if (outColors)
    *outColors = colors;
  if (outChanged)
    *outChanged = changed;
}

static void RasterizeChar(uint8_t ch, uint8_t fg, uint8_t bg, int screen_x, int screen_y) {
  const font_t *font = fonts[ui_font_index];
  const uint16_t *glyph = (*font)[ch];

  uint32_t fg_color = RGB565toRGB888(palette[fg]);
  uint32_t bg_color = RGB565toRGB888(palette[bg]);

  for (int py = 0; py < CHARGFX_CHAR_HEIGHT; ++py) {
    uint16_t row = glyph[py];

    for (int px = 0; px < CHARGFX_CHAR_WIDTH; ++px) {
      int pixel_x = screen_x + px;
      int pixel_y = screen_y + py;

      if (pixel_x >= 0 && pixel_x < CHARGFX_SCREEN_WIDTH && pixel_y >= 0 && pixel_y < CHARGFX_SCREEN_HEIGHT) {
        uint32_t color = (row & (0x200 >> px)) ? fg_color : bg_color;
        pixel_buffer[(screen_y + CHARGFX_CHAR_WIDTH - px - 1) * CHARGFX_SCREEN_WIDTH + screen_x + py] = color;
      }
    }
  }
}

static uint16_t rgb565_brightness(uint16_t color, uint8_t brightness) {
  static const int8_t sine_brightness[32] = {0, 1,  2,  3,  4,  5,  5,  6,  6,  5,  5,  4,  4,  3,  2,  1,
                                             0, -1, -2, -3, -4, -5, -5, -6, -6, -5, -5, -4, -4, -3, -2, -1};

  const int32_t level = sine_brightness[brightness & 0x1f];

  int32_t red = (color >> 11) & 0x1F;
  int32_t green = (color >> 5) & 0x3F;
  int32_t blue = color & 0x1F;

  red += level;
  green += level * 2;
  blue += level;

  if (red < 0)
    red = 0;
  else if (red > 31)
    red = 31;
  if (green < 0)
    green = 0;
  else if (green > 63)
    green = 63;
  if (blue < 0)
    blue = 0;
  else if (blue > 31)
    blue = 31;

  return (uint16_t)((red << 11) | (green << 5) | blue);
}

static void RasterizeCharWithPulse(uint8_t ch, uint8_t fg, uint8_t bg, int screen_x, int screen_y, int pulse) {
  uint16_t fg_normal = palette[fg];
  uint16_t bg_normal = palette[bg];
  uint16_t fg_pulse = rgb565_brightness(fg_normal, pulse);
  uint16_t bg_pulse = rgb565_brightness(bg_normal, pulse);

  uint32_t fg_rgb = RGB565toRGB888(fg_normal);
  uint32_t bg_rgb = RGB565toRGB888(bg_normal);
  uint32_t fg_pulse_rgb = RGB565toRGB888(fg_pulse);
  uint32_t bg_pulse_rgb = RGB565toRGB888(bg_pulse);

  const font_t *font = fonts[ui_font_index];
  const uint16_t *glyph = (*font)[ch];
  const uint16_t empty_mask[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const uint16_t (*font_mask)[10] = font_masks[ui_font_index];
  const int8_t *font_mask_index = font_mask_indices[ui_font_index];
  int8_t mask_index = font_mask_index[ch];
  const uint16_t *mask = (mask_index == -1) ? empty_mask : font_mask[mask_index];

  for (int py = 0; py < CHARGFX_CHAR_HEIGHT; ++py) {
    uint16_t pixels = glyph[py];
    uint16_t mask_bits = mask[py];

    for (int px = CHARGFX_CHAR_WIDTH - 1; px >= 0; --px) {
      uint16_t bit_mask = 1u << px;
      int pixel_x = screen_x + (CHARGFX_CHAR_WIDTH - 1 - px);
      int pixel_y = screen_y + py;

      if (pixel_x >= 0 && pixel_x < CHARGFX_SCREEN_WIDTH && pixel_y >= 0 && pixel_y < CHARGFX_SCREEN_HEIGHT) {
        uint32_t color;
        if (mask_bits & bit_mask) {
          color = (pixels & bit_mask) ? fg_rgb : bg_rgb;
        } else {
          color = (pixels & bit_mask) ? fg_pulse_rgb : bg_pulse_rgb;
        }
        pixel_buffer[(screen_y + px) * CHARGFX_SCREEN_WIDTH + screen_x + py] = color;
      }
    }
  }
}

void chargfx_draw_screen() {
  for (int y = 0; y < CHARGFX_TEXT_HEIGHT; ++y) {
    for (int x = 0; x < CHARGFX_TEXT_WIDTH; ++x) {
      int idx = y * CHARGFX_TEXT_WIDTH + x;
      uint8_t ch = screen[idx];
      uint8_t color_byte = colors[idx];
      uint8_t fg = (color_byte >> 4) & 0x0F;
      uint8_t bg = color_byte & 0x0F;
      int pixel_x = x * CHARGFX_CHAR_WIDTH;
      int pixel_y = y * CHARGFX_CHAR_HEIGHT;
      RasterizeChar(ch, fg, bg, pixel_x, pixel_y);
    }
  }
  memset(changed, false, sizeof(changed));
}

void chargfx_draw_changed() {
  for (int y = 0; y < CHARGFX_TEXT_HEIGHT; ++y) {
    for (int x = 0; x < CHARGFX_TEXT_WIDTH; ++x) {
      int idx = y * CHARGFX_TEXT_WIDTH + x;
      if (!changed[idx])
        continue;
      uint8_t ch = screen[idx];
      uint8_t color_byte = colors[idx];
      uint8_t fg = (color_byte >> 4) & 0x0F;
      uint8_t bg = color_byte & 0x0F;
      int pixel_x = x * CHARGFX_CHAR_WIDTH;
      int pixel_y = y * CHARGFX_CHAR_HEIGHT;
      RasterizeChar(ch, fg, bg, pixel_x, pixel_y);
    }
  }
  memset(changed, false, sizeof(changed));
}

void chargfx_draw_focus_rect(uint8_t x, uint8_t y, uint8_t width) {
  for (int i = 0; i < width; ++i) {
    int idx = y * CHARGFX_TEXT_WIDTH + (x + i);
    changed[idx] = true;
  }
  static uint8_t pulse = 0;
  pulse++;

  for (int i = 0; i < width; ++i) {
    int idx = y * CHARGFX_TEXT_WIDTH + (x + i);
    uint8_t ch = screen[idx];
    uint8_t color_byte = colors[idx];
    uint8_t fg = (color_byte >> 4) & 0x0F;
    uint8_t bg = color_byte & 0x0F;
    int pixel_x = (x + i) * CHARGFX_CHAR_WIDTH;
    int pixel_y = y * CHARGFX_CHAR_HEIGHT;
    RasterizeCharWithPulse(ch, fg, bg, pixel_x, pixel_y, pulse);
  }
}

void chargfx_draw_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  for (int cy = 0; cy < height; ++cy) {
    for (int cx = 0; cx < width; ++cx) {
      int idx = (y + cy) * CHARGFX_TEXT_WIDTH + (x + cx);
      changed[idx] = true;
    }
  }
  chargfx_draw_changed();
}

void chargfx_draw_highlight_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  chargfx_draw_region(x, y, width, height);
}

void chargfx_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  uint32_t color = RGB565toRGB888(palette[screen_fg_color]);
  uint16_t x_end = std::min((uint16_t)(x + width), (uint16_t)CHARGFX_SCREEN_WIDTH);
  uint16_t y_end = std::min((uint16_t)(y + height), (uint16_t)CHARGFX_SCREEN_HEIGHT);

  for (uint16_t py = y; py < y_end; ++py) {
    for (uint16_t px = x; px < x_end; ++px) {
      pixel_buffer[py * CHARGFX_SCREEN_WIDTH + px] = color;
    }
  }
}
