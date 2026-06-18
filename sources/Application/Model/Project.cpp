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

#include "Project.h"
#include "Application/Instruments/CommandList.h"
#include "Application/Instruments/SampleInstrument.h"
#include "Application/Instruments/SamplePool.h"
#include "Application/Persistency/PersistencyService.h"
#include "Application/Player/SyncMaster.h"
#include "Foundation/Variables/WatchedVariable.h"
#include "Groove.h"
#include "Scale.h"
#include "Services/Midi/MidiService.h"
#include "System/Console/Trace.h"
#include "System/io/Status.h"
#include "Table.h"

#include <math.h>

#define DEFAULT_CHANNEL_VOLUME 99
#define DEFAULT_PREVIEW_VOLUME 60

#define DEFAULT_MASTER_VOLUME 60

#define DATA_UNUSED_VALUE 0xFF

Project::Project(const char *name)
    : Persistent("Project"), VariableContainer(&variables_), song_(),
      tempoNudge_(0), tempo_(FourCC::VarTempo, DEFAULT_TEMPO),
      masterVolume_(FourCC::VarMasterVolume, DEFAULT_MASTER_VOLUME),
      channelVolume1_(FourCC::VarChannel1Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume2_(FourCC::VarChannel2Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume3_(FourCC::VarChannel3Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume4_(FourCC::VarChannel4Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume5_(FourCC::VarChannel5Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume6_(FourCC::VarChannel6Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume7_(FourCC::VarChannel7Volume, DEFAULT_CHANNEL_VOLUME),
      channelVolume8_(FourCC::VarChannel8Volume, DEFAULT_CHANNEL_VOLUME),
      wrap_(FourCC::VarWrap, false), transpose_(FourCC::VarTranspose, 0),
      scale_(FourCC::VarScale, scaleNames, numScales, 0),
      scaleRoot_(FourCC::VarScaleRoot, noteNames, 12, 0),
      projectName_(FourCC::VarProjectName, name),
      previewVolume_(FourCC::VarPreviewVolume, DEFAULT_PREVIEW_VOLUME) {

  this->variables_.insert(variables_.end(), &tempo_);
  this->variables_.insert(variables_.end(), &masterVolume_);

  // Add individual channel volume variables to the container
  this->variables_.insert(variables_.end(), &channelVolume1_);
  this->variables_.insert(variables_.end(), &channelVolume2_);
  this->variables_.insert(variables_.end(), &channelVolume3_);
  this->variables_.insert(variables_.end(), &channelVolume4_);
  this->variables_.insert(variables_.end(), &channelVolume5_);
  this->variables_.insert(variables_.end(), &channelVolume6_);
  this->variables_.insert(variables_.end(), &channelVolume7_);
  this->variables_.insert(variables_.end(), &channelVolume8_);

  this->variables_.insert(variables_.end(), &wrap_);
  this->variables_.insert(variables_.end(), &transpose_);
  this->variables_.insert(variables_.end(), &scale_);
  scale_.SetInt(0);
  this->variables_.insert(variables_.end(), &scaleRoot_);
  scaleRoot_.SetInt(0); // Default to C (0)
  this->variables_.insert(variables_.end(), &projectName_);
  this->variables_.insert(variables_.end(), &previewVolume_);

  // Project name is now managed through the WatchedVariable

  // look if we can find a sav file

  // Makes sure the tables exists for restoring

  TableHolder::GetInstance();

  Groove::GetInstance()->Clear();

  tempoTapCount_ = 0;

  Status::Set("About to load project");
}

Project::~Project() {}

void Project::Load(const char *name) {
  tempoNudge_ = 0;
  tempoTapCount_ = 0;
  for (int i = 0; i < MAX_TAP; i++) {
    lastTap_[i] = 0;
  }

  song_.Reset();
  instrumentBank_.Reset();
  Groove::GetInstance()->Clear();

  tempo_.Reset();
  masterVolume_.Reset();
  channelVolume1_.Reset();
  channelVolume2_.Reset();
  channelVolume3_.Reset();
  channelVolume4_.Reset();
  channelVolume5_.Reset();
  channelVolume6_.Reset();
  channelVolume7_.Reset();
  channelVolume8_.Reset();
  wrap_.Reset();
  transpose_.Reset();
  scale_.Reset();
  scaleRoot_.Reset();
  previewVolume_.Reset();

  if (name) {
    projectName_.SetString(name, true);
  } else {
    projectName_.SetString("", true);
  }

  Status::Set("About to load project");
}

int Project::GetScale() {
  Variable *v = FindVariable(FourCC::VarScale);
  NAssert(v);
  return v->GetInt();
}

uint8_t Project::GetScaleRoot() {
  Variable *v = FindVariable(FourCC::VarScaleRoot);
  NAssert(v);
  return v->GetInt();
}

int Project::GetTempo() {
  Variable *v = FindVariable(FourCC::VarTempo);
  NAssert(v);
  int tempo = v->GetInt() + tempoNudge_;
  return tempo;
}

int Project::GetMasterVolume() {
  Variable *v = FindVariable(FourCC::VarMasterVolume);
  NAssert(v);
  return v->GetInt();
}

int Project::GetChannelVolume(int channel) {
  // Return the appropriate channel volume variable
  switch (channel) {
  case 0:
    return channelVolume1_.GetInt();
  case 1:
    return channelVolume2_.GetInt();
  case 2:
    return channelVolume3_.GetInt();
  case 3:
    return channelVolume4_.GetInt();
  case 4:
    return channelVolume5_.GetInt();
  case 5:
    return channelVolume6_.GetInt();
  case 6:
    return channelVolume7_.GetInt();
  case 7:
    return channelVolume8_.GetInt();
  default:
    NAssert(false);
    return 0;
  }
}

void Project::GetProjectName(char *name) {
  Variable *v = FindVariable(FourCC::VarProjectName);
  strncpy(name, v->GetString().c_str(), MAX_PROJECT_NAME_LENGTH);
  name[MAX_PROJECT_NAME_LENGTH] = '\0';
}

void Project::SetProjectName(char *name) {
  Variable *v = FindVariable(FourCC::VarProjectName);
  char buffer[MAX_PROJECT_NAME_LENGTH + 1];
  strncpy(buffer, name, MAX_PROJECT_NAME_LENGTH);
  buffer[MAX_PROJECT_NAME_LENGTH] = '\0';
  v->SetString(buffer, true);
}

void Project::NudgeTempo(int value) {
  if ((GetTempo() + tempoNudge_) > 0) {
    tempoNudge_ += value;
  }
}

void Project::Trigger() {
  if (tempoNudge_ != 0) {
    if (tempoNudge_ > 0) {
      tempoNudge_--;
    } else {
      tempoNudge_++;
    };
  }
}

int Project::GetTranspose() {
  Variable *v = FindVariable(FourCC::VarTranspose);
  NAssert(v);
  int result = v->GetInt();
  if (result > 0x80) {
    result -= 128;
  }
  return result;
}

bool Project::Wrap() {
  Variable *v = FindVariable(FourCC::VarWrap);
  NAssert(v);
  return v->GetBool();
}

InstrumentBank *Project::GetInstrumentBank() { return &instrumentBank_; };

void Project::Update(Observable &o, I_ObservableData *d) {
  // Nothing to do here for now
}

void Project::Purge() {

  song_.chain_.ClearAllocation();
  song_.phrase_.ClearAllocation();

  for (int i = 0; i < SONG_ROW_COUNT; i++) {
    for (int j = 0; j < SONG_CHANNEL_COUNT; j++) {
      uint8_t v = song_.rows_[i].chains[j];
      if (v != DATA_UNUSED_VALUE) {
        song_.chain_.SetUsed(v);
      }
    }
  }

  for (int i = 0; i < CHAIN_COUNT; i++) {
    if (song_.chain_.IsUsed(i)) {
      for (int j = 0; j < PHRASES_PER_CHAIN; j++) {
        uint8_t p = song_.chain_.steps_[i][j].phrase;
        if (p != DATA_UNUSED_VALUE) {
          song_.phrase_.SetUsed(p);
        }
      }
    } else {
      for (int j = 0; j < PHRASES_PER_CHAIN; j++) {
        song_.chain_.steps_[i][j].phrase = DATA_UNUSED_VALUE;
        song_.chain_.steps_[i][j].transpose = 0x00;
      }
    }
  }

  static const uint8_t kNone = static_cast<uint8_t>(static_cast<char>(FourCC::InstrumentCommandNone));
  for (int i = 0; i < PHRASE_COUNT; i++) {
    for (int j = 0; j < 16; j++) {
      if (!song_.phrase_.IsUsed(i)) {
        PhraseStep *step = &(song_.phrase_.steps_[i][j]);
        step->note = DATA_UNUSED_VALUE;
        step->instrument = DATA_UNUSED_VALUE;
        step->cmd1 = kNone;
        step->param1 = 0;
        step->cmd2 = kNone;
        step->param2 = 0;
      }
    };
  }
}

// Returns true if sample is used by at least 1 Sampler instrument
bool Project::SampleInUse(
    etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> filename) {
  InstrumentBank *bank = GetInstrumentBank();

  for (int i = 0; i < MAX_INSTRUMENT_COUNT; i++) {
    I_Instrument *instrument = bank->GetInstrument(i);
    if (instrument->GetType() == IT_SAMPLE) {
      SampleInstrument *si = (SampleInstrument *)instrument;
      auto instrSample = si->GetSampleFileName();
      if (instrSample == filename) {
        return true;
      }
    };
  }
  return false;
}

// remove all samples that are not in use by an instrument
void Project::PurgeSamples() {
  // clear used flag
  bool isUsed[MAX_SAMPLES] = {false};

  // flag all samples actually used
  InstrumentBank *bank = GetInstrumentBank();
  for (int i = 0; i < MAX_INSTRUMENT_COUNT; i++) {
    I_Instrument *instrument = bank->GetInstrument(i);
    if (instrument->GetType() == IT_SAMPLE) {
      SampleInstrument *si = (SampleInstrument *)instrument;
      int index = si->GetSampleIndex();
      if (index >= 0)
        isUsed[index] = true;
    };
  }

  // Now remove all unused samples from disk
  int purged = 0;
  SamplePool *sp = SamplePool::GetInstance();
  for (int i = 0; i < MAX_SAMPLES; i++) {
    if ((!isUsed[i]) && (sp->GetSource(i - purged))) {
      sp->PurgeSample(i - purged, projectName_.GetString().c_str());
      Trace::Debug("Purged sample [%d]", i - purged);
      purged++;
    } else {
      Trace::Debug("Sample [%d] not purged", i);
    }
  };
  Trace::Debug("Purged %d samples", purged);
}

void Project::PurgeInstruments() {

  bool used[MAX_INSTRUMENT_COUNT] = {false};
  for (int i = 0; i < PHRASE_COUNT; i++) {
    for (int j = 0; j < 16; j++) {
      uint8_t instr = song_.phrase_.steps_[i][j].instrument;
      if (instr != DATA_UNUSED_VALUE) {
        NAssert(instr < MAX_INSTRUMENT_COUNT);
        used[instr] = true;
      }
    }
  }

  InstrumentBank *bank = GetInstrumentBank();
  for (int i = 0; i < MAX_INSTRUMENT_COUNT; i++) {
    if (!used[i]) {
      I_Instrument *instrument = bank->GetInstrument(i);
      if (instrument->GetType() == IT_SAMPLE) {
        SampleInstrument *sampleInstrument =
            static_cast<SampleInstrument *>(instrument);
        sampleInstrument->AssignSample(NO_SAMPLE);
      }
      // we dont reorder indexes on release so safe to call inside this loop
      bank->releaseInstrument(i);
      Trace::Debug("Set unused instrument slot [%d] to NONE", i);
    }
  }
}

void Project::RestoreContent(PersistencyDocument *doc) {
  bool attr = doc->NextAttribute();
  doc->version_ = 32;
  int tableRatio = 0;
  while (attr) {
    if (!strcmp(doc->attrname_, XML_ATTR_VERSION)) {
      doc->version_ = int(atof(doc->attrval_) * 100);
    }
    if (!strcmp(doc->attrname_, XML_ATTR_TABLE_RATIO)) {
      tableRatio = atoi(doc->attrval_);
    }
    attr = doc->NextAttribute();
  }
  if (!tableRatio)
    tableRatio = (doc->version_ <= 32) ? 2 : 1;
  SyncMaster::GetInstance()->SetTableRatio(tableRatio);

  // Now loop on all variables
  bool elem = doc->FirstChild();
  while (elem) {
    bool attr = doc->NextAttribute();
    char name[MAX_VARIABLE_STRING_LENGTH + 1];
    char value[MAX_VARIABLE_STRING_LENGTH + 1];
    while (attr) {
      if (!strcmp(doc->attrname_, XML_ATTR_NAME)) {
        strcpy(name, doc->attrval_);
      }
      if (!strcmp(doc->attrname_, XML_ATTR_VALUE)) {
        strcpy(value, doc->attrval_);
      }
      attr = doc->NextAttribute();
    }
    Variable *v = FindVariable(name);
    // Project name now comes from the directory, so ignore any persisted value.
    if (v && v->GetID() != FourCC::VarProjectName) {
      v->SetString(value);
    }
    elem = doc->NextSibling();
  }
}

void Project::SaveContent(tinyxml2::XMLPrinter *printer) {

  // store project version
  printer->PushAttribute(XML_ATTR_VERSION, PROJECT_NUMBER);

  // store table ratio if not one
  int tableRatio = SyncMaster::GetInstance()->GetTableRatio();
  if (tableRatio != 1) {
    printer->PushAttribute(XML_ATTR_TABLE_RATIO, tableRatio);
  }

  // save all of the project's parameters
  auto it = variables_.begin();
  for (size_t i = 0; i < variables_.size(); i++) {
    Variable *currentVar = *it;
    // Persist everything except the project name, which is derived from the
    // projects directory name
    if (currentVar->GetID() == FourCC::VarProjectName) {
      it++;
      continue;
    }

    printer->OpenElement(XML_ELEM_PARAMETER);
    printer->PushAttribute(XML_ATTR_NAME, currentVar->GetName());
    printer->PushAttribute(XML_ATTR_VALUE, currentVar->GetString().c_str());
    printer->CloseElement();
    it++;
  }
}

void Project::OnTempoTap() {

  unsigned long now = System::GetInstance()->GetClock();

  if (tempoTapCount_ != 0) {
    // count last tick tempo and see if in range
    unsigned millisec = now - lastTap_[tempoTapCount_ - 1];
    int t = int(60000 / (float)millisec);
    if (t > 30) {
      if (tempoTapCount_ == MAX_TAP) {
        for (int i = 0; i < int(tempoTapCount_) - 1; i++) {
          lastTap_[i] = lastTap_[i + 1];
        }
      } else {
        tempoTapCount_++;
      }
      int tempo =
          int(60000 * (tempoTapCount_ - 1) / (float)(now - lastTap_[0]));
      Variable *v = FindVariable(FourCC::VarTempo);
      // ensure tempo is within range
      tempo = std::clamp((unsigned short)tempo, MIN_TEMPO, MAX_TEMPO);
      v->SetInt(tempo);
    } else {
      tempoTapCount_ = 1;
    }
  } else {
    tempoTapCount_ = 1;
  }
  lastTap_[tempoTapCount_ - 1] = now;
}
