/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "DrumInstrument.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "I_Instrument.h"
#include <string.h>

drum_voice_t DrumInstrument::voices_[SONG_CHANNEL_COUNT];

DrumInstrument::DrumInstrument()
    : I_Instrument(&variables_), vVoice0_(Token::DrumInstrumentParamsVoice0, defaultInstrument0),
      vVoice1_(Token::DrumInstrumentParamsVoice1, defaultInstrument1),
      vVoice2_(Token::DrumInstrumentParamsVoice2, defaultInstrument2),
      vVoice3_(Token::DrumInstrumentParamsVoice3, defaultInstrument3),
      vVoice4_(Token::DrumInstrumentParamsVoice4, defaultInstrument4),
      vVoice5_(Token::DrumInstrumentParamsVoice5, defaultInstrument5),
      vVoice6_(Token::DrumInstrumentParamsVoice6, defaultInstrument6),
      vVoice7_(Token::DrumInstrumentParamsVoice7, defaultInstrument7),
      vVoice8_(Token::DrumInstrumentParamsVoice8, defaultInstrument8),
      vVoice9_(Token::DrumInstrumentParamsVoice9, defaultInstrument9),
      vVoice10_(Token::DrumInstrumentParamsVoice10, defaultInstrument10),
      vVoice11_(Token::DrumInstrumentParamsVoice11, defaultInstrument11),
      vCharacter_(Token::DrumInstrumentParamsCharacter, defaultCharacter) {

  // Initialize exported variables
  // name_ is now an etl::string in the base class, not a Variable
  variables_.insert(variables_.end(), &vVoice0_);
  variables_.insert(variables_.end(), &vVoice1_);
  variables_.insert(variables_.end(), &vVoice2_);
  variables_.insert(variables_.end(), &vVoice3_);
  variables_.insert(variables_.end(), &vVoice4_);
  variables_.insert(variables_.end(), &vVoice5_);
  variables_.insert(variables_.end(), &vVoice6_);
  variables_.insert(variables_.end(), &vVoice7_);
  variables_.insert(variables_.end(), &vVoice8_);
  variables_.insert(variables_.end(), &vVoice9_);
  variables_.insert(variables_.end(), &vVoice10_);
  variables_.insert(variables_.end(), &vVoice11_);

  variables_.insert(variables_.end(), &vCharacter_);
}

void DrumInstrument::Stop(int channel) {
  voices_[channel].stop();
}

bool DrumInstrument::Start(int channel, unsigned char note, uint8_t volume, bool retrigger) {
  // get the instrument parameters from the instrument and pass them to the
  // current voice
  uint8_t calculatedVolume = (volume == NO_VOLUME) ? 255 : volumeLUT[volume];

  voices_[channel].note_on(note, calculatedVolume, retrigger, (drum_parameters_t)getInstrumentParameters(note));

  return true;
}

bool DrumInstrument::Render(int channel, fixed *buffer, int size, bool updateTick) {
  // PROFILE_SCOPE("DrumInstrument::Render");
  drum_voice_t &v = voices_[channel];

  for (int s = 0; s < size; s++) {
    v.sample(buffer, buffer + 1);

    // Output to both channels
    buffer += 2;
  }

  return true;
}

void DrumInstrument::ProcessCommand(int channel, Token token, uint16_t value) {
  switch (token) {
    case Token::InstrumentCommandSetInstrumentParameter:
      voices_[channel].set_instrument_parameter(value >> 8, value & 0xFF);
      break;

    case Token::InstrumentCommandArpeggiator:
      break;

    case Token::InstrumentCommandKill:
    case Token::InstrumentCommandGateOff:
      voices_[channel].stop();
      break;

    case Token::InstrumentCommandCrush:
      voices_[channel].bitcrush = value && 0x0f;
      voices_[channel].drive = value >> 8;
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
      voices_[channel].volume = value & 0xff;
      break;

    case Token::InstrumentCommandPitchFineTune:
      break;

    case Token::InstrumentCommandInstrumentRetrigger:
      break;
  }
}

// TODO POD: implement and adjust accordingly
bool DrumInstrument::SupportsCommand(Token token) {
  return false;
}

void DrumInstrument::SetStepVolume(int channel, uint8_t volume) {
  uint8_t calculatedVolume = (volume == NO_VOLUME) ? 255 : volumeLUT[volume];
  voices_[channel].set_step_volume(calculatedVolume);
}

drum_parameters_t DrumInstrument::getInstrumentParameters(uint8_t note) {
  auto it = variables_.begin();
  std::advance(it, note % 12);
  Variable *var = *it;
  drum_parameters_t params = std::bit_cast<drum_parameters_t>(var->GetInt());
  params.character = vCharacter_.GetInt();
  return params;
}

void DrumInstrument::noteDisplay(uint8_t note, char (&out)[4]) {
  if (note >= LOWEST_NOTE && note <= HIGHEST_NOTE) {
    strcpy(out, drumShortNames[note % 12]);
    return;
  }

  I_Instrument::noteDisplay(note, out);
}