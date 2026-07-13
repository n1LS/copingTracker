/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#pragma once

static const char drumNames[12][9] = {
  "BassDrum", // c
  "Rimshot ",
  "Snare   ", // d
  "Clap    ",
  "Snare 2 ", // e
  "Tom 1   ",
  "HHclosed", // f#
  "Tom 2   ",
  "HHalt   ", // g#
  "Tom 3   ",
  "HHopen  ", // a#
  "Crash   ",
};

static const char drumShortNames[12][4] = {
  "BDr", // c
  "Rim",
  "Snr", // d
  "Clp",
  "Sn2", // e
  "TmL",
  "HHc", // f#
  "TmM",
  "HHp", // g#
  "TmH",
  "HHo", // a#
  "Csh",
};

enum drum_constants_e {
  drumEnvAttackThreshold = 65530,
  drumEnvDecayThreshold = 10,
  drumNumWaveforms = 8,
  drumQ16_16_1 = 0x0001'0000,
  drumTicks100Hz = 441,
  drumTicks1000Hz = 44,
};

enum drum_wave_type_e : uint8_t {
  drumWavePulse12_5 = 0,
  drumWavePulse25,
  drumWavePulse50,
  drumWaveTriangle,
  drumWaveNoiseGameBoy7,
  drumWaveNoiseNES,
  drumWaveNoiseSN76489,
  drumWaveNoiseWhite,
  drumWaveLastItem = drumWaveNoiseWhite,
  drumWaveNone
};

enum drum_env_state_e : uint8_t { drumEnvIdle, drumEnvDecay };

enum drum_instrument_defaults_e {
  defaultInstrument0 = 0x1234,
  defaultInstrument1 = 0x5463,
  defaultInstrument2 = 0x1234,
  defaultInstrument3 = 0xfff3,
  defaultInstrument4 = 0x2145,
  defaultInstrument5 = 0x2345,
  defaultInstrument6 = 0x8653,
  defaultInstrument7 = 0x3242,
  defaultInstrument8 = 0x3423,
  defaultInstrument9 = 0x8888,
  defaultInstrument10 = 0x8888,
  defaultInstrument11 = 0x8888,
};

typedef union drumFlags {
  struct {
    uint8_t retrigger : 1;
    uint8_t unused : 7;
  };
  uint8_t byte;
} drumFlags;