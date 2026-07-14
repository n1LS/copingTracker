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

#include "Application/Utils/TintChar.h"
#include <cstdio>
#include <cstring>

struct CommandHelp {
  etl::array<TintChar, 27> line1;
  etl::array<TintChar, 27> line2;
  etl::array<TintChar, 27> line3;
  etl::array<TintChar, 27> line4;
};

#define LEGEND(N, A, B, C, D)                                                                                          \
  static constexpr CommandHelp legend_##N = {A, B, C, D};                                                              \
  return legend_##N;

// clang-format off

// CAUTION: all strings must fit in the line length limits!
// Second line is max 26 chars
CommandHelp getCommandHelp(Token command) {
  switch (command) {
    case Token::InstrumentCommandKill: 
      return CommandHelp( 
        makeTintString(
          "f  7    a 7               ",
          "0                         ",
          "KILl: --aa                "),
        makeTintString(
          "7                         ",
          "0                         ",
          "Stop song playback after  "),
        makeTintString(
          "a 7                       ",
          "0                         ",
          "aa ticks.                 "),
        makeTintString(
          "7                         ",
          "0                         ",
          "                          ")
      );
    case Token::InstrumentCommandArpeggiator:
      return CommandHelp(
        makeTintString(
          "f  7      abcd            ",
          "0                         ",
          "ARPeggio: abcd            "),
        makeTintString(
          "7                         ",
          "0                         ",
          "Cycle through the relative"),
        makeTintString(
          "7       a7b7c7d7          ",
          "0                         ",
          "offsets a,b,c&d. If a step"),
        makeTintString(
          "7                         ",
          "0                         ",
          "is 0 the arp starts over. ")
      );

    case Token::InstrumentCommandVolume:
      return CommandHelp(
        makeTintString(
          "f  7    a b 7             ",
          "0                         ",
          "VOLume: aabb              "),
        makeTintString(
          "7                   b 7   ",
          "0                         ",
          "Move towards volume bb at "),
        makeTintString(
          "7        a 7              ",
          "0                         ",
          "the rate aa.              "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandVelocity:
      return CommandHelp(
        makeTintString(
          "f  7        b 7           ",
          "0                         ",
          "VELocity: --bb            "),
        makeTintString(
          "7                         ",
          "0                         ",
          "send MIDI velocity command"),
        makeTintString(
          "b 7                       ",
          "0                         ",
          "bb                        "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandPitchSlide:
      return CommandHelp(
        makeTintString(
          "f7    f 7    a b 7        ",
          "0                         ",
          "Pitch SLide: aabb         "),
        makeTintString(
          "7              b 7        ",
          "0                         ",
          "Slide to pitch bb at rate "),
        makeTintString(
          "a 7                       ",
          "0                         ",
          "aa.                       "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandHop:
      return CommandHelp(
        makeTintString(
          "f  7 a b 7                ",
          "0                         ",
          "HOP: aabb                 "),
        makeTintString(
          "7           b 7    a 7    ",
          "0                         ",
          "Hop to step bb for aa     "),
        makeTintString(
          "7                         ",
          "0                         ",
          "times.                    "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandLegato:
      return CommandHelp(
        makeTintString(
          "f  7    a b 7             ",
          "0                         ",
          "LEGato: aabb              "),
        makeTintString(
          "7              b 7        ",
          "0                         ",
          "Slide to pitch bb at speed"),
        makeTintString(
          "a 7                       ",
          "0                         ",
          "aa.                       "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandRetrigger:
      return CommandHelp(
        makeTintString(
          "f7f77f7    a b 7          ",
          "0                         ",
          "ReTriGger: aabb           "),
        makeTintString(
          "7       b 7     a 7       ",
          "0                         ",
          "Sample: bb loop+aa offset "),
        makeTintString(
          "7     b 7                 ",
          "0                         ",
          "MIDI: bb                  "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandLoopOffset:
      return CommandHelp(
        makeTintString(
          "f7   f 7    a   7         ",
          "0                         ",
          "Loop OFset: aaaa          "),
        makeTintString(
          "7                         ",
          "0                         ",
          "Shift loop start & end    "),
        makeTintString(
          "7         a   7           ",
          "0                         ",
          "values by aaaa            "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandTempo:
      return CommandHelp(
        makeTintString(
          "f7 ff7  a  7              ",
          "0                         ",
          "TemPO: -aaa               "),
        makeTintString(
          "7                      a  ",
          "0                         ",
          "set tempo to hex value aaa"),
        makeTintString(
          "7          8              ",
          "0                         ",
          "Max is 12C == 300bpm      "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandMidiCC:
      return CommandHelp(
        makeTintString(
          "f7   f 7 a b 7            ",
          "0                         ",
          "Midi CC: aabb             "),
        makeTintString(
          "7          a 7      b 7   ",
          "0                         ",
          "CC message aa value bb    "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandMidiPC:
      return CommandHelp(
        makeTintString(
          "f7   f7      f7        b 7",
          "0                         ",
          "Midi Program Change: --bb "),
        makeTintString(
          "7                   b 7   ",
          "0                         ",
          "send program change bb    "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandPlayOfset:
      return CommandHelp(
        makeTintString(
          "f7   f 7     a b 7        ",
          "0                         ",
          "Play OFfset: aabb         "),
        makeTintString(
          "7                a 7      ",
          "0                         ",
          "jump absolute to aa & move"),
        makeTintString(
          "7               b 7       ",
          "0                         ",
          "relative signed bb        "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandFilterResonance:
      return CommandHelp(
        makeTintString(
          "f7       f7f  a b 7       ",
          "0                         ",
          "Filter & ReS: aabb        "),
        makeTintString(
          "7     a 7           b 7   ",
          "0                         ",
          "speed aa, resonance bb    "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandLowPassFilter:
      return CommandHelp(
        makeTintString(
          "f7f 7   a b 7             ",
          "0                         ",
          "FiLTer: aabb              "),
        makeTintString(
          "7                 a 7     ",
          "0                         ",
          "Set filter cutoff aa and  "),
        makeTintString(
          "7         b               ",
          "0                         ",
          "resonance bb.             "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandTable:
      return CommandHelp(
        makeTintString(
          "f7f 7    b 7              ",
          "0                         ",
          "TaBLe: --bb               "),
        makeTintString(
          "7         b 7             ",
          "0                         ",
          "Run table bb.             "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandCrush:
      return CommandHelp(
        makeTintString(
          "7       f7 f 7 a 7b7      ",
          "0                         ",
          "drive & CruSH: aa-b       "),
        makeTintString(
          "7                  a 7    ",
          "0                         ",
          "Set Drive level to aa and "),
        makeTintString(
          "7         b78             ",
          "0                         ",
          "Crush to -b (0-F)         "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandFilterCut:
      return CommandHelp(
        makeTintString(
          "f7     f7f7    a b 7      ",
          "0                         ",
          "Filter CuToff: aabb       "),
        makeTintString(
          "7                         ",
          "0                         ",
          "Move towards target cutoff"),
        makeTintString(
          "b 7        a 7            ",
          "0                         ",
          "bb at rate aa.            "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandPan:
      return CommandHelp(
        makeTintString(
          "f  7 a b 7                ",
          "0                         ",
          "PAN: aabb                 "),
        makeTintString(
          "7     a 7       b 7       ",
          "0                         ",
          "speed aa, value bb        "),
        makeTintString(
          "8                         ",
          "0                         ",
          "00=right, 80=center,      "),
        makeTintString(
          "8                         ",
          "0                         ",
          "FF=left                   ")
      );

    case Token::InstrumentCommandGroove:
      return CommandHelp(
        makeTintString(
          "f 7 f7  a b 7             ",
          "0                         ",
          "GRooVe: aabb              "),
        makeTintString(
          "7   b 7    a 7            ",
          "0                         ",
          "set bb. If aa > 0 set this"),
        makeTintString(
          "7                         ",
          "0                         ",
          "groove on all tracks      "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandInstrumentRetrigger:
      return CommandHelp(
        makeTintString(
          "f7         f7f7         b ",
          "0                         ",
          "Instrument ReTrigger: --bb"),
        makeTintString(
          "7                         ",
          "0                         ",
          "retrigger & transpose by  "),
        makeTintString(
          "b 7                       ",
          "0                         ",
          "bb semitones              "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandPitchFineTune:
      return CommandHelp(
        makeTintString(
          "f7   f7  f7    a b 7      ",
          "0                         ",
          "PitchFineTune: aabb       "),
        makeTintString(
          "7               b 7       ",
          "0                         ",
          "Reach fine tune bb with   "),
        makeTintString(
          "7     a 7 8               ",
          "0                         ",
          "speed aa. Max ~1 semitone."),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandDelay:
      return CommandHelp(
        makeTintString(
          "f7f7f7    a7              ",
          "0                         ",
          "DeLaY: ---a               "),
        makeTintString(
          "7                   a7    ",
          "0                         ",
          "Delay this event by a + 1 "),
        makeTintString(
          "7                         ",
          "0                         ",
          "steps                     "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandSetInstrumentParameter:
      return CommandHelp(
        makeTintString(
          "f7  f7         f7         ",
          "0                         ",
          "Set Instrument Parameter  "),
        makeTintString(
          "a b 7               a 7   ",
          "0                         ",
          "aabb: set parameter aa to "),
        makeTintString(
          "7     b 7                 ",
          "0                         ",
          "value bb                  "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandStop:
      return CommandHelp(
        makeTintString(
          "f7   f7    f7             ",
          "0                         ",
          "Stop Table Playback ----  "),
        makeTintString(
          "7                         ",
          "0                         ",
          "Instantly stop playing the"),
        makeTintString(
          "7                         ",
          "0                         ",
          "current table.            "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandGateOff:
      return CommandHelp(
        makeTintString(
          "f7  f 7                   ",
          "0                         ",
          "GateOFf ----:             "),
        makeTintString(
          "7                         ",
          "0                         ",
          "Synth only!               "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandMidiChord:
      return CommandHelp(
        makeTintString(
          "f7   f 7    abcd7         ",
          "0                         ",
          "Midi CHord: abcd          "),
        makeTintString(
          "7                         ",
          "0                         ",
          "Play a chord with the rel-"),
        makeTintString(
          "7              a7 b7 c7 d7",
          "0                         ",
          "ative pitches +a,+b,+c,+d "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );

    case Token::InstrumentCommandVibrato:
      return CommandHelp(
        makeTintString(
          "f  7     a b 7            ",
          "0                         ",
          "VIBrato: aabb             "),
        makeTintString(
          "7                       b ",
          "0                         ",
          "Play a vibrato of depth bb"),
        makeTintString(
          "7       a 7               ",
          "0                         ",
          "at rate aa                "),
        makeTintString(
          "f                         ",
          "0                         ",
          "                          ")
      );      
  }
  
  return CommandHelp( 
    makeTintString(
        "9                         ",
        "0                         ",
        "This should never happen. "),
    makeTintString(
        "f                         ",
        "0                         ",
        "Please report this error  "),
    makeTintString(
        "f                         ",
        "0                         ",
        "code: 0xf00d              "),
    makeTintString(
        "7                         ",
        "0                         ",
        "                   Thanks!")
  );
}

// clang-format on