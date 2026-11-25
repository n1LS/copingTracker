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

#include "InstrumentBank.h"
#include "Application/Instruments/MidiInstrument.h"
#include "Application/Instruments/SampleInstrument.h"
#include "Application/Instruments/SamplePool.h"
#include "Application/Model/Config.h"
#include "Application/Persistency/PersistencyService.h"
#include "Application/Player/PlayerMixer.h"
#include "Application/Utils/char.h"
#include "ChiptuneInstrument.h"
#include "DrumInstrument.h"
#include "Filters.h"
#include "LSDJKitInstrument.h"
#include "MidiInstrument.h"
#include "StackInstrument.h"
#include "System/io/Status.h"

#define XML_DEBUG_LOGGING 1

// Contain all instrument definition
InstrumentBank::InstrumentBank() : Persistent("InstrumentBank"), instrumentPool_() {

  for (size_t i = 0; i < instruments_.max_size(); i++) {
    instruments_[i] = &none_;
  }

  Status::Set("All instruments preloaded");
}

InstrumentBank::~InstrumentBank() {
  Reset();
}

void InstrumentBank::Reset() {
  instrumentPool_.release_all();

  // all instruments are gone, their addresses will be reused by new ones
  I_Instrument::ReleaseAllVoices();

  for (size_t i = 0; i < instruments_.max_size(); i++) {
    instruments_[i] = &none_;
  }
  sidOscCount = 0;
}

I_Instrument *InstrumentBank::GetInstrument(int i) {
  return (i != NO_INSTRUMENT) ? instruments_[i] : nullptr;
}

// "instruments": [{"id": 3, "type": "sample", ...}] of the instruments that are not empty, the id and the type come
// first: the instrument is created from them before the rest is read
void InstrumentBank::SaveContent(JsonWriter &writer) {
  writer.BeginArray("instruments");
  int i = 0;
  for (auto &instr : instruments_) {
    if (!instr->IsEmpty()) {
      writer.BeginObject();
      writer.Int("id", i);
      // Let the instrument save its own content
      instr->SaveContent(writer);
      writer.EndObject();
    }
    i++;
  }
  writer.EndArray();
}

void InstrumentBank::RestoreContent(JsonReader &reader) {
  while (reader.NextKey()) {
    if (!reader.KeyIs("instruments")) {
      reader.Skip();
      continue;
    }
    if (!reader.BeginArray()) {
      return;
    }
    while (reader.NextElement()) {
      if (!reader.BeginObject()) {
        return;
      }
      int id = -1;
      InstrumentType instrType = IT_SAMPLE; // default if no type in the file
      I_Instrument *instr = nullptr;

      // the instrument is created when the first member that is not its id or type shows up
      auto createInstrument = [&]() {
        if (instr || id < 0 || id >= MAX_INSTRUMENT_COUNT) {
          return;
        }
        if (AssignInstrumentToSlot(instrType, id) != InstrumentAssignResult::Success) {
          Trace::Error("Failed to allocate instrument type:%d", instrType);
          // TODO: need to show user error message that proj file is invalid
          return;
        }
        instr = instruments_[id];
      };

      while (reader.NextKey()) {
        if (reader.KeyIs("id")) {
          id = reader.ReadInt();
        } else if (reader.KeyIs("type")) {
          char type[16];
          reader.ReadString(type, sizeof(type));
          for (uint32_t t = 0; t < IT_LAST; t++) {
            if (!strcasecmp(type, InstrumentTypeNames[t].persistence)) {
              instrType = (InstrumentType)t;
              break;
            }
          }
        } else {
          createInstrument();
          if (!instr || !instr->RestoreKey(reader)) {
            reader.Skip();
          }
        }
      }
      createInstrument();
      if (instr) {
        instr->RestoreFinished();
      }
    }
  }
}

void InstrumentBank::Init() {
}

// Get the next available instance of the given Instrument type from the pool of
// unused Instruments and assign it to the given instrument "slot id"
InstrumentAssignResult InstrumentBank::AssignInstrumentToSlot(InstrumentType type, uint8_t id) {
  I_Instrument *current = nullptr;

  switch (type) {
    case IT_SAMPLE:
      current = instrumentPool_.create<SampleInstrument>();
      break;
    case IT_MIDI:
      current = instrumentPool_.create<MidiInstrument>();
      break;
    case IT_CHIPTUNE:
      current = instrumentPool_.create<ChiptuneInstrument>();
      break;
    case IT_DRUM:
      current = instrumentPool_.create<DrumInstrument>();
      break;
    case IT_LSDJKIT:
      current = instrumentPool_.create<LSDJKitInstrument>();
      break;
    case IT_STACK:
      current = instrumentPool_.create<StackInstrument>();
      break;
    case IT_NONE:
      instruments_[id] = &none_;
      return InstrumentAssignResult::Success;
    default:
      break;
  }

  if (current == nullptr) {
    Trace::Error("Instrument pool exhausted");
    return InstrumentAssignResult::PoolExhausted;
  }

  if (!current->Init()) {
    Trace::Error("Failed to initialize new %s instrument of type", InstrumentTypeNames[type].full);
    purgeInstrument(current);
    return InstrumentAssignResult::InitFailed;
  }

  instruments_[id] = current;
  return InstrumentAssignResult::Success;
}

void InstrumentBank::purgeInstrument(I_Instrument *instrument) {
  // the pool reuses this address, a new instrument must not inherit the voices
  I_Instrument::ReleaseVoices(instrument);

  switch (instrument->GetType()) {
    case IT_SAMPLE:
      instrumentPool_.destroy(static_cast<SampleInstrument *>(instrument));
      break;
    case IT_MIDI:
      instrumentPool_.destroy(static_cast<MidiInstrument *>(instrument));
      break;
    case IT_CHIPTUNE:
      instrumentPool_.destroy(static_cast<ChiptuneInstrument *>(instrument));
      break;
    case IT_DRUM:
      instrumentPool_.destroy(static_cast<DrumInstrument *>(instrument));
      break;
    case IT_LSDJKIT:
      instrumentPool_.destroy(static_cast<LSDJKitInstrument *>(instrument));
      break;
    case IT_STACK:
      instrumentPool_.destroy(static_cast<StackInstrument *>(instrument));
      break;
    case IT_NONE:
      // NA: None is a "singleton" so no need to release from pool
      // BUT it can be assigned to any number of slots
    default:
      break;
  }
}

void InstrumentBank::releaseInstrument(uint16_t id) {
  auto instrument = instruments_[id];

  // The player may have picked this instrument up after the caller decided to
  // replace it (eg. while a confirmation dialog was open). Detach it from any
  // channel first: purgeInstrument() runs the destructor and hands the memory
  // back to the pool, so a channel still holding the pointer would render
  // freed memory.
  PlayerMixer::GetInstance()->ReleaseInstrument(instrument);

  purgeInstrument(instrument);
  instruments_[id] = &none_;
}

uint16_t InstrumentBank::GetNextFreeInstrumentSlotId() {
  for (uint16_t i = 0; i < instruments_.max_size(); i++) {
    if (instruments_[i] == &none_) {
      return i;
    }
  }
  return NO_MORE_INSTRUMENT;
}

uint16_t InstrumentBank::Clone(uint16_t i) {
  I_Instrument *src = instruments_[i];

  // Find next available instrument slot
  auto nextFreeInstrumentSlotId = GetNextFreeInstrumentSlotId();

  if (nextFreeInstrumentSlotId == NO_MORE_INSTRUMENT) {
    return NO_MORE_INSTRUMENT;
  }

  InstrumentAssignResult result = AssignInstrumentToSlot(src->GetType(), nextFreeInstrumentSlotId);

  if (result != InstrumentAssignResult::Success) {
    return NO_MORE_INSTRUMENT;
  }

  I_Instrument *dst = instruments_[nextFreeInstrumentSlotId];

  // sanity check not trying to clone into itself
  if (src == dst) {
    return NO_MORE_INSTRUMENT;
  }

  for (auto it = src->Variables()->begin(); it != src->Variables()->end(); it++) {
    Variable *dstV = dst->FindVariable((*it)->GetID());
    if (dstV) {
      dstV->CopyFrom(**it);
    }
  }
  dst->CopyEffectsFrom(*src);
  return nextFreeInstrumentSlotId;
}

void InstrumentBank::OnStart() {
  for (auto &elem : instruments_) {
    elem->OnStart();
  }
  init_filters();
}

uint32_t InstrumentBank::UsedInstrumentCount() const {
  uint32_t count = 0;
  for (auto &elem : instruments_) {
    if (!elem->IsEmpty()) {
      count++;
    }
  }
  return count;
}

void InstrumentBank::OnSampleRemoved(int removedIndex) {
  // Iterate through all instruments and update their sample references
  for (auto &instrument : instruments_) {
    // Only need to check SampleInstrument types
    if (instrument->GetType() == IT_SAMPLE) {
      SampleInstrument *sampleInstr = static_cast<SampleInstrument *>(instrument);

      // Get the current sample index from the instrument's variable
      Variable *vSample = sampleInstr->FindVariable(Token::SampleInstrumentSample);
      if (vSample) {
        int currentSampleIndex = vSample->GetInt();

        if (currentSampleIndex == removedIndex) {
          // This instrument was using the removed sample - set to NO_SAMPLE
          vSample->SetInt(NO_SAMPLE);
          Trace::Log("InstrumentBank", "Instrument %p sample cleared (was using removed sample %d)", sampleInstr,
                     removedIndex);
        } else if (currentSampleIndex > removedIndex) {
          // This instrument was using a sample that shifted - decrement index
          vSample->SetInt(currentSampleIndex - 1);
          Trace::Log("InstrumentBank", "Instrument %p sample index updated from %d to %d", sampleInstr,
                     currentSampleIndex, currentSampleIndex - 1);
        }
        // If currentSampleIndex < removedIndex, no change needed
      }
    }
  }
}

NoneInstrument *InstrumentBank::noneInstrument() {
  return &none_;
}
