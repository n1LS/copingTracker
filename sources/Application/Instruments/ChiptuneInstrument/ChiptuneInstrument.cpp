/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "ChiptuneInstrument.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "I_Instrument.h"
#include <string.h>

ChiptuneInstrument::ChiptuneInstrument()
    : I_Instrument(&variables_), vArpSpeed_(Token::ChiptuneInstrumentArpSpeed, defaultArpSpeed),
      vAttack_(Token::ChiptuneInstrumentAttack, defaultAttack), vBurst_(Token::ChiptuneInstrumentBurst, defaultBurst),
      vDecay_(Token::ChiptuneInstrumentDecay, defaultDecay), vLength_(Token::ChiptuneInstrumentLength, defaultLength),
      vSweepAmount_(Token::ChiptuneInstrumentSweepAmount, defaultSweepAmount),
      vSweepTime_(Token::ChiptuneInstrumentSweepTime, defaultSweepTime),
      vTranspose_(Token::ChiptuneInstrumentTranspose, defaultTranspose),
      vVibratoDelay_(Token::ChiptuneInstrumentVibratoDelay, defaultVibratoDelay),
      vVibratoDepth_(Token::ChiptuneInstrumentVibrato, defaultVibratoDepth),
      vWaveform_(Token::ChiptuneInstrumentWaveform, chiptune_waveforms, numWaveforms, defaultWaveform) {
  // Initialize exported variables
  // name_ is now an etl::string in the base class, not a Variable
  InsertBaseVariables();
  variables_.push_back(&vWaveform_);
  variables_.push_back(&vTranspose_);
  variables_.push_back(&vBurst_);
  variables_.push_back(&vArpSpeed_);
  variables_.push_back(&vLength_);
  variables_.push_back(&vAttack_);
  variables_.push_back(&vDecay_);
  variables_.push_back(&vVibratoDelay_);
  variables_.push_back(&vVibratoDepth_);
  variables_.push_back(&vSweepTime_);
  variables_.push_back(&vSweepAmount_);
}

void ChiptuneInstrument::Stop(int channel) {
  voices_[channel].chiptune.stop();
}

void ChiptuneInstrument::InitVoice(int channel) {
  voices_[channel].chiptune.init();
}

bool ChiptuneInstrument::Start(int channel, unsigned char note, uint8_t volume, bool retrigger) {
  // get the instrument parameters from the instrument and pass them to the
  // current voice
  uint8_t calculatedVolume = EffectiveVolume(volume);

  voices_[channel].chiptune.note_on(note, calculatedVolume, retrigger, getInstrumentParameters());

  return true;
}

bool ChiptuneInstrument::Render(int channel, fixed *buffer, int size, bool updateTick) {
  // PROFILE_SCOPE("ChiptuneInstrument::Render");

  for (int s = 0; s < size; s++) {
    voices_[channel].chiptune.sample(buffer, buffer + 1);

    // Output to both channels
    buffer += 2;
  }

  return true;
}

void ChiptuneInstrument::ProcessCommand(int channel, Token token, uint16_t value) {
  switch (token) {
    case Token::InstrumentCommandSetInstrumentParameter:
      voices_[channel].chiptune.set_instrument_parameter(value >> 8, value & 0xFF);
      break;

    case Token::InstrumentCommandArpeggiator:
      voices_[channel].chiptune.command_init_arp(value);
      break;

    case Token::InstrumentCommandKill:
    case Token::InstrumentCommandGateOff:
      voices_[channel].chiptune.stop();
      break;

    case Token::InstrumentCommandCrush:
      voices_[channel].chiptune.bitcrush = value & 0x0f;
      voices_[channel].chiptune.drive = value >> 8;
      break;

    case Token::InstrumentCommandVibrato:
      voices_[channel].chiptune.command_init_vibrato(value >> 8, value & 0xFF);
      break;

    case Token::InstrumentCommandPan:
      voices_[channel].chiptune.command_init_pan(value >> 8, value & 0xFF);
      break;

    case Token::InstrumentCommandPitchSlide:
      voices_[channel].chiptune.command_init_pitch_shift(value >> 8, value & 0xFF);
      break;

    case Token::InstrumentCommandLegato:
      voices_[channel].chiptune.command_init_legato(value >> 8, (int8_t)(value & 0xFF));
      break;

    case Token::InstrumentCommandVolume:
      voices_[channel].chiptune.command_init_volume(value >> 8, value & 0xFF);
      break;

    case Token::InstrumentCommandPitchFineTune:
      voices_[channel].chiptune.command_init_finetune(value >> 8, (int8_t)(value & 0xFF));
      break;

    case Token::InstrumentCommandInstrumentRetrigger:
      voices_[channel].chiptune.command_init_instrument_retrigger(value >> 8, (int8_t)(value & 0xff));
      break;
  }
}

// TODO nILS: implement and adjust accordingly
bool ChiptuneInstrument::SupportsCommand(Token token) {
  return false;
}

void ChiptuneInstrument::SetStepVolume(int channel, uint8_t volume) {
  uint8_t calculatedVolume = EffectiveVolume(volume);
  voices_[channel].chiptune.set_step_volume(calculatedVolume);
}

InstrumentParameters ChiptuneInstrument::getInstrumentParameters() {
  InstrumentParameters params;

  params.wave = (chiptune_wave_type_e)vWaveform_.GetInt();
  params.attack = vAttack_.GetInt();
  params.decay = vDecay_.GetInt();
  // off == -1, map to uint8_t range
  params.length = vLength_.GetInt() < 0 ? 0 : vLength_.GetInt();
  params.burst = vBurst_.GetInt() < 0 ? 0 : vBurst_.GetInt();
  params.vibratoDepth = vVibratoDepth_.GetInt();
  params.vibratoDelay = vVibratoDelay_.GetInt();
  params.transpose = vTranspose_.GetInt();
  params.arpSpeed = vArpSpeed_.GetInt();
  params.sweepTime = vSweepTime_.GetInt();
  params.sweepAmount = vSweepAmount_.GetInt();

  return params;
}
