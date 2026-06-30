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

#include <cstdio>
#include <cstring>

struct HelpLegend {
  const char line1[27]; // 26 + '\0'
  const char line2[27]; // 26 + '\0'
  const char line3[27]; // 26 + '\0'
  const char line4[27]; // 26 + '\0'
};

#define LEGEND(N, A, B, C, D)                                                                                          \
  static constexpr HelpLegend legend_##N = {A, B, C, D};                                                               \
  return legend_##N;

// CAUTION: all strings must fit in the line length limits!
// First line is max 32 - battery gauge (4) = 28
// Second line is max 32 chars
HelpLegend getHelpLegend(FourCC command) {
  switch (command) {
    case FourCC::InstrumentCommandKill:
      LEGEND(00,
    "KILl: --aa                ",
    "Stop songn playback after ",
    "aa ticks.                 ",
    "                          ");
    case FourCC::InstrumentCommandLoopOffset:
      LEGEND(01,
    "Loop OFset: aaaa          ",
    "Shift loop start & end by ",
    "aaaa samples.             ",
    "                          ");
    case FourCC::InstrumentCommandArpeggiator:
      LEGEND(02,
    "ARPeggio: abcd            ",
    "Cycle through the relative",
    "offsets a,b,c&d. If a step",
    "is 0 the arp starts over.");
    case FourCC::InstrumentCommandVolume:
      LEGEND(03,
    "VOLume: aabb              ",
    "Move towards volume bb at ",
    "the rate aa.              ",
    "                          ");
    case FourCC::InstrumentCommandVelocity:
      LEGEND(04,
    "VELocity: --bb            ",
    "send MIDI velocity command",
    "bb                        ",
    "                          ");
    case FourCC::InstrumentCommandPitchSlide:
      LEGEND(05,
    "Pitch SLide: aabb         ",
    "Slide to pitch bb at rate ",
    "aa.                       ",
    "                          ");
    case FourCC::InstrumentCommandHop:
      LEGEND(06,
    "HOP: aabb                 ",
    "Hop to step bb for aa     ",
    "times.                    ",
    "                          ");
    case FourCC::InstrumentCommandLegato:
      LEGEND(07,
    "LEGato: aabb              ",
    "Slide to pitch bb at speed",
    "aa.                       ",
    "                          ");
    case FourCC::InstrumentCommandRetrigger:
      LEGEND(08,
    "ReTriGger: aabb           ",
    "Sample: bb loop+aa offset ",
    "MIDI: bb                  ",
    "                          ");
    case FourCC::InstrumentCommandTempo:
      LEGEND(09,
    "TemPO: -aaa               ",
    "set tempo to hex value aaa",
    "Max is 12C == 300bpm      ",
    "                          ");
    case FourCC::InstrumentCommandMidiCC:
      LEGEND(10,
    "Midi CC: aabb             ",
    "CC message aa value bb    ",
    "                          ",
    "                          ");
    case FourCC::InstrumentCommandMidiPC:
      LEGEND(11,
    "Midi Program Change: --bb ",
    "send program change bb    ",
    "                          ",
    "                          ");
    case FourCC::InstrumentCommandPlayOfset:
      LEGEND(12,
    "Play OFfset: aabb         ",
    "jump absolute to aa & move",
    "relative signed bb        ",
    "                          ");
    case FourCC::InstrumentCommandFilterResonance:
      LEGEND(13,
    "Filter & ReS: aabb        ",
    "speed aa, resonance bb    ",
    "                          ",
    "                          ");
    case FourCC::InstrumentCommandLowPassFilter:
      LEGEND(14,
    "FiLTer: aabb              ",
    "Set filter cutoff aa and  ",
    "resonance bb.             ",
    "                          ");
    case FourCC::InstrumentCommandTable:
      LEGEND(15,
    "TaBLe: --bb               ",
    "Run table bb.             ",
    "                          ",
    "                          ");
    case FourCC::InstrumentCommandCrush:
      LEGEND(16,
    "drive & CruSH: aa-b       ",
    "Set drive level to aa and ",
    "Crush to -b (0-F)         ",
    "                          ");
    case FourCC::InstrumentCommandFilterCut:
      LEGEND(17,
    "Filter CuToff: aabb       ",
    "Move towards target cutoff",
    "bb at rate aa.            ",
    "                          ");
    case FourCC::InstrumentCommandPan:;
      LEGEND(18,
    "PAN: aabb                 ",
    "speed aa, value bb        ",
    "00=right, 80=center,      ",
    "FF=left                   ");
    case FourCC::InstrumentCommandGroove:
      LEGEND(19,
    "GRooVe: aabb              ",
    "set bb. If aa > 0 set this",
    "groove on all tracks      ",
    "                          ");
    case FourCC::InstrumentCommandInstrumentRetrigger:
      LEGEND(20,
    "Instrument ReTrigger: --bb",
    "retrigger & transpose by  ",
    "bb                        ",
    "                          ");
    case FourCC::InstrumentCommandPitchFineTune:
      LEGEND(21,
    "PitchFineTune: aabb       ",
    "speed aa, tune bb         ",
    "(~+/-1st)                 ",
    "                          ");
    case FourCC::InstrumentCommandDelay:
      LEGEND(22,
    "DeLaY: ---b               ",
    "delay b+1 ticks           ",
    "                          ",
    "                          ");
    case FourCC::InstrumentCommandSetInstrumentParameter:
      LEGEND(23,
    "Set Instrument Parameter  ",
    "aabb: set param aa to     ",
    "value bb                  ",
    "                          ");
    case FourCC::InstrumentCommandStop:
      LEGEND(24,
    "Stop Table Playback       ",
    "Instantly stop playing the",
    "current table.            ",
    "                          ");
    case FourCC::InstrumentCommandGateOff:
      LEGEND(25,
    "GateOFf: Synth only       ",
    "                          ",
    "                          ",
    "                          ");
    case FourCC::InstrumentCommandMidiChord:
      LEGEND(26,
    "Midi CHord:abcd           ",
    "send rel notes:+a,+b,+c,+d",
    "                          ",
    "                          ");
    case FourCC::InstrumentCommandVibrato:
      LEGEND(27,
    "VIBrato:aabb              ",
    "rate aa, depth bb         ",
    "                          ",
    "                          ");
  }

  LEGEND(28,
    "Unknown Command: This     ",
    "should not happen.        ",
    "Report bug #1334        ",
    "                          ");
}
