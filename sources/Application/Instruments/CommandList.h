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

#ifndef _COMMAND_LIST_H_
#define _COMMAND_LIST_H_

#include "Foundation/Types/Types.h"

class CommandList {
public:
  static Token GetNext(Token current);
  static Token GetPrev(Token current);
  static Token GetNextAlpha(Token current);
  static Token GetPrevAlpha(Token current);

  static Token GetFirst();
  static Token GetFirstAlpha();

  static const int CommandCount;
  static const Token AllCommands[];

  // Applies command-specific range limits to parameter values
  // Currently handles:
  // - VEL: Ensures MIDI velocity values don't exceed 127 (0x7F)
  // Can be extended to handle other commands in the future
  // Returns the range-limited parameter value
  static uint16_t RangeLimitCommandParam(Token command, uint16_t paramValue);
};
#endif
