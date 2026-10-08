/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "LSDJKitInstrument.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "I_Instrument.h"
#include "LSDJKits.generated.h"
#include <string.h>

LSDJKitInstrument::LSDJKitInstrument()
    : I_Instrument(&variables_),
      vKit1_(Token::LSDJKitInstrumentKit1, LSDJKits::kitNames, LSDJKits::drum_kit_count, lsdjDefaultKit1),
      vKit2_(Token::LSDJKitInstrumentKit2, LSDJKits::kitNames, LSDJKits::drum_kit_count, lsdjDefaultKit2),
      vBitDepth_(Token::LSDJKitInstrumentBitDepth, lsdjDefaultBitDepth),
      vOffset1_(Token::LSDJKitInstrumentOffset1, lsdjDefaultOffset),
      vLength1_(Token::LSDJKitInstrumentLength1, lsdjDefaultLength),
      vLoop1_(Token::LSDJKitInstrumentLoop1, loopModeNames, loopModeCount, lsdjDefaultLoop),
      vOffset2_(Token::LSDJKitInstrumentOffset2, lsdjDefaultOffset),
      vLength2_(Token::LSDJKitInstrumentLength2, lsdjDefaultLength),
      vLoop2_(Token::LSDJKitInstrumentLoop2, loopModeNames, loopModeCount, lsdjDefaultLoop),
      vSpeed_(Token::LSDJKitInstrumentSpeed, speedModeNames, speedModeCount, lsdjDefaultSpeed),
      vClip_(Token::LSDJKitInstrumentClip, clippingModeNames, clippingModeCount, lsdjDefaultClip) {
  // Initialize exported variables
  // name_ is now an etl::string in the base class, not a Variable
  variables_.push_back(&vKit1_);
  variables_.push_back(&vKit2_);
  variables_.push_back(&vBitDepth_);
  variables_.push_back(&vOffset1_);
  variables_.push_back(&vLength1_);
  variables_.push_back(&vLoop1_);
  variables_.push_back(&vOffset2_);
  variables_.push_back(&vLength2_);
  variables_.push_back(&vLoop2_);
  variables_.push_back(&vSpeed_);
  variables_.push_back(&vClip_);

  InsertBaseVariables();
}

void LSDJKitInstrument::Stop(int channel) {
  voices_[channel].lsdj_kit.stop();
}

void LSDJKitInstrument::InitVoice(int channel) {
  voices_[channel].lsdj_kit.init();
}

bool LSDJKitInstrument::Start(int channel, unsigned char note, uint8_t volume, bool retrigger) {
  // get the instrument parameters from the instrument and pass them to the
  // current voice
  uint8_t calculatedVolume = EffectiveVolume(volume);

  voices_[channel].lsdj_kit.note_on(note, calculatedVolume, retrigger, getInstrumentParameters(note));

  return true;
}

bool LSDJKitInstrument::Render(int channel, fixed *buffer, int size, bool updateTick) {
  // PROFILE_SCOPE("LSDJKitInstrument::Render");

  for (int s = 0; s < size; s++) {
    voices_[channel].lsdj_kit.sample(buffer, buffer + 1);

    // Output to both channels
    buffer += 2;
  }

  return true;
}

void LSDJKitInstrument::ProcessCommand(int channel, Token token, uint16_t value) {
  switch (token) {
    case Token::InstrumentCommandSetInstrumentParameter:
      voices_[channel].lsdj_kit.set_instrument_parameter(value >> 8, value & 0xFF);
      break;

    case Token::InstrumentCommandArpeggiator:
      break;

    case Token::InstrumentCommandKill:
    case Token::InstrumentCommandGateOff:
      voices_[channel].lsdj_kit.stop();
      break;

    case Token::InstrumentCommandCrush:
      voices_[channel].lsdj_kit.bit_depth = std::min(std::max(value & 0x0f, 1), 8);
      voices_[channel].lsdj_kit.drive = value >> 8;
      break;

    case Token::InstrumentCommandVibrato:
      break;

    case Token::InstrumentCommandPan:
      break;

    case Token::InstrumentCommandPitchSlide:
      break;

    case Token::InstrumentCommandLegato:
      break;

    case Token::InstrumentCommandVolume:
      voices_[channel].lsdj_kit.volume = value & 0xff;
      break;

    case Token::InstrumentCommandPitchFineTune:
      break;

    case Token::InstrumentCommandInstrumentRetrigger:
      break;
  }
}

// TODO nILS: implement and adjust accordingly
bool LSDJKitInstrument::SupportsCommand(Token token) {
  return false;
}

void LSDJKitInstrument::SetStepVolume(int channel, uint8_t volume) {
  uint8_t calculatedVolume = EffectiveVolume(volume);
  voices_[channel].lsdj_kit.set_step_volume(calculatedVolume);
}

lsdjkit_parameters_t LSDJKitInstrument::getInstrumentParameters(uint8_t note) {
  lsdjkit_parameters_t params;

  params.kit1 = FindVariable(Token::LSDJKitInstrumentKit1)->GetInt();
  params.kit2 = FindVariable(Token::LSDJKitInstrumentKit2)->GetInt();
  params.bit_depth = FindVariable(Token::LSDJKitInstrumentBitDepth)->GetInt();
  params.pan = EffectivePan();
  params.volume = FindVariable(Token::InstrumentParameterVolume)->GetInt();
  params.speed = FindVariable(Token::LSDJKitInstrumentSpeed)->GetInt();
  params.loop_mode.mode1 = static_cast<lsdjkit_loop_mode_e>(FindVariable(Token::LSDJKitInstrumentLoop1)->GetInt());
  params.loop_mode.mode2 = static_cast<lsdjkit_loop_mode_e>(FindVariable(Token::LSDJKitInstrumentLoop2)->GetInt());
  params.offset[0] = FindVariable(Token::LSDJKitInstrumentOffset1)->GetInt();
  params.offset[1] = FindVariable(Token::LSDJKitInstrumentOffset2)->GetInt();
  params.length[0] = FindVariable(Token::LSDJKitInstrumentLength1)->GetInt();
  params.length[1] = FindVariable(Token::LSDJKitInstrumentLength2)->GetInt();
  params.clip_mode = FindVariable(Token::LSDJKitInstrumentClip)->GetInt();

  return params;
}

void LSDJKitInstrument::noteDisplay(uint8_t note, char (&out)[4]) {
  if (note <= LSDJKIT_HIGHEST_NOTE) {
    Variable *vars[2] = {&vKit1_, &vKit2_};
    int notes[2] = {note / 15, note % 15};

    for (int n = 0; n < 2; n++) {
      int kitId = vars[n]->GetInt();

      if (kitId == NO_KIT) {
        out[n * 2] = '?';
      } else {
        const LSDJKits::Kit *kit = &LSDJKits::kits[kitId];
        int sampleId = notes[n];

        if (sampleId == 0) {
          out[n * 2] = '-';
        } else if (sampleId > (int)kit->num_samples) {
          out[n * 2] = CHAR(char_symbol_indicatorEmpty_s);
        } else {
          out[n * 2] = kit->samples[sampleId - 1].name[0];
        }
      }
    }

    out[1] = ':';
    out[3] = 0;

    return;
  }

  I_Instrument::noteDisplay(note, out);
}

void LSDJKitInstrument::noteDisplayCondensed(uint8_t note, char (&line1)[3], char (&line2)[3]) {
  if (note <= LSDJKIT_HIGHEST_NOTE) {
    const char *hex = "-123456789ABCDEF";

    Variable *vars[2] = {&vKit1_, &vKit2_};
    int notes[2] = {note / 15, note % 15};

    char *lines[2] = {line1, line2};

    for (int n = 0; n < 2; n++) {
      int kitId = vars[n]->GetInt();

      if (kitId == NO_KIT) {
        lines[n][0] = '?';
        lines[n][1] = '?';
      } else {
        const LSDJKits::Kit *kit = &LSDJKits::kits[kitId];
        int sampleId = notes[n];

        if (sampleId == 0 || sampleId > (int)kit->num_samples) {
          lines[n][0] = '-';
          lines[n][1] = '-';
        } else {
          lines[n][0] = kit->samples[sampleId - 1].name[0];
          lines[n][1] = kit->samples[sampleId - 1].name[1];
        }
      }
    }

    line1[2] = 0;
    line2[2] = 0;

    return;
  }

  I_Instrument::noteDisplayCondensed(note, line1, line2);
}

void LSDJKitInstrument::focusedNoteDisplay(uint8_t note, char (&line)[12]) {
  Variable *vars[2] = {&vKit1_, &vKit2_};
  int notes[2] = {note / 15, note % 15};

  for (int n = 0; n < 2; n++) {
    int kitId = vars[n]->GetInt();

    if (kitId == NO_KIT) {
      strcpy(line + 1 + n * 4, "???");
    } else {
      const LSDJKits::Kit *kit = &LSDJKits::kits[kitId];
      int sampleId = notes[n];

      if (sampleId == 0) {
        strcpy(line + 1 + n * 4, "---");
      } else if (sampleId > (int)kit->num_samples) {
        const char *hex = "0123456789ABCDEF";
        strcpy(line + 1 + n * 4, "( )");
        line[2 + n * 4] = hex[sampleId];
      } else {
        strcpy(line + 1 + n * 4, kit->samples[sampleId - 1].name);
      }
    }
  }

  // the second strcpy already terminated the string at line[7]

  line[0] = CHAR(char_button_up_down_s);
  line[4] = ':';
  line[8] = CHAR(char_button_left_right_s);
  line[9] = '\0';
}
