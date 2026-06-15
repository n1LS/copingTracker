/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "mirrorUIProtocol.h"
#include "Application/AppWindow.h"

mirrorUICommand command_;

uint8_t mirrorUI_calculateChecksum(uint8_t *buffer, int size) {
  uint8_t chk = 0;

  while (size-- >= 0) {
    chk ^= *buffer++;
  }

  return chk;
}

void mirrorUI_setChecksum(mirrorUICommand *command) {
  int size = 0;

  switch (command->type) {
    case cmdFont:   
      size = sizeof(mui_CommandFont);
      break;
    case cmdPalette:
      size = sizeof(mui_CommandPalette);
      break;
    case cmdData:
      // length, x, y + data (2 bytes each)
      size = 3 + 2 * command->payload.data.length;
      break;
    default:
      // input not handled in this direction
      break;
  }

  uint8_t checksum = mirrorUI_calculateChecksum((uint8_t *)(command) + 2, size + 1);
  command->payload.raw[size + 1] = checksum;
}

const mirrorUICommand *mirrorUI_command_Font(uint8_t index) {
  command_.type = cmdFont;
  command_.payload.font.fontId = index;
  mirrorUI_setChecksum(&command_);
  return &command_;
}

const mirrorUICommand *mirrorUI_command_Palette(uint16_t *palette) {
  mirrorUI_setChecksum(&command_);
  return &command_;
}

const mirrorUICommand *mirrorUI_command_Data(uint8_t length, uint8_t *text, uint8_t *color) {
  mirrorUI_setChecksum(&command_);
  return &command_;
}

void mirrorUI_Flush(uint8_t *screen, uint8_t *colors, bool *changed) {
  int index = 0;

#define WAITING_FOR_CHANGED 0

  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    int x = 0;

    int state = WAITING_FOR_CHANGED;

    while (x < SCREEN_WIDTH) {
      if (changed[index]) {

      }    
    }
  }
}

/*


// Helper function for byte escaping.
static uint16_t addByteEscaped(char *buffer, uint16_t bufferIndex, char byte) {
  if (byte == REMOTE_UI_CMD_MARKER || byte == REMOTE_UI_ESC_CHAR) {
    buffer[bufferIndex++] = REMOTE_UI_ESC_CHAR;
    buffer[bufferIndex++] = byte ^ REMOTE_UI_ESC_XOR;
  } else {
    buffer[bufferIndex++] = byte;
  }
  return bufferIndex;
}

// Helper function to add a 16-bit value, escaping each of its two bytes.
static uint16_t add16bitEscaped(char *buffer, uint16_t bufferIndex, uint16_t val) {
  bufferIndex = addByteEscaped(buffer, bufferIndex, val & 0xFF);        // Add LSB
  bufferIndex = addByteEscaped(buffer, bufferIndex, (val >> 8) & 0xFF); // Add MSB
  return bufferIndex;
}

void mirrorUIFontCommand(uint8_t uifontIndex, char *buffer) {
  buffer[0] = REMOTE_UI_CMD_MARKER;
  buffer[1] = SETFONT_CMD;
  buffer[2] = uifontIndex + ASCII_SPACE_OFFSET;
}

void mirrorUIDrawCharCommand(const char c, uint8_t x, uint8_t y, bool invert, char *buffer) {
  buffer[0] = REMOTE_UI_CMD_MARKER;
  buffer[1] = TEXT_CMD;
  buffer[2] = c;
  buffer[3] = x + ASCII_SPACE_OFFSET; // to avoid sending NUL (aka 0)
  buffer[4] = y + ASCII_SPACE_OFFSET;
  buffer[5] = invert ? 127 : 0;
}

uint16_t mirrorUIDrawRectCommand(int left, int top, int width, int height, char *buffer) {
  uint16_t bufferIndex = 0;
  buffer[bufferIndex++] = REMOTE_UI_CMD_MARKER;
  buffer[bufferIndex++] = DRAWRECT_CMD;
  bufferIndex = add16bitEscaped(buffer, bufferIndex, left);
  bufferIndex = add16bitEscaped(buffer, bufferIndex, top);
  bufferIndex = add16bitEscaped(buffer, bufferIndex, width);
  bufferIndex = add16bitEscaped(buffer, bufferIndex, height);
  return bufferIndex;
}

void mirrorUIClearCommand(uint16_t r, uint16_t g, uint16_t b, char *buffer) {
  buffer[0] = REMOTE_UI_CMD_MARKER;
  buffer[1] = CLEAR_CMD;
  buffer[2] = r;
  buffer[3] = g;
  buffer[4] = b;
}

// todo: cleanup and make these a single function
uint16_t mirrorUISetColorCommand(uint16_t r, uint16_t g, uint16_t b, char *buffer) {
  uint16_t bufferIndex = 0;
  buffer[bufferIndex++] = REMOTE_UI_CMD_MARKER;
  buffer[bufferIndex++] = SETCOLOR_CMD;
  bufferIndex = addByteEscaped(buffer, bufferIndex, r);
  bufferIndex = addByteEscaped(buffer, bufferIndex, g);
  bufferIndex = addByteEscaped(buffer, bufferIndex, b);
  return bufferIndex;
}

uint16_t mirrorUISetBackgroundColorCommand(uint16_t r, uint16_t g, uint16_t b, char *buffer) {
  uint16_t bufferIndex = 0;
  buffer[bufferIndex++] = REMOTE_UI_CMD_MARKER;
  buffer[bufferIndex++] = SETBACKGROUNDCOLOR_CMD;
  bufferIndex = addByteEscaped(buffer, bufferIndex, r);
  bufferIndex = addByteEscaped(buffer, bufferIndex, g);
  bufferIndex = addByteEscaped(buffer, bufferIndex, b);
  return bufferIndex;
}
*/