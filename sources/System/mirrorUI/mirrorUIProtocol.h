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

#ifndef _MIRRORUIPROTOCOL_H_
#define _MIRRORUIPROTOCOL_H_

#include <cstdint>

#define GUIColorToRGB565(color) ((color.r_ & 0b11111000) << 8) | ((color.g_ & 0b11111100) << 3) | (color.b_ >> 3)

enum mirrorUICommandType: uint8_t {
  cmdPalette = 0x00,
  cmdData = 0x01,
  cmdFont = 0x02,
  cmdInput = 0x03
};

typedef struct mui_CommandDataCharacter {
  struct {
    uint8_t fg:4;
    uint8_t bg:4;
  } color;
  uint8_t character;
} mui_CommandDataCharacter;

typedef struct mui_CommandData {
  uint8_t x;
  uint8_t y;
  uint8_t length;

  mui_CommandDataCharacter characters[32];
} mui_CommandData;

typedef struct rgb565color {
  uint8_t r:5;
  uint8_t g:6;
  uint8_t b:5;
} rgb565color;

typedef struct mui_CommandPalette {
  rgb565color color[16];
} mui_CommandPalette;

typedef struct mui_CommandFont {
  uint8_t fontId;
} mui_CommandFont;

typedef struct mirrorUICommand {
  // header, not transmitted
  uint8_t payloadSize;

  // data blob
  mirrorUICommandType type;

  union {
    mui_CommandData data;
    mui_CommandPalette palette;
    mui_CommandFont font;
    uint8_t *raw;
  } payload;

  uint8_t *bytes() {
    return (uint8_t *)&type;
  }
} mirrorUICommand;

const mirrorUICommand *mirrorUI_command_Font(uint8_t index);
const mirrorUICommand *mirrorUI_command_Palette(rgb565color palette[16]);
const mirrorUICommand *mirrorUI_command_Data(uint8_t length, uint8_t *text, uint8_t *color);

void mirrorUI_Flush(uint8_t *screen, uint8_t *colors, bool *changed);

#endif
