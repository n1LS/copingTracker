/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker Boot Manager
 */

#include "bootloader_menu.h"
#include "bootloader_gfx.h"
#include "path_utils.h"
#include <cstdint>
#include <cstring>

int menu_show_firmware_selection(void) {
  return 0;
}

static void render_text(uint8_t x, uint8_t y, const char *s) {
  gfx_set_cursor(x, y);
  while (*s && x < TEXT_WIDTH) {
    x++;
    gfx_putc(*s++);
  }
}

// Render `s` starting at (x, y), then pad with spaces out to (x + width) so
// any previous longer content at that row is overwritten.
static void render_text_padded(uint8_t x, uint8_t y, const char *s, uint8_t width) {
  uint8_t col = x;
  const uint8_t end = (x + width < TEXT_WIDTH) ? (x + width) : TEXT_WIDTH;
  gfx_set_cursor(col, y);
  while (*s && col < end) {
    col++;
    gfx_putc(*s++);
  }
  while (col < end) {
    col++;
    gfx_putc(' ');
  }
}

// GRUB-style single-line box with the section label embedded in the top
// border. Cols 0 and TEXT_WIDTH-1 of every row between top_y..bot_y hold the
// vertical sides; the dynamic list renderer must keep cols 1..TEXT_WIDTH-2
// to itself so it doesn't paint over the borders.
static void draw_list_box(uint8_t top_y, uint8_t bot_y) {
  // Top border: ┌───...──┐
  gfx_set_foreground(LIGHT_GRAY);
  gfx_set_cursor(1, top_y);
  gfx_putc(GLYPH(char_border_single_topLeft_s));

  uint8_t col = 2;
  gfx_set_foreground(LIGHT_GRAY);
  while (col < TEXT_WIDTH - 2) {
    gfx_set_cursor(col++, top_y);
    gfx_putc(GLYPH(char_border_single_horizontal_s));
  }
  gfx_set_cursor(TEXT_WIDTH - 2, top_y);
  gfx_putc(GLYPH(char_border_single_topRight_s));

  // Vertical sides on every row between top and bottom.
  for (uint8_t y = top_y + 1; y < bot_y; ++y) {
    gfx_set_cursor(1, y);
    gfx_putc(GLYPH(char_border_single_vertical_s));
    gfx_set_cursor(TEXT_WIDTH - 2, y);
    gfx_putc(GLYPH(char_border_single_vertical_s));
  }

  // Bottom border.
  gfx_set_cursor(1, bot_y);
  gfx_putc(GLYPH(char_border_single_bottomLeft_s));
  for (uint8_t x = 2; x < TEXT_WIDTH - 2; ++x) {
    gfx_set_cursor(x, bot_y);
    gfx_putc(GLYPH(char_border_single_horizontal_s));
  }
  gfx_set_cursor(TEXT_WIDTH - 2, bot_y);
  gfx_putc(GLYPH(char_border_single_bottomRight_s));
}

void menu_render_static(void) {
  gfx_clear(BLACK);
  gfx_set_background(BLUE);
  gfx_set_foreground(WHITE);

  // Title bar — full-width inverted.
  char title[TEXT_WIDTH + 1];
  std::memset(title, ' ', TEXT_WIDTH);
  title[TEXT_WIDTH] = 0;
  const char *t = "pBM - copingTracker Boot Manager";
  std::memcpy(title + 1, t, std::strlen(t));
  render_text(0, 0, title);

  // Static labels.
  gfx_set_background(BLACK);
  gfx_set_foreground(LIGHT_GRAY);
  render_text(1, 2, "Installed:");

  // box around the firmware list. Box spans rows 4..23; the
  // dynamic list lives on rows 5..22 inside it.
  draw_list_box(4, 23);

  // Key legend at the bottom
  gfx_set_foreground(LIGHT_GRAY);
  // max length      "KEYNAME " |                             "                             "
  render_text(1, 25, "  ENTER " char_border_single_vertical_s " flash & boot selection      ");
  render_text(1, 26, "   PLAY " char_border_single_vertical_s " boot installed firmware     ");
  render_text(1, 27, "DOWN,UP " char_border_single_vertical_s " change selection            ");
  render_text(1, 28, "   EDIT " char_border_single_vertical_s " run bootloader & upload .uf2");

  gfx_draw_changed();
}

void menu_render_main(const Uf2FileEntry *uf2_files, int uf2_count, int selected_index, const char *installed_bin,
                      bool sd_ready, int auto_boot_timeout) {
  // Installed firmware name (row 3). Strip leading '/' and trailing
  // .bin extension for display.
  char fw_buf[TEXT_WIDTH];
  const char *fw_src = (installed_bin && installed_bin[0]) ? bl_path_basename(installed_bin) : "(none)";
  bl_copy_str(fw_buf, sizeof(fw_buf), fw_src);
  (void)bl_strip_extension_ci(fw_buf, ".bin");
  gfx_set_foreground(WHITE);
  render_text_padded(12, 2, fw_buf, TEXT_WIDTH - 4);

  // Firmware list rows 5..19 (14 rows). Every row is repainted (with
  // padding) so transitions between empty/non-empty and between different
  // file counts clean themselves up.
  const int kListRow0 = 6;
  const int kRowsAvail = 16;

  // List content is bounded by the box drawn in menu_render_static: cols 0
  // and TEXT_WIDTH-1 hold the vertical borders, so dynamic content can only
  // touch cols 1..TEXT_WIDTH-2 (width = TEXT_WIDTH - 2).
  const uint8_t kBoxInnerX = 3;
  const uint8_t kBoxInnerWidth = TEXT_WIDTH - 6;

  if (!sd_ready || uf2_count == 0) {
    menu_show_message_box("No firmware files found.", "Add .uf2 to SD card root, then reboot.");
  } else {
    const int shown = uf2_count < kRowsAvail ? uf2_count : kRowsAvail;

    for (int i = 0; i < kRowsAvail; ++i) {
      const bool sel = (i == selected_index);

      if (sel) {
        gfx_set_background(BLUE);
        gfx_set_foreground(WHITE);
      } else {
        gfx_set_background(BLACK);
        gfx_set_foreground(WHITE);
      }

      const uint8_t y = static_cast<uint8_t>(kListRow0 + i);
      if (i >= shown) {
        break;
      }

      // Show the firmware's display name: strip a leading directory and
      // any trailing .bin/.uf2 extension.
      const char *name = bl_path_basename(uf2_files[i].path);
      char name_buf[64];
      bl_copy_str(name_buf, sizeof(name_buf), name);
      if (!bl_strip_extension_ci(name_buf, ".bin")) {
        (void)bl_strip_extension_ci(name_buf, ".uf2");
      }

      // Prefix the installed entry with "* "; align all others with "  ".
      // Total row: 2-char prefix + up to (kBoxInnerWidth-2) chars of name.
      const bool is_installed = installed_bin && installed_bin[0] && bl_str_equals_ci(uf2_files[i].path, installed_bin);
      char row_buf[40];
      row_buf[0] = is_installed ? '*' : ' ';
      row_buf[1] = ' ';
      bl_copy_str(row_buf + 2, sizeof(row_buf) - 2, name_buf);

      render_text_padded(3, y, row_buf, static_cast<uint8_t>(kBoxInnerWidth));
    }
  }

  // Auto-boot status row. Keep this on the display; menu_show_message()
  // currently writes to the serial console only.
  gfx_set_background(BLACK);
  gfx_set_foreground(YELLOW);

  if (auto_boot_timeout > 0) {
    char text[35] = "Auto-Boot in Xs. Any key to abort.";
    text[13] = '0' + (auto_boot_timeout / 1000);
    menu_show_message(text);
  }

  gfx_draw_changed();
}

void menu_show_message_box(const char *line1, const char *line2, Color color) {
  // 4-row yellow info box: same layout as menu_show_sd_warning but YELLOW (informational, not error).
  const uint8_t y_top = 13;
  const uint8_t y_line1 = y_top + 1;
  const uint8_t y_line2 = y_line1 + 1;
  const uint8_t y_bot = y_line2 + 1;

  gfx_set_background(BLACK);
  gfx_set_foreground(color);

  gfx_set_cursor(1, y_top);
  gfx_putc(GLYPH(char_border_single_topLeft_s));
  for (uint8_t x = 2; x < TEXT_WIDTH - 2; ++x) {
    gfx_set_cursor(x, y_top);
    gfx_putc(GLYPH(char_border_single_horizontal_s));
  }
  gfx_set_cursor(TEXT_WIDTH - 2, y_top);
  gfx_putc(GLYPH(char_border_single_topRight_s));

  auto render_row = [](uint8_t y, const char *text) {
    gfx_set_cursor(1, y);
    gfx_putc(GLYPH(char_border_single_vertical_s));
    uint8_t col = 2;
    const uint8_t end = TEXT_WIDTH - 2;
    gfx_set_cursor(col, y);
    auto put = [&](char c) {
      if (col < end) {
        col++;
        gfx_putc(c);
      }
    };
    put(' ');
    while (*text) {
      put(*text++);
    }
    while (col < end) {
      put(' ');
    }
    gfx_set_cursor(TEXT_WIDTH - 2, y);
    gfx_putc(GLYPH(char_border_single_vertical_s));
  };

  render_row(y_line1, line1);
  render_row(y_line2, line2);

  gfx_set_cursor(1, y_bot);
  gfx_putc(GLYPH(char_border_single_bottomLeft_s));
  for (uint8_t x = 2; x < TEXT_WIDTH - 2; ++x) {
    gfx_set_cursor(x, y_bot);
    gfx_putc(GLYPH(char_border_single_horizontal_s));
  }
  gfx_set_cursor(TEXT_WIDTH - 2, y_bot);
  gfx_putc(GLYPH(char_border_single_bottomRight_s));

  gfx_draw_changed();
}

void menu_show_sd_warning() {
  menu_show_message_box("SD card cannot be read.", "Insert a card & press any button to reboot", LIGHT_RED);
}

void menu_show_message(const char *message, const char *message2, Color color) {
  // Single-line modal box spanning the full width, vertically centered in
  // the list area. The next menu_render_main() will fully overwrite it.
  const uint8_t y_top = 14;
  const uint8_t y_mid = 15;
  const uint8_t y_bot = 16;

  gfx_set_foreground(color);

  // Top border.
  gfx_set_cursor(1, y_top);
  gfx_putc(GLYPH(char_border_single_topLeft_s));
  for (uint8_t x = 2; x < TEXT_WIDTH - 2; ++x) {
    gfx_set_cursor(x, y_top);
    gfx_putc(GLYPH(char_border_single_horizontal_s));
  }
  gfx_set_cursor(TEXT_WIDTH - 2, y_top);
  gfx_putc(GLYPH(char_border_single_topRight_s));

  gfx_set_cursor(1, y_mid);
  gfx_putc(GLYPH(char_border_single_vertical_s));

  uint8_t col = 2;
  const uint8_t end = TEXT_WIDTH - 2;
  gfx_set_cursor(col, y_mid);
  auto write_char = [&](char c) {
    if (col < end) {
      col++;
      gfx_putc(c);
    }
  };
  write_char(' ');
  while (*message)
    write_char(*message++);
  if (message2) {
    write_char(' ');
    while (*message2)
      write_char(*message2++);
  }
  while (col < end)
    write_char(' ');

  gfx_set_cursor(TEXT_WIDTH - 2, y_mid);
  gfx_putc(GLYPH(char_border_single_vertical_s));

  // Bottom border.
  gfx_set_cursor(1, y_bot);
  gfx_putc(GLYPH(char_border_single_bottomLeft_s));
  for (uint8_t x = 2; x < TEXT_WIDTH - 2; ++x) {
    gfx_set_cursor(x, y_bot);
    gfx_putc(GLYPH(char_border_single_horizontal_s));
  }
  gfx_set_cursor(TEXT_WIDTH - 2, y_bot);
  gfx_putc(GLYPH(char_border_single_bottomRight_s));

  gfx_draw_changed();
}
