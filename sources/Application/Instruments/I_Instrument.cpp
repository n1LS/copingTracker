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

bool I_Instrument::RestoreEffectParameter(const char *name, const char *value) {
  for (Token token : EffectParameterTokens) {
    if (strcasecmp(token.c_str(), name) == 0) {
      uint8_t *param = EffectParameter(token);
      bool isOn = (token == Token::InstrumentParameterEqOn) || (token == Token::InstrumentParameterCompOn);
      if (isOn) {
        // saved the way a bool Variable persists
        *param = (strcasecmp(value, "true") == 0) || (atoi(value) != 0);
      } else {
        int v = atoi(value);
        *param = (v < 0) ? 0 : ((v > 0xFF) ? 0xFF : v);
      }
      return true;
    }
  }
  return false;
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

void I_Instrument::SaveContent(tinyxml2::XMLPrinter *printer) {
  // Add firmware version information
  printer->PushAttribute(XML_ATTR_VERSION, PRODUCT_VERSION);
  // Save the instrument type
  printer->PushAttribute(XML_ATTR_TYPE, InstrumentTypeNames[GetType()].persistence);

  // Save the instrument name as its not stored in the Variables
  if (!name_.empty()) {
    printer->OpenElement(XML_ELEM_PARAMETER);
    printer->PushAttribute(XML_ATTR_NAME, XML_ATTR_INSTRUMENT_NAME);
    printer->PushAttribute(XML_ATTR_VALUE, name_.c_str());
    printer->CloseElement(); // PARAM
  }

  // Save all the instrument's parameters
  for (auto it = Variables()->begin(); it != Variables()->end(); it++) {
    printer->OpenElement(XML_ELEM_PARAMETER);
    printer->PushAttribute(XML_ATTR_NAME, (*it)->GetName());
    printer->PushAttribute(XML_ATTR_VALUE, (*it)->GetPersistenceString().c_str());
    printer->CloseElement(); // PARAM
  }

  // The equalizer and compressor settings are not Variables, they are saved
  // the same way though so the project files stay compatible
  for (Token token : EffectParameterTokens) {
    bool isOn = (token == Token::InstrumentParameterEqOn) || (token == Token::InstrumentParameterCompOn);
    char value[8];
    if (isOn) {
      npf_snprintf(value, sizeof(value), "%s", *EffectParameter(token) ? "true" : "false");
    } else {
      npf_snprintf(value, sizeof(value), "%d", *EffectParameter(token));
    }
    printer->OpenElement(XML_ELEM_PARAMETER);
    printer->PushAttribute(XML_ATTR_NAME, token.c_str());
    printer->PushAttribute(XML_ATTR_VALUE, value);
    printer->CloseElement(); // PARAM
  }
}

void I_Instrument::RestoreContent(PersistencyDocument *doc) {
  // First, check for TYPE attribute in the INSTRUMENT element
  bool hasAttr = doc->NextAttribute();
  while (hasAttr) {
    if (!strcasecmp(doc->attrname(), XML_ATTR_TYPE)) {
      Trace::Log("I_INSTRUMENT", "Instrument type from XML: %s", doc->attrval());
      // TODO: We already know the instrument type so need to validate it
      // matches the imported one here
    }
    hasAttr = doc->NextAttribute();
  }

  // Navigate to the first child of the INSTRUMENT element (which should be a
  // PARAM element)
  bool subelem = doc->FirstChild();

  while (subelem) {
    // Process the PARAM element attributes
    bool hasAttr = doc->NextAttribute();
    char name[MAX_VARIABLE_STRING_LENGTH + 1] = "";
    char value[MAX_VARIABLE_STRING_LENGTH + 1] = "";
    while (hasAttr) {
      if (!strcasecmp(doc->attrname(), XML_ATTR_NAME)) {
        strcpy(name, doc->attrval());
      }
      if (!strcasecmp(doc->attrname(), XML_ATTR_VALUE)) {
        strcpy(value, doc->attrval());
      }
      hasAttr = doc->NextAttribute();
    }

    if (name[0] != '\0' && value[0] != '\0') {
      // Special handling for InstrumentName parameter
      if (!strcasecmp(name, XML_ATTR_INSTRUMENT_NAME)) {
        // Set the instrument name directly
        SetName(value);
        Trace::Log("I_INSTRUMENT", "Set instrument name: %s", value);
      } else if (!RestoreEffectParameter(name, value)) {
        // For other parameters, find the variable and set its value
        bool found = false;

        // Find the variable with this name and set its value
        for (auto it = Variables()->begin(); it != Variables()->end(); it++) {
          if (!strcasecmp((*it)->GetName(), name)) {
            (*it)->SetString(value);
            // Trace::Log("I_INSTRUMENT", "Set parameter: %s = %s", name,
            // value);
            found = true;
            break;
          }
        }

        if (!found) {
          Trace::Error("Parameter '%s' not found in instrument", name);
        }
      }
    }

    // Move to the next PARAM element
    subelem = doc->NextSibling();
  }
  // Trace::Log("I_INSTRUMENT", "RestoreContent: Restored %d parameters",
  //            paramCount);

  // Update any UI variables that represent the instrument name
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