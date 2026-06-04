#ifndef PICOTRACKER_BOOTLOADER_CHARGFX_H
#define PICOTRACKER_BOOTLOADER_CHARGFX_H

#include "Adapters/picoTracker/display/ili9341.h"
#include "Foundation/Types/Colors.h"
#include "Foundation/Constants/SpecialCharacters.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHAR_HEIGHT 8
#define CHAR_WIDTH 8
#define TEXT_WIDTH (ILI9341_TFTHEIGHT / CHAR_WIDTH)
#define TEXT_HEIGHT (ILI9341_TFTWIDTH / CHAR_HEIGHT)

void chargfx_set_cursor(uint8_t x, uint8_t y);
void chargfx_draw_screen(void);
void chargfx_draw_sub_region(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void chargfx_draw_changed();
void chargfx_putc(char c);
void chargfx_set_foreground(Color color);
void chargfx_set_background(Color color);
void chargfx_clear(Color color);

#ifdef __cplusplus
}
#endif

#endif