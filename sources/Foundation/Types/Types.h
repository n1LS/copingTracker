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

struct FourCC {
  // While the names of the FourCC codes can be changed, their values CANNOT.
  // Values are used as is in save files, so any changes would cause save files
  // to break.
  enum enum_type {
    InstrumentCommandArpeggiator = 0,              // ARP
    InstrumentCommandCrush = 2,                    // CSH
    InstrumentCommandDelay = 4,                    // DLY
    InstrumentCommandFilterCut = 20,               // FCT
    InstrumentCommandLowPassFilter = 22,           // FLT
    InstrumentCommandFilterResonance = 25,         // FRES
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
    InstrumentCommandMidiChord = 143,              // MCH

    SampleInstrumentCrushVolume = 3,
    SampleInstrumentVolume = 19,
    SampleInstrumentCrush = 114,
    SampleInstrumentSample = 54,
    SampleInstrumentInterpolation = 28,
    SampleInstrumentDownsample = 5,
    SampleInstrumentRootNote = 51,
    SampleInstrumentFineTune = 24,
    SampleInstrumentPan = 43,
    SampleInstrumentFilterCutOff = 115,
    SampleInstrumentFilterResonance = 116,
    SampleInstrumentFilterType = 23,
    SampleInstrumentFilterMode = 21,
    SampleInstrumentStart = 56,
    SampleInstrumentLoopMode = 34,
    SampleInstrumentLoopStart = 37,
    SampleInstrumentEnd = 6,
    SampleInstrumentTable = 117,
    SampleInstrumentTableAutomation = 60,

    MidiInstrumentChannel = 1,
    MidiInstrumentNoteLength = 32,
    MidiInstrumentVolume = 118,
    MidiInstrumentTable = 119,
    MidiInstrumentTableAutomation = 120,
    MidiInstrumentName = 144,
    MidiInstrumentProgram = 160,

    SIDInstrumentWaveform = 72,
    SIDInstrument1FilterCut = 79,
    SIDInstrument2FilterCut = 83,
    SIDInstrument3FilterCut = 87,
    SIDInstrument1FilterResonance = 80,
    SIDInstrument2FilterResonance = 84,
    SIDInstrument3FilterResonance = 88,
    SIDInstrument1FilterMode = 81,
    SIDInstrument2FilterMode = 85,
    SIDInstrument3FilterMode = 89,
    SIDInstrument1Volume = 82,
    SIDInstrument2Volume = 86,
    SIDInstrument3Volume = 90,
    SIDInstrumentPulseWidth = 71,
    SIDInstrumentVSync = 75,
    SIDInstrumentRingModulator = 76,
    SIDInstrumentADSR = 77,
    SIDInstrumentFilterOn = 78,
    SIDInstrumentVoice3Off = 91,
    SIDInstrumentTable = 121,
    SIDInstrumentTableAutomation = 122,
    SIDInstrumentOSCNumber = 142,

    OPALInstrumentChannel = 123,
    OPALInstrumentAlgorithm = 124,
    OPALInstrumentFeedback = 125,
    OPALInstrumentDeepTremeloVibrato = 126,

    OPALInstrumentOp1Level = 127,
    OPALInstrumentOp1Multiplier = 128,
    OPALInstrumentOp1KeyScaleLevel = 130,
    OPALInstrumentOp1ADSR = 131,
    OPALInstrumentOp1WaveShape = 132,
    OPALInstrumentOp1TremVibSusKSR = 133,

    OPALInstrumentOp2Level = 134,
    OPALInstrumentOp2Multiplier = 135,
    OPALInstrumentOp2KeyScaleLevel = 136,
    OPALInstrumentOp2ADSR = 137,
    OPALInstrumentOp2WaveShape = 138,
    OPALInstrumentOp2TremVibSusKSR = 139,

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

    // 142 is taken for SIDInstrumentOSCNumber
    // 143 is taken for InstrumentCommandMidiChord
    // 144 is taken for InstrumentMidiName
    // 145 is taken for ActionExport
    // 146 is taken for ActionImport
    // 147 is taken for ActionOK
    // 148 is taken for InstrumentName
    // 149 is taken for ActionRenderMixdown
    // 150 is taken for ActionRenderStems
    // 151 is taken for VarMidiClockSync
    // 152 is taken for VarPlayColor
    // 153 is taken for VarMuteColor
    // 154 is taken for VarSongViewFEColor
    // 155 is taken for VarSongView00Color
    // 156 is taken for VarRowColor
    // 157 is taken for VarRow2Color
    // 158 is taken for VarMajorBeatColor
    // 159 is taken for ActionShowTheme
    // 160 is taken for MidiInstrumentProgram
    // 161 is taken for VarScaleRoot
    // 162 is taken for VarPreviewVolume
    // 163 is taken for VarChannel1Volume
    // 164 is taken for VarChannel2Volume
    // 165 is taken for VarChannel3Volume
    // 166 is taken for VarChannel4Volume
    // 167 is taken for VarChannel5Volume
    // 168 is taken for VarChannel6Volume
    // 169 is taken for VarChannel7Volume
    // 170 is taken for VarChannel8Volume
    // 171 is taken for SampleInstrumentSlices
    // 172 is taken for ActionThemeName
    // 173 is taken for VarThemeName
    // 174 is taken for VarBacklightLevel
    // 175 is taken for ActionShowSampleEditor
    // 177 is taken for VarSampleEditStart
    // 178 is taken for VarSampleEditStop
    // 179 is taken for ActionLoadAndSave
    // 180 is taken for ActionCancel
    // 181 is taken for VarSampleEditOperation
    // 184 is taken for ActionShowSampleSlices
    // 185 is taken for VarImportResampler
    // 186 is taken for ActionAutoSlice

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
    ChiptuneInstrumentLevel = 203,
    ChiptuneInstrumentLength = 204,
    ChiptuneInstrumentBurst = 205,
    ChiptuneInstrumentVibrato = 206,
    ChiptuneInstrumentVibratoDelay = 207,
    ChiptuneInstrumentTranspose = 208,
    ChiptuneInstrumentTable = 209,
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

    ActionMassStorage = 50,
    VarOutputVolume = 74,
    ActionModulation = 213,

    // 93-98 free (formerly MacroInstrument fields)
    // 176 is free
    // 182 is free
    // 183 is free

    Default = 255, // "    "
  };

  uint8_t raw() {
    return static_cast<uint8_t>(*this);
  }

#define ETL_ENUM_TYPE_16(value, name)                                                                                  \
  static_assert(sizeof(name) <= 17, "ETL_ENUM_TYPE string \"" name "\" exceeds 16 characters");                        \
  ETL_ENUM_TYPE(value, name)

  ETL_DECLARE_ENUM_TYPE(FourCC, uint8_t)

  // Not all enums need reflection. Only cases where we need reflection is the
  // FourCC codes that need to be converted to text in order to display on
  // screen
  ETL_ENUM_TYPE_16(InstrumentCommandArpeggiator, "ARP")
  ETL_ENUM_TYPE_16(InstrumentCommandCrush, "CSH")
  ETL_ENUM_TYPE_16(InstrumentCommandKill, "KIL")
  ETL_ENUM_TYPE_16(InstrumentCommandLoopOffset, "LOF")
  ETL_ENUM_TYPE_16(InstrumentCommandVelocity, "VEL")
  ETL_ENUM_TYPE_16(InstrumentCommandVolume, "VOL")
  ETL_ENUM_TYPE_16(InstrumentCommandPitchSlide, "PSL")
  ETL_ENUM_TYPE_16(InstrumentCommandHop, "HOP")
  ETL_ENUM_TYPE_16(InstrumentCommandLegato, "LEG")
  ETL_ENUM_TYPE_16(InstrumentCommandRetrigger, "RTG")
  ETL_ENUM_TYPE_16(InstrumentCommandTempo, "TPO")
  ETL_ENUM_TYPE_16(InstrumentCommandMidiCC, "MCC")
  ETL_ENUM_TYPE_16(InstrumentCommandMidiPC, "MPC")
  ETL_ENUM_TYPE_16(InstrumentCommandPlayOfset, "POF")
  ETL_ENUM_TYPE_16(InstrumentCommandLowPassFilter, "FLT")
  ETL_ENUM_TYPE_16(InstrumentCommandTable, "TBL")
  ETL_ENUM_TYPE_16(InstrumentCommandFilterCut, "FCT")
  ETL_ENUM_TYPE_16(InstrumentCommandFilterResonance, "FRS")
  ETL_ENUM_TYPE_16(InstrumentCommandPan, "PAN")
  ETL_ENUM_TYPE_16(InstrumentCommandGateOff, "GOF")
  ETL_ENUM_TYPE_16(InstrumentCommandGroove, "GRV")
  ETL_ENUM_TYPE_16(InstrumentCommandSetInstrumentParameter, "SIP")
  ETL_ENUM_TYPE_16(InstrumentCommandStop, "STP")
  ETL_ENUM_TYPE_16(InstrumentCommandNone, "---")
  ETL_ENUM_TYPE_16(InstrumentCommandPitchFineTune, "PFT")
  ETL_ENUM_TYPE_16(InstrumentCommandDelay, "DLY")
  ETL_ENUM_TYPE_16(InstrumentCommandInstrumentRetrigger, "IRT")
  ETL_ENUM_TYPE_16(InstrumentCommandMidiChord, "MCH")
  ETL_ENUM_TYPE_16(InstrumentCommandVibrato, "VIB")

  ETL_ENUM_TYPE_16(VarLineOut, "line-out")
  ETL_ENUM_TYPE_16(VarMidiDevice, "midi-device")
  ETL_ENUM_TYPE_16(VarMidiSync, "midi-sync")
  ETL_ENUM_TYPE_16(VarMidiClockSync, "midi-clock-sync")
  ETL_ENUM_TYPE_16(VarMirrorUI, "mirror-ui")
  ETL_ENUM_TYPE_16(VarUIFont, "ui-font")
  ETL_ENUM_TYPE_16(VarThemeName, "theme-name")
  ETL_ENUM_TYPE_16(VarScaleRoot, "scale-root")
  ETL_ENUM_TYPE_16(SampleInstrumentSample, "Sample")
  ETL_ENUM_TYPE_16(SampleInstrumentVolume, "Volume")
  ETL_ENUM_TYPE_16(SampleInstrumentInterpolation, "Interpolation")
  ETL_ENUM_TYPE_16(SampleInstrumentCrush, "Crush")
  ETL_ENUM_TYPE_16(SampleInstrumentCrushVolume, "CrushDrive")
  ETL_ENUM_TYPE_16(SampleInstrumentDownsample, "Downsample")
  ETL_ENUM_TYPE_16(SampleInstrumentRootNote, "RootNote")
  ETL_ENUM_TYPE_16(SampleInstrumentFineTune, "Finetune")
  ETL_ENUM_TYPE_16(SampleInstrumentPan, "Pan")
  ETL_ENUM_TYPE_16(SampleInstrumentFilterCutOff, "FilterCutoff")
  ETL_ENUM_TYPE_16(SampleInstrumentFilterResonance, "FilterResonance")
  ETL_ENUM_TYPE_16(SampleInstrumentFilterType, "FilterType")
  ETL_ENUM_TYPE_16(SampleInstrumentFilterMode, "FilterMode")
  ETL_ENUM_TYPE_16(SampleInstrumentStart, "Start")
  ETL_ENUM_TYPE_16(SampleInstrumentLoopMode, "LoopMode")
  ETL_ENUM_TYPE_16(SampleInstrumentLoopStart, "LoopStart")
  ETL_ENUM_TYPE_16(SampleInstrumentEnd, "End")
  ETL_ENUM_TYPE_16(SampleInstrumentTable, "Table")
  ETL_ENUM_TYPE_16(SampleInstrumentTableAutomation, "TableAutomation")
  ETL_ENUM_TYPE_16(MidiInstrumentChannel, "Channel")
  ETL_ENUM_TYPE_16(InstrumentName, "Name")
  ETL_ENUM_TYPE_16(MidiInstrumentName, "MidiName")
  ETL_ENUM_TYPE_16(MidiInstrumentNoteLength, "NoteLength")
  ETL_ENUM_TYPE_16(MidiInstrumentVolume, "Volume")
  ETL_ENUM_TYPE_16(MidiInstrumentTable, "Table")
  ETL_ENUM_TYPE_16(MidiInstrumentTableAutomation, "TableAutomation")
  ETL_ENUM_TYPE_16(MidiInstrumentProgram, "Program")
  ETL_ENUM_TYPE_16(SIDInstrumentWaveform, "OscWaveform")
  ETL_ENUM_TYPE_16(SIDInstrument1FilterCut, "FilterCutoff1")
  ETL_ENUM_TYPE_16(SIDInstrument1FilterResonance, "FilterResonance1")
  ETL_ENUM_TYPE_16(SIDInstrument1FilterMode, "FilterMode1")
  ETL_ENUM_TYPE_16(SIDInstrument1Volume, "Volume1")
  ETL_ENUM_TYPE_16(SIDInstrument2FilterCut, "FilterCutoff2")
  ETL_ENUM_TYPE_16(SIDInstrument2FilterResonance, "FilterResonance2")
  ETL_ENUM_TYPE_16(SIDInstrument2FilterMode, "FilterMode2")
  ETL_ENUM_TYPE_16(SIDInstrument2Volume, "Volume2")
  ETL_ENUM_TYPE_16(SIDInstrumentPulseWidth, "OscPulseWidth")
  ETL_ENUM_TYPE_16(SIDInstrumentVSync, "OscSync")
  ETL_ENUM_TYPE_16(SIDInstrumentRingModulator, "OscRingMod")
  ETL_ENUM_TYPE_16(SIDInstrumentADSR, "OscADSR")
  ETL_ENUM_TYPE_16(SIDInstrumentFilterOn, "VoiceFilterOn")
  ETL_ENUM_TYPE_16(SIDInstrumentTable, "Table")
  ETL_ENUM_TYPE_16(SIDInstrumentTableAutomation, "table automation")
  ETL_ENUM_TYPE_16(SIDInstrumentOSCNumber, "OscNum")

  // channel variable not currently used by OPAL instruments but maybe in future
  ETL_ENUM_TYPE_16(OPALInstrumentChannel, "Channel")
  ETL_ENUM_TYPE_16(OPALInstrumentAlgorithm, "Algorithm")
  ETL_ENUM_TYPE_16(OPALInstrumentFeedback, "Feedback")
  ETL_ENUM_TYPE_16(OPALInstrumentDeepTremeloVibrato, "DeepTremVibrato")

  ETL_ENUM_TYPE_16(OPALInstrumentOp1Level, "Op1Level")
  ETL_ENUM_TYPE_16(OPALInstrumentOp1Multiplier, "Op1Multiplier")
  ETL_ENUM_TYPE_16(OPALInstrumentOp1KeyScaleLevel, "Op1KeyscaleLevel")
  ETL_ENUM_TYPE_16(OPALInstrumentOp1ADSR, "Op1ADSR")
  ETL_ENUM_TYPE_16(OPALInstrumentOp1WaveShape, "Op1Waveshape")
  ETL_ENUM_TYPE_16(OPALInstrumentOp1TremVibSusKSR, "Op1TremVibSusKSR")

  ETL_ENUM_TYPE_16(OPALInstrumentOp2Level, "Op2Level")
  ETL_ENUM_TYPE_16(OPALInstrumentOp2Multiplier, "Op2Multiplier")
  ETL_ENUM_TYPE_16(OPALInstrumentOp2KeyScaleLevel, "Op2KeyScaleLevel")
  ETL_ENUM_TYPE_16(OPALInstrumentOp2ADSR, "Op2ADSR")
  ETL_ENUM_TYPE_16(OPALInstrumentOp2WaveShape, "Op2Waveshape")
  ETL_ENUM_TYPE_16(OPALInstrumentOp2TremVibSusKSR, "Op2TremVibSusKSR")

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
  ETL_ENUM_TYPE_16(ChiptuneInstrumentLevel, "Level")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentLength, "Length")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentBurst, "Burst")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentVibrato, "Vibrato")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentVibratoDelay, "VibratoDelay")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentTranspose, "Transpose")
  ETL_ENUM_TYPE_16(ChiptuneInstrumentTable, "Table")
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

  ETL_ENUM_TYPE_16(Default, "Default")
  ETL_END_ENUM_TYPE
};

typedef uint32_t stereosample;

#endif
