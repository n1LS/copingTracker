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

#ifndef _APP_TYPES_H_
#define _APP_TYPES_H_

#include "Externals/etl/include/etl/enum_type.h"
#include <stdint.h>

enum TextCase { tcRegular, tcUpper, tcLower, Count };

struct Token {
  // While the names of the Token codes can be changed, their values CANNOT.
  // Values are used as is in save files, so any changes would cause save files
  // to break.
  enum enum_type {
    InstrumentCommandArpeggiator = 0,              // ARP
    InstrumentCommandCrush = 2,                    // CSH
    InstrumentCommandDelay = 4,                    // DLY
    InstrumentCommandFilterCut = 20,               // FCT
    InstrumentCommandLowPassFilter = 22,           // FLT
    InstrumentCommandFilterResonance = 25,         // FRS
    InstrumentCommandGateOff = 92,                 // GOF
    InstrumentCommandGroove = 26,                  // GRV
    InstrumentCommandHop = 27,                     // HOP
    InstrumentCommandRetrigger = 52,               // RTR
    InstrumentCommandInstrumentRetrigger = 29,     // IRT
    InstrumentCommandKill = 30,                    // KIL
    InstrumentCommandLegato = 31,                  // LEG
    InstrumentCommandLoopOffset = 36,              // LOF
    InstrumentCommandMidiCC = 38,                  // MCC
    InstrumentCommandMidiPC = 39,                  // MPG
    InstrumentCommandPan = 42,                     // PAN
    InstrumentCommandPitchFineTune = 44,           // PFT
    InstrumentCommandPlayOfset = 46,               // POF
    InstrumentCommandPitchSlide = 48,              // PSL
    InstrumentCommandSetInstrumentParameter = 188, // SIP
    InstrumentCommandStop = 55,                    // STP
    InstrumentCommandTable = 58,                   // TBL
    InstrumentCommandTempo = 62,                   // TPO
    InstrumentCommandVelocity = 66,                // VEL
    InstrumentCommandVolume = 69,                  // VOL
    InstrumentCommandVibrato = 73,                 // VIB
    InstrumentCommandNone = 45,                    // ---
    InstrumentCommandChordUp = 143,                // CHU
    InstrumentCommandChordDown = 98,               // CHD
    InstrumentCommandChordBidirectional = 229,     // CHB

    InstrumentParameterVolume = 19,
    InstrumentParameterPan = 43,
    InstrumentParameterTable = 117,
    InstrumentParameterTableAutomation = 60,

    SampleInstrumentCrushVolume = 3,
    SampleInstrumentCrush = 114,
    SampleInstrumentSample = 54,
    SampleInstrumentInterpolation = 28,
    SampleInstrumentDownsample = 5,
    SampleInstrumentRootNote = 51,
    SampleInstrumentFineTune = 24,
    SampleInstrumentFilterCutOff = 115,
    SampleInstrumentFilterResonance = 116,
    SampleInstrumentFilterType = 23,
    SampleInstrumentFilterMode = 21,
    SampleInstrumentStart = 56,
    SampleInstrumentLoopMode = 34,
    SampleInstrumentLoopStart = 37,
    SampleInstrumentEnd = 6,
    SampleInstrumentGMInstrument = 191, // GM bank instrument index (0..kGMInstrumentCount-1), -1 = off
    SampleInstrumentAttack = 93,
    SampleInstrumentDecay = 94,
    SampleInstrumentSustain = 95,
    SampleInstrumentRelease = 96,

    MidiInstrumentChannel = 1,
    MidiInstrumentNoteLength = 32,
    MidiInstrumentTableAutomation = 120,
    MidiInstrumentName = 144,
    MidiInstrumentProgram = 160,

    ServicePersistency = 57,

    TrigTempoTap = 65,
    TrigSeqQueueRow = 64,
    TrigVolumeIncrease = 68,
    TrigVolumeDecrease = 67,
    TrigEventEnter = 7,
    TrigEventEdit = 8,
    TrigEventLeft = 10,
    TrigEventRight = 13,
    TrigEventUp = 15,
    TrigEventDown = 9,
    TrigEventAlt = 11,
    TrigEventNav = 12,
    TrigEventPlay = 14,

    VarTempo = 33,
    VarMasterVolume = 41,
    VarPreviewVolume = 161,
    VarWrap = 70,
    VarTranspose = 63,
    VarScale = 16,
    VarScaleRoot = 162,
    VarProjectName = 99,
    VarMidiDevice = 40,
    VarLineOut = 17,
    // colors
    VarColor_0 = 104,
    VarColor_1 = 105,
    VarColor_2 = 106,
    VarColor_3 = 111,
    VarColor_4 = 152,
    VarColor_5 = 153,
    VarColor_6 = 154,
    VarColor_7 = 155,
    VarColor_8 = 107,
    VarColor_9 = 110,
    VarColor_A = 156,
    VarColor_B = 108,
    VarColor_C = 157,
    VarColor_D = 158,
    VarColor_E = 109,
    VarColor_F = 103,

    VarMidiSync = 112,
    VarMidiClockSync = 151,
    VarMirrorUI = 140,
    VarUIFont = 141,
    VarTextCase = 118,

    VarChannel1Volume = 163,
    VarChannel2Volume = 164,
    VarChannel3Volume = 165,
    VarChannel4Volume = 166,
    VarChannel5Volume = 167,
    VarChannel6Volume = 168,
    VarChannel7Volume = 169,
    VarChannel8Volume = 170,
    VarThemeName = 173, // Variable for storing the current theme name

    VarInstrumentType = 113,
    ActionBPMChanged = 61,
    ActionPurge = 49,
    ActionPurgeInstrument = 47,
    ActionProjectRename = 102,
    ActionBrowse = 35,
    ActionSave = 53,
    ActionLoadAndSave = 179,
    ActionCancel = 180,
    ActionNewProject = 101,
    ActionRandomName = 100,
    ActionBootSelect = 18,
    ActionEdit = 59,
    ActionExport = 145,
    ActionImport = 146,
    ActionOK = 147,
    InstrumentName = 148,
    ActionRenderMixdown = 149,
    ActionRenderStems = 150,
    ActionShowTheme = 159,
    ActionThemeName = 172,
    SampleInstrumentSlices = 171,
    VarBacklightLevel = 174,
    ActionShowSampleEditor = 175,
    ActionShowSampleSlices = 184,
    VarSampleEditStart = 177,
    VarSampleEditEnd = 178,
    VarSampleEditOperation = 181,
    VarImportResampler = 185,
    VarConfigCommandPicker = 190,

    ActionAutoSlice = 186,
    ActionSlicingRevert = 187,
    ActionSlicingSave = 189,

    ChiptuneInstrumentWaveform = 200,
    ChiptuneInstrumentAttack = 201,
    ChiptuneInstrumentDecay = 202,
    ChiptuneInstrumentLength = 204,
    ChiptuneInstrumentBurst = 205,
    ChiptuneInstrumentVibrato = 206,
    ChiptuneInstrumentVibratoDelay = 207,
    ChiptuneInstrumentTranspose = 208,
    ChiptuneInstrumentSweepTime = 210,
    ChiptuneInstrumentSweepAmount = 211,
    ChiptuneInstrumentArpSpeed = 212,

    DrumInstrumentParamsVoice0 = 214,
    DrumInstrumentParamsVoice1 = 215,
    DrumInstrumentParamsVoice2 = 216,
    DrumInstrumentParamsVoice3 = 217,
    DrumInstrumentParamsVoice4 = 218,
    DrumInstrumentParamsVoice5 = 219,
    DrumInstrumentParamsVoice6 = 220,
    DrumInstrumentParamsVoice7 = 221,
    DrumInstrumentParamsVoice8 = 222,
    DrumInstrumentParamsVoice9 = 223,
    DrumInstrumentParamsVoice10 = 224,
    DrumInstrumentParamsVoice11 = 225,
    DrumInstrumentParamsCharacter = 226,

    ActionMassStorage = 50,
    VarOutputVolume = 74,
    ActionModulation = 213,
    ActionDelete = 182,

    VarKeyDelay = 139,
    VarKeyRepeat = 176,

    StackInstrumentSpread = 97,
    StackInstrumentWave = 183,
    StackInstrumentTranspose = 192,
    StackInstrumentAttack = 195,
    StackInstrumentDecay = 196,
    StackInstrumentSustain = 197,
    StackInstrumentRelease = 198,
    StackInstrumentBrightness = 227,
    StackInstrumentGlide = 228,
    StackInstrumentChord = 230,

    VarPhraseLength = 254,

    // 119 free      1
    // 121-122 free  2
    // 193-194 free  2
    // 199 free      1
    // 203 free      1
    // 209 free      1
    // 230-253 free 24
    // ----------------
    //               33

    Default = 255, // "    "
  };

  uint16_t raw() {
    return static_cast<uint16_t>(*this);
  }

  uint8_t raw8() {
    // this is not lossless and must be used with caution
    return static_cast<uint8_t>(*this);
  }

#define ETL_ENUM_TYPE_16(value, name)                                                                                  \
  static_assert(sizeof(name) <= 17, "ETL_ENUM_TYPE string \"" name "\" exceeds 16 characters");                        \
  ETL_ENUM_TYPE(value, name)

  ETL_DECLARE_ENUM_TYPE(Token, uint8_t)

  // Not all enums need reflection. Only cases where we need reflection is the
  // Token codes that need to be converted to text in order to display on
  // screen
  ETL_ENUM_TYPE_16(InstrumentCommandArpeggiator, "Arp")
  ETL_ENUM_TYPE_16(InstrumentCommandCrush, "Csh")
  ETL_ENUM_TYPE_16(InstrumentCommandKill, "Kil")
  ETL_ENUM_TYPE_16(InstrumentCommandLoopOffset, "LOf")
  ETL_ENUM_TYPE_16(InstrumentCommandVelocity, "Vel")
  ETL_ENUM_TYPE_16(InstrumentCommandVolume, "Vol")
  ETL_ENUM_TYPE_16(InstrumentCommandPitchSlide, "PSl")
  ETL_ENUM_TYPE_16(InstrumentCommandHop, "Hop")
  ETL_ENUM_TYPE_16(InstrumentCommandLegato, "Leg")
  ETL_ENUM_TYPE_16(InstrumentCommandRetrigger, "Rtg")
  ETL_ENUM_TYPE_16(InstrumentCommandTempo, "Tpo")
  ETL_ENUM_TYPE_16(InstrumentCommandMidiCC, "MCC")
  ETL_ENUM_TYPE_16(InstrumentCommandMidiPC, "MPC")
  ETL_ENUM_TYPE_16(InstrumentCommandPlayOfset, "POf")
  ETL_ENUM_TYPE_16(InstrumentCommandLowPassFilter, "Flt")
  ETL_ENUM_TYPE_16(InstrumentCommandTable, "Tbl")
  ETL_ENUM_TYPE_16(InstrumentCommandFilterCut, "FCt")
  ETL_ENUM_TYPE_16(InstrumentCommandFilterResonance, "FRs")
  ETL_ENUM_TYPE_16(InstrumentCommandPan, "Pan")
  ETL_ENUM_TYPE_16(InstrumentCommandGateOff, "GOf")
  ETL_ENUM_TYPE_16(InstrumentCommandGroove, "Grv")
  ETL_ENUM_TYPE_16(InstrumentCommandSetInstrumentParameter, "SIP")
  ETL_ENUM_TYPE_16(InstrumentCommandStop, "Stp")
  ETL_ENUM_TYPE_16(InstrumentCommandNone, "---")
  ETL_ENUM_TYPE_16(InstrumentCommandPitchFineTune, "PFt")
  ETL_ENUM_TYPE_16(InstrumentCommandDelay, "Dly")
  ETL_ENUM_TYPE_16(InstrumentCommandInstrumentRetrigger, "IRt")
  ETL_ENUM_TYPE_16(InstrumentCommandChordUp, "ChU")
  ETL_ENUM_TYPE_16(InstrumentCommandChordDown, "ChD")
  ETL_ENUM_TYPE_16(InstrumentCommandChordBidirectional, "ChB")
  ETL_ENUM_TYPE_16(InstrumentCommandVibrato, "Vib")

  ETL_ENUM_TYPE_16(InstrumentParameterVolume, "Volume")
  ETL_ENUM_TYPE_16(InstrumentParameterPan, "Pan")
  ETL_ENUM_TYPE_16(InstrumentParameterTable, "Table")
  ETL_ENUM_TYPE_16(InstrumentParameterTableAutomation, "Automate")

  ETL_ENUM_TYPE_16(VarKeyDelay, "key-delay")
  ETL_ENUM_TYPE_16(VarKeyRepeat, "key-repeat")
  ETL_ENUM_TYPE_16(VarLineOut, "line-out")
  ETL_ENUM_TYPE_16(VarMidiDevice, "midi-device")
  ETL_ENUM_TYPE_16(VarMidiSync, "midi-sync")
  ETL_ENUM_TYPE_16(VarMidiClockSync, "midi-clock-sync")
  ETL_ENUM_TYPE_16(VarMirrorUI, "mirror-ui")
  ETL_ENUM_TYPE_16(VarUIFont, "ui-font")
  ETL_ENUM_TYPE_16(VarTextCase, "text-case")
  ETL_ENUM_TYPE_16(VarThemeName, "theme-name")
  ETL_ENUM_TYPE_16(VarScaleRoot, "scale-root")
  ETL_ENUM_TYPE_16(VarPhraseLength, "phrase-length")
  ETL_ENUM_TYPE_16(SampleInstrumentSample, "Sample")
  ETL_ENUM_TYPE_16(SampleInstrumentInterpolation, "Interpolation")
  ETL_ENUM_TYPE_16(SampleInstrumentCrush, "Crush")
  ETL_ENUM_TYPE_16(SampleInstrumentCrushVolume, "CrushDrive")
  ETL_ENUM_TYPE_16(SampleInstrumentDownsample, "Downsample")
  ETL_ENUM_TYPE_16(SampleInstrumentRootNote, "RootNote")
  ETL_ENUM_TYPE_16(SampleInstrumentFineTune, "Finetune")
  ETL_ENUM_TYPE_16(SampleInstrumentFilterCutOff, "FilterCutoff")
  ETL_ENUM_TYPE_16(SampleInstrumentFilterResonance, "FilterResonance")
  ETL_ENUM_TYPE_16(SampleInstrumentFilterType, "FilterType")
  ETL_ENUM_TYPE_16(SampleInstrumentFilterMode, "FilterMode")
  ETL_ENUM_TYPE_16(SampleInstrumentStart, "Start")
  ETL_ENUM_TYPE_16(SampleInstrumentLoopMode, "LoopMode")
  ETL_ENUM_TYPE_16(SampleInstrumentLoopStart, "LoopStart")
  ETL_ENUM_TYPE_16(SampleInstrumentEnd, "End")
  ETL_ENUM_TYPE_16(SampleInstrumentGMInstrument, "GMInstrument")
  ETL_ENUM_TYPE_16(SampleInstrumentAttack, "Attack")
  ETL_ENUM_TYPE_16(SampleInstrumentDecay, "Decay")
  ETL_ENUM_TYPE_16(SampleInstrumentSustain, "Sustain")
  ETL_ENUM_TYPE_16(SampleInstrumentRelease, "Release")

  ETL_ENUM_TYPE_16(MidiInstrumentChannel, "Channel")

  ETL_ENUM_TYPE_16(InstrumentName, "Name")

  ETL_ENUM_TYPE_16(MidiInstrumentName, "MidiName")
  ETL_ENUM_TYPE_16(MidiInstrumentNoteLength, "NoteLength")
  ETL_ENUM_TYPE_16(MidiInstrumentProgram, "Program")

  ETL_ENUM_TYPE_16(StackInstrumentSpread, "Spread")
  ETL_ENUM_TYPE_16(StackInstrumentWave, "Wave")
  ETL_ENUM_TYPE_16(StackInstrumentTranspose, "Transpose")
  ETL_ENUM_TYPE_16(StackInstrumentAttack, "Attack")
  ETL_ENUM_TYPE_16(StackInstrumentDecay, "Decay")
  ETL_ENUM_TYPE_16(StackInstrumentSustain, "Sustain")
  ETL_ENUM_TYPE_16(StackInstrumentRelease, "Release")
  ETL_ENUM_TYPE_16(StackInstrumentBrightness, "Brightness")
  ETL_ENUM_TYPE_16(StackInstrumentGlide, "Glide")
  ETL_ENUM_TYPE_16(StackInstrumentChord, "Chord")

  ETL_ENUM_TYPE_16(VarColor_0, "color0")
  ETL_ENUM_TYPE_16(VarColor_1, "color1")
  ETL_ENUM_TYPE_16(VarColor_2, "color2")
  ETL_ENUM_TYPE_16(VarColor_3, "color3")
  ETL_ENUM_TYPE_16(VarColor_4, "color4")
  ETL_ENUM_TYPE_16(VarColor_5, "color5")
  ETL_ENUM_TYPE_16(VarColor_6, "color6")
  ETL_ENUM_TYPE_16(VarColor_7, "color7")
  ETL_ENUM_TYPE_16(VarColor_8, "color8")
  ETL_ENUM_TYPE_16(VarColor_9, "color9")
  ETL_ENUM_TYPE_16(VarColor_A, "color10")
  ETL_ENUM_TYPE_16(VarColor_B, "color11")
  ETL_ENUM_TYPE_16(VarColor_C, "color12")
  ETL_ENUM_TYPE_16(VarColor_D, "color13")
  ETL_ENUM_TYPE_16(VarColor_E, "color14")
  ETL_ENUM_TYPE_16(VarColor_F, "color15")

  ETL_ENUM_TYPE_16(VarTempo, "Tempo")
  ETL_ENUM_TYPE_16(VarMasterVolume, "Master")
  ETL_ENUM_TYPE_16(VarPreviewVolume, "Preview")
  ETL_ENUM_TYPE_16(VarWrap, "Wrap")
  ETL_ENUM_TYPE_16(VarTranspose, "Transpose")
  ETL_ENUM_TYPE_16(VarScale, "Scale")
  ETL_ENUM_TYPE_16(VarProjectName, "ProjectName")
  ETL_ENUM_TYPE_16(VarInstrumentType, "InstrumentType")
  ETL_ENUM_TYPE_16(VarChannel1Volume, "channel1vol")
  ETL_ENUM_TYPE_16(VarChannel2Volume, "channel2vol")
  ETL_ENUM_TYPE_16(VarChannel3Volume, "channel3vol")
  ETL_ENUM_TYPE_16(VarChannel4Volume, "channel4vol")
  ETL_ENUM_TYPE_16(VarChannel5Volume, "channel5vol")
  ETL_ENUM_TYPE_16(VarChannel6Volume, "channel6vol")
  ETL_ENUM_TYPE_16(VarChannel7Volume, "channel7vol")
  ETL_ENUM_TYPE_16(VarChannel8Volume, "channel8vol")

  ETL_ENUM_TYPE_16(ActionEdit, "Edit")
  ETL_ENUM_TYPE_16(ActionExport, "Export")
  ETL_ENUM_TYPE_16(ActionImport, "Import")
  ETL_ENUM_TYPE_16(ActionThemeName, "theme-name")
  ETL_ENUM_TYPE_16(VarBacklightLevel, "backlight-level")
  ETL_ENUM_TYPE_16(VarOutputVolume, "output-volume")
  ETL_ENUM_TYPE_16(VarImportResampler, "import-resampler")
  ETL_ENUM_TYPE_16(VarConfigCommandPicker, "command-picker")

  // Chiptune Instrument Variables
  ETL_ENUM_TYPE_16(ChiptuneInstrumentWaveform, "Waveform")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentAttack, "Attack")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentDecay, "Decay")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentLength, "Length")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentBurst, "Burst")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentVibrato, "Vibrato")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentVibratoDelay, "VibratoDelay")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentTranspose, "Transpose")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentSweepTime, "SweepTime")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentSweepAmount, "SweepAmount")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentArpSpeed, "ArpSpeed")

  // Drum Instrument Variables
  ETL_ENUM_TYPE_16(DrumInstrumentParamsVoice0, "DrumInstument0")
  ETL_ENUM_TYPE_16(DrumInstrumentParamsVoice1, "DrumInstument1")
  ETL_ENUM_TYPE_16(DrumInstrumentParamsVoice2, "DrumInstument2")
  ETL_ENUM_TYPE_16(DrumInstrumentParamsVoice3, "DrumInstument3")
  ETL_ENUM_TYPE_16(DrumInstrumentParamsVoice4, "DrumInstument4")
  ETL_ENUM_TYPE_16(DrumInstrumentParamsVoice5, "DrumInstument5")
  ETL_ENUM_TYPE_16(DrumInstrumentParamsVoice6, "DrumInstument6")
  ETL_ENUM_TYPE_16(DrumInstrumentParamsVoice7, "DrumInstument7")
  ETL_ENUM_TYPE_16(DrumInstrumentParamsVoice8, "DrumInstument8")
  ETL_ENUM_TYPE_16(DrumInstrumentParamsVoice9, "DrumInstument9")
  ETL_ENUM_TYPE_16(DrumInstrumentParamsVoice10, "DrumInstument10")
  ETL_ENUM_TYPE_16(DrumInstrumentParamsVoice11, "DrumInstument11")
  ETL_ENUM_TYPE_16(DrumInstrumentParamsCharacter, "DrumCharacter")

  ETL_ENUM_TYPE_16(Default, "Default")
  ETL_END_ENUM_TYPE
};

typedef uint32_t stereosample;

#endif
