/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "mirrorUI.h"
#include "mirrorUIProtocol.h"
#include "Application/AppWindow.h"
#include "tusb.h"
#include "Adapters/picoTracker/display/chargfx.h"
#include <cstdint>

static mirrorUICommand command_;

void mirrorUI_sendCommand(mirrorUICommand *command) {
  command->payload[command->payloadSize] = 0x7f;  
  command->payloadSize++;

  if (!tud_cdc_connected())
        return;

    uint32_t total = command->payloadSize;
    uint32_t i = 0;

    while (i < total) {
        tud_task(); // keep USB stack alive

        uint32_t avail = tud_cdc_write_available();
        if (avail == 0) {
            tud_cdc_write_flush();
            continue;
        }

        uint32_t n = total - i;
        if (n > avail)
            n = avail;

        uint32_t written = tud_cdc_write(command->payload + i, n);

        if (written == 0) {
            tud_task();
            tud_cdc_write_flush();
            continue;
        }

        i += written;
    }

    tud_cdc_write_flush(); // flush once at end
}

void mirrorUI_Flush(uint8_t *screen, uint8_t *colors, bool *changed, bool fullUpdate) {
  int index = 0;

  #define WAITING_FOR_CHANGED 0
  #define IN_CHANGED 1
   
  command_.payload[0] = cmdData;

  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    int length = 0;
    int ptr = 2;
    int state = WAITING_FOR_CHANGED;

    for (int x = 0; x < SCREEN_WIDTH; x++) {
      switch (state) {
        case WAITING_FOR_CHANGED:
          if (changed[index] || fullUpdate) {
            // reset writings
            state = IN_CHANGED;
            command_.payload[2] = x;
            command_.payload[3] = y;
            ptr = 4;
            command_.payload[ptr++] = colors[index];
            command_.payload[ptr++] = screen[index];
            length++;
          }
          break;
        case IN_CHANGED:
          // Check bounds before accessing changed array
          if (changed[index] || fullUpdate || 
              ((x + 1 < SCREEN_WIDTH) && changed[index + 1]) ||
              ((x + 2 < SCREEN_WIDTH) && changed[index + 2]) || 
              ((x + 3 < SCREEN_WIDTH) && changed[index + 3])) {
            // keep going
            command_.payload[ptr++] = colors[index];
            command_.payload[ptr++] = screen[index];
            length++;
          } else {
            // done with this block, output
            command_.payload[1] = length;
            command_.payloadSize = length * 2 + 4;
            // mirrorUI_setChecksum(&command_);
            mirrorUI_sendCommand(&command_);
            // clear
            length = 0;
            // reset state
            state = WAITING_FOR_CHANGED;
          }
          break;
      }
      index++;
    }

    if (state == IN_CHANGED) {
      // ended with a block at the end of the row
      command_.payload[1] = length;
      command_.payloadSize = length * 2 + 4;
      // mirrorUI_setChecksum(&command_);
      mirrorUI_sendCommand(&command_);
    }
  }
}

void mirrorUI_sendPalette(uint16_t *color) {
  uint16_t *palette = chargfx_get_palette();

  for (int c = 0; c < 16; c++) {
    uint8_t val = palette[c];
    uint8_t val2 = palette[c] >> 8;
    command_.payload[1 + c * 2] = val2;
    command_.payload[2 + c * 2] = val;
  }
  
  command_.payload[0] = 0;
  command_.payloadSize = 33;

  mirrorUI_sendCommand(&command_);
}

mirrorUICommand *mirrorUI_getCommand() {
  return &command_;
}

void mirrorUI_connected() {
  // send palette
  mirrorUI_sendPalette(chargfx_get_palette());
  // send font
  mirrorUICommand *command = mirrorUI_getCommand();
  mirrorUI_command_Font(command, chargfx_get_font_index());
  mirrorUI_sendCommand(command);
  // send screen
  uint8_t *scr, *col;
  bool *chg;
  chargfx_get_screen_storage(&scr, &col, &chg);
  mirrorUI_Flush(scr, col, chg, true);
}