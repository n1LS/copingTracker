/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "StackInstrument.h"

stack_voice_t StackInstrument::voices_[SONG_CHANNEL_COUNT];

// extract a signed nibble from the lowest nibble of a uint
#define int4(value) ((value & 0x08) ? (value & 0x0f) - 16 : value & 0x0f)

StackInstrument::StackInstrument()
    : I_Instrument(&variables_), spread_(Token::StackInstrumentSpread, stackDefaultSpread),
      wave_(Token::StackInstrumentWave, stackWaveNames, stackNumWaveforms, stackDefaultWave),
      transpose_(Token::StackInstrumentTranspose, stackDefaultTranspose), table_(Token::StackInstrumentTable, VAR_OFF),
      tableAuto_(Token::StackInstrumentTableAutomation, false),
      attack_(Token::StackInstrumentAttack, stackDefaultAttack), decay_(Token::StackInstrumentDecay, stackDefaultDecay),
      sustain_(Token::StackInstrumentSustain, stackDefaultSustain),
      release_(Token::StackInstrumentRelease, stackDefaultRelease),
      volume_(Token::StackInstrumentVolume, stackDefaultVolume),
      brightness_(Token::StackInstrumentBrightness, stackDefaultBrightness),
      glide_(Token::StackInstrumentGlide, stackDefaultGlide) {

  // Initialize exported variables
  // name_ is now an etl::string in the base class, not a Variable
  variables_.insert(variables_.end(), &spread_);
  variables_.insert(variables_.end(), &wave_);
  variables_.insert(variables_.end(), &transpose_);
  variables_.insert(variables_.end(), &table_);
  variables_.insert(variables_.end(), &tableAuto_);
  variables_.insert(variables_.end(), &attack_);
  variables_.insert(variables_.end(), &decay_);
  variables_.insert(variables_.end(), &sustain_);
  variables_.insert(variables_.end(), &release_);
  variables_.insert(variables_.end(), &volume_);
  variables_.insert(variables_.end(), &brightness_);
  variables_.insert(variables_.end(), &glide_);
}

StackInstrument::~StackInstrument() {
}

void StackInstrument::Stop(int channel) {
  voices_[channel].stop();
}

bool StackInstrument::Start(int channel, unsigned char note, uint8_t volume, bool retrigger) {
  // get the instrument parameters from the instrument and pass them to the
  // current voice
  uint8_t calculatedVolume = (volume == NO_VOLUME) ? 255 : volumeLUT[volume];

  voices_[channel].note_on(note, calculatedVolume, retrigger, getInstrumentParameters());

  return true;
}

bool StackInstrument::Render(int channel, fixed *buffer, int size, bool updateTick) {
  // PROFILE_SCOPE("StackInstrument::Render");
  stack_voice_t &v = voices_[channel];

  for (int s = 0; s < size; s++) {
    v.sample(buffer, buffer + 1);

    // Output to both channels
    buffer += 2;
  }

  return true;
}

void StackInstrument::ProcessCommand(int channel, Token token, uint16_t value) {
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

    case Token::InstrumentCommandChordUp:
      voices_[channel].set_chord((value & 0xf000) >> 12, (value & 0x0f00) >> 8, (value & 0x00f0) >> 4, value & 0x000f);
      break;

    case Token::InstrumentCommandChordDown:
      voices_[channel].set_chord(-((value & 0xf000) >> 12), -((value & 0x0f00) >> 8), -((value & 0x00f0) >> 4),
                                 -(value & 0x000f));
      break;

    case Token::InstrumentCommandChordBidirectional:
      voices_[channel].set_chord(int4(value >> 12), int4(value >> 8), int4(value >> 4), int4(value));
      break;
  }
}

// TODO POD: implement and adjust accordingly
bool StackInstrument::SupportsCommand(Token token) {
  return false;
}

void StackInstrument::SetStepVolume(int channel, uint8_t volume) {
  uint8_t calculatedVolume = (volume == NO_VOLUME) ? 255 : volumeLUT[volume];
  voices_[channel].set_step_volume(calculatedVolume);
}

stack_parameters_t StackInstrument::getInstrumentParameters() {
  stack_parameters_t params;

  params.spread = spread_.GetInt();
  params.attack = attack_.GetInt();
  params.decay = decay_.GetInt();
  params.sustain = sustain_.GetInt();
  params.release = release_.GetInt();
  params.volume = volume_.GetInt();
  params.brightness = brightness_.GetInt();
  params.glide = glide_.GetInt();
  params.wave = wave_.GetInt();
  params.transpose = transpose_.GetInt();

  return params;
}