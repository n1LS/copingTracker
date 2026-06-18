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
    VarColor_0_Black = 104,
    VarColor_1_Maroon = 105,
    VarColor_2_Green = 106,
    VarColor_3_Olive = 111,
    VarColor_4_Blue = 152,
    VarColor_5_Purple = 153,
    VarColor_6_Turqoise = 154,
    VarColor_7_LightyGray = 155,
    VarColor_8_Gray = 107,
    VarColor_9_Red = 110,
    VarColor_A_Lime = 156,
    VarColor_B_Yellow = 108,
    VarColor_C_LightBlue = 157,
    VarColor_D_Magenta = 158,
    VarColor_E_Cyan = 109,
    VarColor_F_White = 103,

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
    // 188 is taken for InstrumentCommandSetInstrumentParameter
    // 189 is taken for VarOutputVolume

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
    ActionAutoSlice = 186,

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

    ActionMassStorage = 50,
    VarOutputVolume = 74,
    ActionModulation = 213,

    // 93-98 free (formerly MacroInstrument fields)
    // 176 is free
    // 182 is free
    // 183 is free
    // 187 is free

    Default = 255, // "    "
  };

  uint8_t raw() {
    return static_cast<uint8_t>(*this);
  }

  ETL_DECLARE_ENUM_TYPE(FourCC, uint8_t)

  // Not all enums need reflection. Only cases where we need reflection is the
  // FourCC codes that need to be converted to text in order to display on
  // screen
  ETL_ENUM_TYPE(InstrumentCommandArpeggiator, "ARP")
  ETL_ENUM_TYPE(InstrumentCommandCrush, "CSH")
  ETL_ENUM_TYPE(InstrumentCommandKill, "KIL")
  ETL_ENUM_TYPE(InstrumentCommandLoopOffset, "LOF")
  ETL_ENUM_TYPE(InstrumentCommandVelocity, "VEL")
  ETL_ENUM_TYPE(InstrumentCommandVolume, "VOL")
  ETL_ENUM_TYPE(InstrumentCommandPitchSlide, "PSL")
  ETL_ENUM_TYPE(InstrumentCommandHop, "HOP")
  ETL_ENUM_TYPE(InstrumentCommandLegato, "LEG")
  ETL_ENUM_TYPE(InstrumentCommandRetrigger, "RTG")
  ETL_ENUM_TYPE(InstrumentCommandTempo, "TPO")
  ETL_ENUM_TYPE(InstrumentCommandMidiCC, "MCC")
  ETL_ENUM_TYPE(InstrumentCommandMidiPC, "MPC")
  ETL_ENUM_TYPE(InstrumentCommandPlayOfset, "POF")
  ETL_ENUM_TYPE(InstrumentCommandLowPassFilter, "FLT")
  ETL_ENUM_TYPE(InstrumentCommandTable, "TBL")
  ETL_ENUM_TYPE(InstrumentCommandFilterCut, "FCT")
  ETL_ENUM_TYPE(InstrumentCommandFilterResonance, "FRS")
  ETL_ENUM_TYPE(InstrumentCommandPan, "PAN")
  ETL_ENUM_TYPE(InstrumentCommandGateOff, "GOF")
  ETL_ENUM_TYPE(InstrumentCommandGroove, "GRV")
  ETL_ENUM_TYPE(InstrumentCommandSetInstrumentParameter, "SIP")
  ETL_ENUM_TYPE(InstrumentCommandStop, "STP")
  ETL_ENUM_TYPE(InstrumentCommandNone, "---")
  ETL_ENUM_TYPE(InstrumentCommandPitchFineTune, "PFT")
  ETL_ENUM_TYPE(InstrumentCommandDelay, "DLY")
  ETL_ENUM_TYPE(InstrumentCommandInstrumentRetrigger, "IRT")
  ETL_ENUM_TYPE(InstrumentCommandMidiChord, "MCH")
  ETL_ENUM_TYPE(InstrumentCommandVibrato, "VIB")

  ETL_ENUM_TYPE(VarLineOut, "LineOut")
  ETL_ENUM_TYPE(VarMidiDevice, "MidiDevice")
  ETL_ENUM_TYPE(VarMidiSync, "MidiSync")
  ETL_ENUM_TYPE(VarMidiClockSync, "MidiClockSync")
  ETL_ENUM_TYPE(VarMirrorUI, "mirrorUI")
  ETL_ENUM_TYPE(VarUIFont, "UIFont")
  ETL_ENUM_TYPE(VarThemeName, "ThemeName")
  ETL_ENUM_TYPE(VarScaleRoot, "ScaleRoot")
  ETL_ENUM_TYPE(SampleInstrumentSample, "Sample")
  ETL_ENUM_TYPE(SampleInstrumentVolume, "Volume")
  ETL_ENUM_TYPE(SampleInstrumentInterpolation, "Interpolation")
  ETL_ENUM_TYPE(SampleInstrumentCrush, "Crush")
  ETL_ENUM_TYPE(SampleInstrumentCrushVolume, "CrushDrive")
  ETL_ENUM_TYPE(SampleInstrumentDownsample, "Downsample")
  ETL_ENUM_TYPE(SampleInstrumentRootNote, "RootNote")
  ETL_ENUM_TYPE(SampleInstrumentFineTune, "Finetune")
  ETL_ENUM_TYPE(SampleInstrumentPan, "Pan")
  ETL_ENUM_TYPE(SampleInstrumentFilterCutOff, "FilterCutoff")
  ETL_ENUM_TYPE(SampleInstrumentFilterResonance, "FilterResonance")
  ETL_ENUM_TYPE(SampleInstrumentFilterType, "FilterType")
  ETL_ENUM_TYPE(SampleInstrumentFilterMode, "FilterMode")
  ETL_ENUM_TYPE(SampleInstrumentStart, "Start")
  ETL_ENUM_TYPE(SampleInstrumentLoopMode, "LoopMode")
  ETL_ENUM_TYPE(SampleInstrumentLoopStart, "LoopStart")
  ETL_ENUM_TYPE(SampleInstrumentEnd, "End")
  ETL_ENUM_TYPE(SampleInstrumentTable, "Table")
  ETL_ENUM_TYPE(SampleInstrumentTableAutomation, "TableAutomation")
  ETL_ENUM_TYPE(MidiInstrumentChannel, "Channel")
  ETL_ENUM_TYPE(InstrumentName, "Name")
  ETL_ENUM_TYPE(MidiInstrumentName, "MidiName")
  ETL_ENUM_TYPE(MidiInstrumentNoteLength, "NoteLength")
  ETL_ENUM_TYPE(MidiInstrumentVolume, "Volume")
  ETL_ENUM_TYPE(MidiInstrumentTable, "Table")
  ETL_ENUM_TYPE(MidiInstrumentTableAutomation, "TableAutomation")
  ETL_ENUM_TYPE(MidiInstrumentProgram, "Program")
  ETL_ENUM_TYPE(SIDInstrumentWaveform, "OscWaveform")
  ETL_ENUM_TYPE(SIDInstrument1FilterCut, "FilterCutoff1")
  ETL_ENUM_TYPE(SIDInstrument1FilterResonance, "FilterResonance1")
  ETL_ENUM_TYPE(SIDInstrument1FilterMode, "FilterMode1")
  ETL_ENUM_TYPE(SIDInstrument1Volume, "Volume1")
  ETL_ENUM_TYPE(SIDInstrument2FilterCut, "FilterCutoff2")
  ETL_ENUM_TYPE(SIDInstrument2FilterResonance, "FilterResonance2")
  ETL_ENUM_TYPE(SIDInstrument2FilterMode, "FilterMode2")
  ETL_ENUM_TYPE(SIDInstrument2Volume, "Volume2")
  ETL_ENUM_TYPE(SIDInstrumentPulseWidth, "OscPulseWidth")
  ETL_ENUM_TYPE(SIDInstrumentVSync, "OscSync")
  ETL_ENUM_TYPE(SIDInstrumentRingModulator, "OscRingMod")
  ETL_ENUM_TYPE(SIDInstrumentADSR, "OscADSR")
  ETL_ENUM_TYPE(SIDInstrumentFilterOn, "VoiceFilterOn")
  ETL_ENUM_TYPE(SIDInstrumentTable, "Table")
  ETL_ENUM_TYPE(SIDInstrumentTableAutomation, "table automation")
  ETL_ENUM_TYPE(SIDInstrumentOSCNumber, "OscNum")

  // channel variable not currently used by OPAL instruments but maybe in future
  ETL_ENUM_TYPE(OPALInstrumentChannel, "Channel")
  ETL_ENUM_TYPE(OPALInstrumentAlgorithm, "Algorithm")
  ETL_ENUM_TYPE(OPALInstrumentFeedback, "Feedback")
  ETL_ENUM_TYPE(OPALInstrumentDeepTremeloVibrato, "DeepTremoloVibrato")

  ETL_ENUM_TYPE(OPALInstrumentOp1Level, "Op1Level")
  ETL_ENUM_TYPE(OPALInstrumentOp1Multiplier, "Op1Multiplier")
  ETL_ENUM_TYPE(OPALInstrumentOp1KeyScaleLevel, "Op1KeyscaleLevel")
  ETL_ENUM_TYPE(OPALInstrumentOp1ADSR, "Op1ADSR")
  ETL_ENUM_TYPE(OPALInstrumentOp1WaveShape, "Op1Waveshape")
  ETL_ENUM_TYPE(OPALInstrumentOp1TremVibSusKSR, "Op1TremVibSusKSR")

  ETL_ENUM_TYPE(OPALInstrumentOp2Level, "Op2Level")
  ETL_ENUM_TYPE(OPALInstrumentOp2Multiplier, "Op2Multiplier")
  ETL_ENUM_TYPE(OPALInstrumentOp2KeyScaleLevel, "Op2KeyScaleLevel")
  ETL_ENUM_TYPE(OPALInstrumentOp2ADSR, "Op2ADSR")
  ETL_ENUM_TYPE(OPALInstrumentOp2WaveShape, "Op2Waveshape")
  ETL_ENUM_TYPE(OPALInstrumentOp2TremVibSusKSR, "Op2TremVibSusKSR")

  ETL_ENUM_TYPE(VarColor_0_Black, "Color0")
  ETL_ENUM_TYPE(VarColor_1_Maroon, "Color1")
  ETL_ENUM_TYPE(VarColor_2_Green, "Color2")
  ETL_ENUM_TYPE(VarColor_3_Olive, "Color3")
  ETL_ENUM_TYPE(VarColor_4_Blue, "Color4")
  ETL_ENUM_TYPE(VarColor_5_Purple, "Color5")
  ETL_ENUM_TYPE(VarColor_6_Turqoise, "Color6")
  ETL_ENUM_TYPE(VarColor_7_LightyGray, "Color7")
  ETL_ENUM_TYPE(VarColor_8_Gray, "Color8")
  ETL_ENUM_TYPE(VarColor_9_Red, "Color9")
  ETL_ENUM_TYPE(VarColor_A_Lime, "Color10")
  ETL_ENUM_TYPE(VarColor_B_Yellow, "Color11")
  ETL_ENUM_TYPE(VarColor_C_LightBlue, "Color12")
  ETL_ENUM_TYPE(VarColor_D_Magenta, "Color13")
  ETL_ENUM_TYPE(VarColor_E_Cyan, "Color14")
  ETL_ENUM_TYPE(VarColor_F_White, "Color15")

  ETL_ENUM_TYPE(VarTempo, "Tempo")
  ETL_ENUM_TYPE(VarMasterVolume, "Master")
  ETL_ENUM_TYPE(VarPreviewVolume, "Preview")
  ETL_ENUM_TYPE(VarWrap, "Wrap")
  ETL_ENUM_TYPE(VarTranspose, "Transpose")
  ETL_ENUM_TYPE(VarScale, "Scale")
  ETL_ENUM_TYPE(VarProjectName, "ProjectName")
  ETL_ENUM_TYPE(VarInstrumentType, "InstrumentType")
  ETL_ENUM_TYPE(VarChannel1Volume, "channel1vol")
  ETL_ENUM_TYPE(VarChannel2Volume, "channel2vol")
  ETL_ENUM_TYPE(VarChannel3Volume, "channel3vol")
  ETL_ENUM_TYPE(VarChannel4Volume, "channel4vol")
  ETL_ENUM_TYPE(VarChannel5Volume, "channel5vol")
  ETL_ENUM_TYPE(VarChannel6Volume, "channel6vol")
  ETL_ENUM_TYPE(VarChannel7Volume, "channel7vol")
  ETL_ENUM_TYPE(VarChannel8Volume, "channel8vol")

  ETL_ENUM_TYPE(ActionEdit, "Edit")
  ETL_ENUM_TYPE(ActionExport, "Export")
  ETL_ENUM_TYPE(ActionImport, "Import")
  ETL_ENUM_TYPE(ActionThemeName, "ThemeName")
  ETL_ENUM_TYPE(VarBacklightLevel, "BacklightLevel")
  ETL_ENUM_TYPE(VarOutputVolume, "OutputVolume")
  ETL_ENUM_TYPE(VarImportResampler, "ImportResampling")

  // Chiptune Instrument Variables
  ETL_ENUM_TYPE(ChiptuneInstrumentWaveform, "Waveform")
  ETL_ENUM_TYPE(ChiptuneInstrumentAttack, "Attack")
  ETL_ENUM_TYPE(ChiptuneInstrumentDecay, "Decay")
  ETL_ENUM_TYPE(ChiptuneInstrumentLevel, "Level")
  ETL_ENUM_TYPE(ChiptuneInstrumentLength, "Length")
  ETL_ENUM_TYPE(ChiptuneInstrumentBurst, "Burst")
  ETL_ENUM_TYPE(ChiptuneInstrumentVibrato, "Vibrato")
  ETL_ENUM_TYPE(ChiptuneInstrumentVibratoDelay, "VibratoDelay")
  ETL_ENUM_TYPE(ChiptuneInstrumentTranspose, "Transpose")
  ETL_ENUM_TYPE(ChiptuneInstrumentTable, "Table")
  ETL_ENUM_TYPE(ChiptuneInstrumentSweepTime, "SweepTime")
  ETL_ENUM_TYPE(ChiptuneInstrumentSweepAmount, "SweepAmount")
  ETL_ENUM_TYPE(ChiptuneInstrumentArpSpeed, "ArpSpeed")

  ETL_ENUM_TYPE(Default, "   ")
  ETL_END_ENUM_TYPE
};

typedef uint32_t stereosample;

#endif
