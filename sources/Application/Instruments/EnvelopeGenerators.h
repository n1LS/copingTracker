/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

#include <cstdint>

#include "ChiptuneInstrument/ChiptuneEnums.h"
#include "ChiptuneInstrument/ChiptuneMath.h"
#include "ChiptuneInstrument/ChiptuneTables.h"

#pragma pack(push, 1)
typedef struct adsr_envelope_t {
  uint16_t value;
  uint16_t coefficient; // q0.16
  uint16_t attack;
  uint16_t decay;
  uint16_t sustain; // sustain level (0-65535)
  uint16_t release;
  chiptune_env_state_e state;

  void set_attack(uint8_t a) {
    // map 8 bit attack value to 16 bit coefficient using LUT and interpolation
    attack = interpolateU16(attackCoeffLUT.data(), a);
  }

  void set_decay(uint8_t d) {
    // map 8 bit decay value to 16 bit coefficient using LUT and interpolation
    decay = interpolateU16(decayCoeffLUT.data(), d);
  }

  void set_sustain(uint8_t s) {
    // map 8 bit sustain level (0-255) to 16 bit value (0-65535)
    sustain = (uint16_t)s << 8;
  }

  void set_release(uint8_t r) {
    // map 8 bit release value to 16 bit coefficient using LUT and interpolation
    release = interpolateU16(decayCoeffLUT.data(), r);
  }

  void trigger() {
    coefficient = attack;
    state = envAttack;
    value = 0;
  }

  void release_note() {
    if (state == envAttack || state == envDecay || state == envSustain) {
      coefficient = release;
      state = envRelease;
    }
  }

  void tick() {
    if (state == envIdle)
      return;

    uint32_t diff;
    int32_t tmp;

    switch (state) {
      case envAttack:
        diff = 0xFFFF - value;
        tmp = value + ((diff * coefficient) >> 16);
        if (tmp >= envAttackThreshold) {
          tmp = 0xFFFF;
          coefficient = decay;
          state = envDecay;
        }
        break;

      case envDecay:
        diff = value; // decay from 0xFFFF down to sustain level
        tmp = value - ((diff * coefficient) >> 16);
        if (tmp <= sustain) {
          tmp = sustain;
          state = envSustain;
        }
        break;

      case envSustain:
        // hold at sustain level until release_note() is called
        return;

      case envRelease:
        diff = value; // release from current level down to 0
        tmp = value - ((diff * coefficient) >> 16);
        if (tmp <= envDecayThreshold) {
          tmp = 0;
          state = envIdle;
        }
        break;

      default:
        return;
    }

    value = tmp;
  }
} adsr_envelope_t;
#pragma pack(pop)