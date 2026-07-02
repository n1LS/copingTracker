#include "InputTester.h"
#include "Adapters/copingTracker/display/chargfx.h"
#include "Adapters/copingTracker/system/input.h"
#include "BaseClasses/View.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include <System/Console/nanoprintf.h>
#include <string.h>

// Helper function to draw a single button with optional inversion
static void draw_string(int x, int y, const char *s) {
  while (*s) {
    chargfx_set_cursor(x++, y);
    chargfx_putc(*s++);
  }
}

static void draw_button(uint8_t x, uint8_t y, char label, bool active) {
  // todo: use active to set fg/bg
  const char line[4] = {CHAR(char_border_single_vertical_s), label, CHAR(char_border_single_vertical_s), 0};
  draw_string(x, y + 0, char_border_single_topLeft_s char_border_single_horizontal_s char_border_single_topRight_s);
  draw_string(x, y + 1, (const char *)line);
  draw_string(x, y + 2,
              char_border_single_bottomLeft_s char_border_single_horizontal_s char_border_single_bottomRight_s);
}

void drawInputTester() {
  // Read button state
  uint16_t keys = scanKeys();

  // Title
  draw_string(12, 2, "Button Tester");

  // Row 1: Up, Play, Edit
  draw_button(13, 14, CHAR(char_button_up_s), keys & BM_UP);
  draw_button(16, 14, CHAR(char_button_play_s), keys & BM_PLAY);
  draw_button(19, 14, CHAR(char_button_edit_s), keys & BM_EDIT);

  // Row 2: Left, Down, Right, Enter/Set
  draw_button(10, 17, CHAR(char_button_left_s), keys & BM_LEFT);
  draw_button(13, 17, CHAR(char_button_down_s), keys & BM_DOWN);
  draw_button(16, 17, CHAR(char_button_right_s), keys & BM_RIGHT);
  draw_button(19, 17, CHAR(char_button_enter_s), keys & BM_ENTER);

  // Row 3: Alt, Nav
  draw_button(13, 20, CHAR(char_button_alt_s), keys & BM_ALT);
  draw_button(16, 20, CHAR(char_button_nav_s), keys & BM_NAV);
}
