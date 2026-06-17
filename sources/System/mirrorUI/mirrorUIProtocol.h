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

enum mirrorUICommandType {
  cmdPalette = 0x00,
  cmdData = 0x01,
  cmdFont = 0x02,
  cmdInput = 0x03
};

typedef struct mirrorUICommand {
  uint8_t payload[68];
  uint8_t payloadSize;
} mirrorUICommand;

void mirrorUI_command_Font(mirrorUICommand *command, uint8_t index);
void mirrorUI_command_Palette(mirrorUICommand *command, uint16_t *palette);

void mirrorUI_setChecksum(mirrorUICommand *command);

#endif