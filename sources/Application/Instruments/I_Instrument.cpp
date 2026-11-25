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

#include "I_Instrument.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Application/Persistency/VariableJson.h"
#include "Application/Utils/char.h"
#include "Foundation/Constants/Version.h"
#include "System/Console/Trace.h"

Voice I_Instrument::voices_[SONG_CHANNEL_COUNT];
I_Instrument *I_Instrument::voiceOwner_[SONG_CHANNEL_COUNT];
equalizer_t I_Instrument::equalizers_[SONG_CHANNEL_COUNT];
compressor_t I_Instrument::compressors_[SONG_CHANNEL_COUNT];

I_Instrument::~I_Instrument() {
  // Virtual destructor implementation
}

void I_Instrument::ReleaseVoices(I_Instrument *instrument) {
  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    if (voiceOwner_[i] == instrument) {
      voiceOwner_[i] = nullptr;
    }
  }
}

void I_Instrument::ReleaseAllVoices() {
  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    voiceOwner_[i] = nullptr;
  }
}

constexpr Token I_Instrument::EffectParameterTokens[8];

uint8_t *I_Instrument::EffectParameter(Token token) {
  switch (token) {
    case Token::InstrumentParameterEqOn:
      return &eq_.on;
    case Token::InstrumentParameterEqLow:
      return &eq_.low;
    case Token::InstrumentParameterEqMid:
      return &eq_.mid;
    case Token::InstrumentParameterEqHigh:
      return &eq_.high;
    case Token::InstrumentParameterCompOn:
      return &comp_.on;
    case Token::InstrumentParameterCompThreshold:
      return &comp_.threshold;
    case Token::InstrumentParameterCompRatio:
      return &comp_.ratio;
    case Token::InstrumentParameterCompRelease:
      return &comp_.release;
    default:
      return nullptr;
  }
}

uint8_t *I_Instrument::EffectParameterByName(const char *name, bool &isSwitch) {
  for (Token token : EffectParameterTokens) {
    if (strcasecmp(token.c_str(), name) == 0) {
      isSwitch = (token == Token::InstrumentParameterEqOn) || (token == Token::InstrumentParameterCompOn);
      return EffectParameter(token);
    }
  }
  return nullptr;
}

void I_Instrument::ResetEffects() {
  eq_ = eq_params_t();
  comp_ = comp_params_t();
}

bool I_Instrument::EffectsModified() {
  eq_params_t eqDefault;
  comp_params_t compDefault;
  return memcmp(&eq_, &eqDefault, sizeof(eq_)) != 0 || memcmp(&comp_, &compDefault, sizeof(comp_)) != 0;
}

void I_Instrument::CopyEffectsFrom(I_Instrument &other) {
  eq_ = other.eq_;
  comp_ = other.comp_;
}

void I_Instrument::ProcessEqualizer(int channel, fixed *buffer, int size) {
  equalizer_t &eq = equalizers_[channel];
  if (!eq_.on) {
    eq.active = false;
    return;
  }
  // restart from silence so switching it on doesn't click on stale state
  if (!eq.active) {
    eq.reset();
    eq.active = true;
  }
  eq.set_low(eq_.low);
  eq.set_mid(eq_.mid);
  eq.set_high(eq_.high);

  eq.process_buffer(buffer, size);
}

void I_Instrument::ProcessCompressor(int channel, fixed *buffer, int size) {
  compressor_t &comp = compressors_[channel];
  if (!comp_.on) {
    comp.active = false;
    return;
  }
  // restart from unity gain so switching it on doesn't click on stale state
  if (!comp.active) {
    comp.reset();
    comp.active = true;
  }
  comp.set(comp_.threshold, comp_.ratio, comp_.release);
  comp.process_buffer(buffer, size);
}

// {"type": "sample", "version": "0.5.0", "name": "kick", "variables": {"InstrumentParameterVolume": 255, ...},
//  "effects": {"InstrumentParameterEqOn": true, ...}}
// the type comes first, it is needed to create the instrument before the rest is read
void I_Instrument::SaveContent(JsonWriter &writer) {
  writer.String("type", InstrumentTypeNames[GetType()].persistence);
  writer.String("version", PRODUCT_VERSION);

  // Save the instrument name as its not stored in the Variables
  if (!name_.empty()) {
    writer.String("name", name_.c_str());
  }

  writer.BeginObject("variables");
  for (auto it = Variables()->begin(); it != Variables()->end(); it++) {
    WriteVariableJson(writer, **it);
  }
  writer.EndObject();

  // The equalizer and compressor settings are not Variables, they are saved the same way though
  writer.BeginObject("effects");
  for (Token token : EffectParameterTokens) {
    const bool isOn = (token == Token::InstrumentParameterEqOn) || (token == Token::InstrumentParameterCompOn);
    if (isOn) {
      writer.Bool(token.c_str(), *EffectParameter(token) != 0);
    } else {
      writer.Int(token.c_str(), *EffectParameter(token));
    }
  }
  writer.EndObject();
}

bool I_Instrument::RestoreKey(JsonReader &reader) {
  if (reader.KeyIs("type") || reader.KeyIs("version")) {
    char text[32];
    reader.ReadString(text, sizeof(text));
    // TODO: We already know the instrument type so need to validate it matches the imported one here
    Trace::Log("I_INSTRUMENT", "Instrument %s from file: %s", reader.Key(), text);
    return true;
  }

  if (reader.KeyIs("name")) {
    char text[MAX_VARIABLE_STRING_LENGTH + 1];
    reader.ReadString(text, sizeof(text));
    if (!reader.HadError()) {
      SetName(text);
      Trace::Log("I_INSTRUMENT", "Set instrument name: %s", text);
    }
    return true;
  }

  if (reader.KeyIs("variables")) {
    if (!reader.BeginObject()) {
      return true;
    }
    while (reader.NextKey()) {
      // Find the variable with this name and set its value
      Variable *found = nullptr;
      for (auto it = Variables()->begin(); it != Variables()->end(); it++) {
        if (!strcasecmp((*it)->GetName(), reader.Key())) {
          found = *it;
          break;
        }
      }
      if (found) {
        ReadVariableJson(reader, *found);
      } else {
        Trace::Error("Parameter '%s' not found in instrument", reader.Key());
        reader.Skip();
      }
    }
    return true;
  }

  if (reader.KeyIs("effects")) {
    if (!reader.BeginObject()) {
      return true;
    }
    while (reader.NextKey()) {
      bool isSwitch = false;
      uint8_t *param = EffectParameterByName(reader.Key(), isSwitch);
      if (!param) {
        reader.Skip();
      } else if (isSwitch) {
        const bool on = reader.ReadBool();
        *param = on ? 1 : 0;
      } else {
        const int value = reader.ReadInt();
        *param = (value < 0) ? 0 : ((value > 0xFF) ? 0xFF : value);
      }
    }
    return true;
  }
  return false;
}

void I_Instrument::RestoreContent(JsonReader &reader) {
  while (reader.NextKey()) {
    if (!RestoreKey(reader)) {
      reader.Skip();
    }
  }

  RestoreFinished();
}

// Update any UI variables that represent the instrument name
void I_Instrument::RestoreFinished() {
  Variable *nameVar = FindVariable(Token::InstrumentName);
  if (nameVar && !name_.empty()) {
    nameVar->SetString(name_.c_str());
  }
}

void I_Instrument::Purge() {
  for (auto it = Variables()->begin(); it != Variables()->end(); it++) {
    (*it)->Reset();
  }
  ResetEffects();
}

void I_Instrument::noteDisplay(uint8_t note, char (&out)[4]) {
  if (note == NO_NOTE) {
    npf_snprintf(out, sizeof(out), "---");
  } else if (note == NOTE_OFF) {
    npf_snprintf(out, sizeof(out), "off");
  } else {
    noteToString(note, out);
  }
}

void I_Instrument::focusedNoteDisplay(uint8_t note, char (&line)[12]) {
  char (&noteOut)[4] = *reinterpret_cast<char (*)[4]>(line);
  noteDisplay(note, noteOut);
}

void I_Instrument::noteDisplayCondensed(uint8_t note, char (&line1)[3], char (&line2)[3]) {
  npf_snprintf(line2, sizeof(line2), "  ");

  if (note == NO_NOTE) {
    strcpy(line1, "--");
  } else if (note == NOTE_OFF) {
    strcpy(line1, "off");
  } else {
    char buf[4];
    noteToString(note, buf);

    line1[0] = buf[0];
    line1[1] = buf[1];
    line1[2] = '\0';

    line2[0] = ' ';
    line2[1] = buf[2];
    line2[2] = '\0';
  }
}