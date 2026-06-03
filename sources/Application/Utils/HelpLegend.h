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
  const char line1[29]; // 32 + '\0'
  const char line2[33]; // 28 + '\0'
};

#define LEGEND(N, A, B) static constexpr HelpLegend legend_##N = {A, B}; return legend_##N;

// CAUTION: all strings must fit in the line length limits!
// First line is max 32 - battery gauge (4) = 28
// Second line is max 32 chars
HelpLegend getHelpLegend(FourCC command) {
  switch (command) {
    case FourCC::InstrumentCommandKill:
      LEGEND(00, "KILl: --bb                  ", "stop playing after bb ticks     ")
    case FourCC::InstrumentCommandLoopOffset:
      LEGEND(01, "Loop OFset: aaaa            ", "Shift loop start & end by aaaa  ")
    case FourCC::InstrumentCommandArpeggiator:
      LEGEND(02, "ARPeggio: abcd              ", "Cycle thru relative pitch abcd  ")
    case FourCC::InstrumentCommandVolume:
      LEGEND(03, "VOLume: aabb                ", "reach volume bb at speed aa     ")
    case FourCC::InstrumentCommandVelocity:
      LEGEND(04, "VELocity: --bb              ", "send MIDI velocity cmd bb       ")
    case FourCC::InstrumentCommandPitchSlide:
      LEGEND(05, "Pitch SLide: aabb           ", "speed aa, slide to pitch bb     ")
    case FourCC::InstrumentCommandHop:
      LEGEND(06, "HOP: aabb                   ", "hop to bb aa times              ")
    case FourCC::InstrumentCommandLegato:
      LEGEND(07, "LEGato: aabb                ", "slide to pitch bb at speed aa   ")
    case FourCC::InstrumentCommandRetrigger:
      LEGEND(08, "ReTriGger: aabb             ", "SAMPL:bb loop+aa ofst, MIDI:bb  ")
    case FourCC::InstrumentCommandTempo:
      LEGEND(09, "TemPO: aabb                 ", "set tempo to hex value aabb     ")
    case FourCC::InstrumentCommandMidiCC:
      LEGEND(10, "Midi CC: aabb               ", "CC message aa value bb          ")
    case FourCC::InstrumentCommandMidiPC:
      LEGEND(11, "Midi Program Change: --bb   ", "send program change bb          ")
    case FourCC::InstrumentCommandPlayOfset:
      LEGEND(12, "Play OFfset: aabb           ", "jmp abs aa & mv rel signed bb   ")
    case FourCC::InstrumentCommandFilterResonance:
      LEGEND(13, "Filter & ReS: aabb          ", "speed aa, resonance bb          ")
    case FourCC::InstrumentCommandLowPassFilter:
      LEGEND(14, "FiLTer: aabb                ", "cutoff aa, resonance bb         ")
    case FourCC::InstrumentCommandTable:
      LEGEND(15, "TaBLe: --bb                 ", "run table bb                    ")
    case FourCC::InstrumentCommandCrush:
      LEGEND(16, "drive & CruSH: aa-b         ", "drive aa crush -b               ")
    case FourCC::InstrumentCommandFilterCut:
      LEGEND(17, "Filter CuToff: aabb         ", "speed aa, target cutoff bb      ");
    case FourCC::InstrumentCommandPan:;
      LEGEND(18, "PAN: aabb                   ", "speed aa, value bb (00 right)   ");
    case FourCC::InstrumentCommandGroove:
      LEGEND(19, "GRooVe: aabb                ", "set bb (aa > 0,set all tracks)  ");
    case FourCC::InstrumentCommandInstrumentRetrigger:
      LEGEND(20, "Instrument ReTrigger: --bb  ", "retrigger & transpose by bb     ");
    case FourCC::InstrumentCommandPitchFineTune:
      LEGEND(21, "PitchFineTune: aabb         ", "speed aa, tune bb (~+/-1 st)    ");
    case FourCC::InstrumentCommandDelay:
      LEGEND(22, "DeLaY: ---b                 ", "delay b+1 ticks                 ");
    case FourCC::InstrumentCommandSetInstrumentParameter:
      LEGEND(23, "Set Instrument Parameter    ", "aabb: set param aa to value bb  ");
    case FourCC::InstrumentCommandStop:
      LEGEND(24, "Stop Table Playback         ", "                                ");
    case FourCC::InstrumentCommandGateOff:
      LEGEND(25, "GateOFf: Synth only         ", "                                ");
    case FourCC::InstrumentCommandMidiChord:
      LEGEND(26, "Midi CHord:abcd             ", "send rel notes:+a,+b,+c,+d      ");
    case FourCC::InstrumentCommandVibrato:
      LEGEND(27, "VIBrato:aabb                ", "rate aa, depth bb               ");
  }
    
  LEGEND(28, "Unknown Command: This should", "not happen. Report bug #1334. ");
}
