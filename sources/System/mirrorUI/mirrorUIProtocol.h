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
  cmdInput = 0x02,
  cmdRect = 0x03,
};

enum mirrorUIKey {
  muikUp = 0x01,
  muikLeft = 0x02,
  muikDown = 0x03,
  muikRight = 0x04,
  muikAlt = 0x05,
  muikNav = 0x06,
  muikPlay = 0x07,
  muikEdit = 0x08,
  muikEnter = 0x09,
  muikLastEntry = 0x09
};

enum mirrorUIKeyState { muiksDown = 0x00, muiksUp = 0x01 };

void mirrorUI_handleInput(uint8_t key, uint8_t state);

typedef struct mirrorUICommand {
  uint8_t payload[100];
  uint8_t payloadSize;
} mirrorUICommand;

#endif