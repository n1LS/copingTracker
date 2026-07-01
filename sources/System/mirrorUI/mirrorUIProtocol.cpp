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

uint8_t mirrorUI_calculateChecksum(uint8_t *buffer, int size) {
  uint8_t chk = 0;

  while (size > 0) {
    chk ^= *buffer++;
    size--;
  }

  return chk;
}

void mirrorUI_command_Font(mirrorUICommand *command, uint8_t index) {
  command->payload[0] = cmdFont;
  command->payload[1] = index;
  command->payloadSize = 2;
}

void mirrorUI_command_Palette(mirrorUICommand *command, uint16_t *palette) {
  int index = 1;

  for (int c = 0; c < 16; c++) {
    command->payload[index++] = (palette[c] >> 8);
    command->payload[index++] = (palette[c] & 0xff);
  }

  command->payload[0] = cmdPalette;
  command->payloadSize = 33;
}