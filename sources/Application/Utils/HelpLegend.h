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
#include "Application/Utils/TintChar.h"

struct HelpLegend {
    etl::array<TintChar, 27> line1;
    etl::array<TintChar, 27> line2;
    etl::array<TintChar, 27> line3;
    etl::array<TintChar, 27> line4;
};

#define LEGEND(N, A, B, C, D)                                                                                          \
  static constexpr HelpLegend legend_##N = {A, B, C, D};                                                               \
  return legend_##N;

// clang-format off

// CAUTION: all strings must fit in the line length limits!
// Second line is max 26 chars
HelpLegend getHelpLegend(FourCC command) {
  switch (command) {
    case FourCC::InstrumentCommandKill: 
      return HelpLegend( 
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
    case FourCC::InstrumentCommandArpeggiator:
      return HelpLegend(
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

    case FourCC::InstrumentCommandVolume:
      return HelpLegend(
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

    case FourCC::InstrumentCommandVelocity:
      return HelpLegend(
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

    case FourCC::InstrumentCommandPitchSlide:
      return HelpLegend(
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

    case FourCC::InstrumentCommandHop:
      return HelpLegend(
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

    case FourCC::InstrumentCommandLegato:
      return HelpLegend(
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

    case FourCC::InstrumentCommandRetrigger:
      return HelpLegend(
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

    case FourCC::InstrumentCommandLoopOffset:
      return HelpLegend(
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

    case FourCC::InstrumentCommandTempo:
      return HelpLegend(
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

    case FourCC::InstrumentCommandMidiCC:
      return HelpLegend(
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

    case FourCC::InstrumentCommandMidiPC:
      return HelpLegend(
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

    case FourCC::InstrumentCommandPlayOfset:
      return HelpLegend(
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

    case FourCC::InstrumentCommandFilterResonance:
      return HelpLegend(
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

    case FourCC::InstrumentCommandLowPassFilter:
      return HelpLegend(
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

    case FourCC::InstrumentCommandTable:
      return HelpLegend(
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

    case FourCC::InstrumentCommandCrush:
      return HelpLegend(
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

    case FourCC::InstrumentCommandFilterCut:
      return HelpLegend(
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

    case FourCC::InstrumentCommandPan:
      return HelpLegend(
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

    case FourCC::InstrumentCommandGroove:
      return HelpLegend(
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

    case FourCC::InstrumentCommandInstrumentRetrigger:
      return HelpLegend(
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

    case FourCC::InstrumentCommandPitchFineTune:
      return HelpLegend(
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

    case FourCC::InstrumentCommandDelay:
      return HelpLegend(
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

    case FourCC::InstrumentCommandSetInstrumentParameter:
      return HelpLegend(
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

    case FourCC::InstrumentCommandStop:
      return HelpLegend(
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

    case FourCC::InstrumentCommandGateOff:
      return HelpLegend(
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

    case FourCC::InstrumentCommandMidiChord:
      return HelpLegend(
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

    case FourCC::InstrumentCommandVibrato:
      return HelpLegend(
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
  
  return HelpLegend( 
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