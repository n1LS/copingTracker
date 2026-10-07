/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include "Config/AudioConstants.h"

enum lsdjkit_constants_e {
  lsdjkitTicks100Hz = SAMPLE_RATE_HZ / 100,
  lsdjkitTicks1000Hz = SAMPLE_RATE_HZ / 1000,
};

typedef enum lsdjkit_loop_mode_e {
  loopModeOff = 0,
  loopModeOn = 1,
  loopModeAttack = 2,
} lsdjkit_loop_mode_e;

typedef struct lsdjkit_dual_loop_mode_t {
  uint8_t mode1 : 4;
  uint8_t mode2 : 4;
} lsdjkit_dual_loop_mode_t;

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
    uint8_t initialized : 1;
    uint8_t unused : 6;
  };
  uint8_t byte;
} lsdjkit_flags;

static const int clippingModeCount = 5;
static const char *clippingModeNames[clippingModeCount] = {"No ", char_waveform_pulse_s, char_waveform_sine_s, "Fld",
                                                           "Wrp"};

static const int speedModeCount = 4;
static const char *speedModeNames[speedModeCount] = {"1/4", "1/2", "1.0", "2.0"};

static const int loopModeCount = 3;
static const char *loopModeNames[loopModeCount] = {"Off", "Atk", "Lop"};