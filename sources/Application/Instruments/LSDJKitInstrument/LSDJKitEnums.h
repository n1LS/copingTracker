/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

enum lsdjkit_constants_e {
  lsdjkitTicks100Hz = 441,
  lsdjkitTicks1000Hz = 44,
};

enum lsdjkit_const {
  defaultKit1 = 0,
  defaultKit2 = 1,
  defaultBitDepth = 4,
};

typedef union lsdjkit_flags {
  struct {
    uint8_t retrigger : 1;
    uint8_t unused : 7;
  };
  uint8_t byte;
} lsdjkit_flags;