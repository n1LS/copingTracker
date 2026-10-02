/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

enum lsdjkit_constants_e {
  lsdjkitTicks100Hz = 441, // TODO nILS: sample rate / 100
  lsdjkitTicks1000Hz = 44, // TODO nILS: sample rate / 1000
};

typedef enum lsdjkit_loop_mode_e {
  loopModeOff = 0,
  loopModeOn = 1,
  loopModeAttack = 2,
} lsdjkit_loop_mode_e;

typedef enum lsdjkit_clip_mode_e {
  clipModeNone = 0,
  clipModeHard,
  clipModeSoft,
  clipModeFold,
  clipModeWrap,
} lsdjkit_clip_mode_e;

enum lsdjkit_const {
  lsdjDefaultKit1 = 0,
  lsdjDefaultKit2 = 1,
  lsdjDefaultBitDepth = 4,
  lsdjDefaultOffset = 0,
  lsdjDefaultLength = -1,
  lsdjDefaultSpeed = 2,
  lsdjDefaultClip = 0,
  lsdjDefaultLoop = loopModeOff,
};

typedef union lsdjkit_flags {
  struct {
    uint8_t retrigger : 1;
    uint8_t loop_mode : 2;
    uint8_t unused : 5;
  };
  uint8_t byte;
} lsdjkit_flags;

static const int clippingModeCount = 5;
static const char *clippingModeNames[clippingModeCount] = { "None", "Hard", "Soft", "Fold", "Wrap" };

static const int speedModeCount = 4;
static const char *speedModeNames[speedModeCount] = { "1/4", "1/2", "1.0", "2.0" };

static const int loopModeCount = 3;
static const char *loopModeNames[loopModeCount] = { "Off", "Loop from offset", "Loop from 0" };