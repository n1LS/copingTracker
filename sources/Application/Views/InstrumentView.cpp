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

#include "InstrumentView.h"
#include "Application/Instruments/GMBank_data.generated.h"
#include "Application/Instruments/MidiInstrument.h"
#include "Application/Instruments/SIDInstrument.h"
#include "Application/Instruments/SampleInstrument.h"
#include "Application/Instruments/SamplePool.h"
#include "Application/Instruments/StackInstrument/StackInstrument.h"
#include "Application/Model/Config.h"
#include "Application/Views/SampleEditorView.h"
#include "Application/Views/SampleImportView.h"
#include "BaseClasses/UIHexVarField.h"
#include "BaseClasses/UIIntVarField.h"
#include "BaseClasses/UIIntVarOffField.h"
#include "BaseClasses/UINoteVarField.h"
#include "BaseClasses/UIStaticField.h"
#include "Foundation/Constants/SpecialCharacters.h"
#include "ModalDialogs/MessageBox.h"
#include "System/System/System.h"
#include <Application/Utils/stringutils.h>
#include <cstdint>
#include <etl/format_spec.h>
#include <etl/to_string.h>
#include <nanoprintf.h>

#define sub_item " " char_border_single_verticalRight_s char_border_single_horizontal_s " "
#define last_sub_item " " char_border_single_bottomLeft_s char_border_single_horizontal_s " "

static constexpr InstrumentType kMaxSelectableInstrumentType = static_cast<InstrumentType>(IT_LAST - 1);

InstrumentView::InstrumentView(GUIWindow &w, ViewData *data)
    : FieldView(w, data), instrumentType_(Token::VarInstrumentType, LongInstrumentNames, IT_LAST, 0),
      lastSampleIndex_(-1), suppressSampleChangeWarning_(false) {
  project_ = data->project_;

  static const char *tabs[] = {"-", "Smpl", "Chip", "Drum", "Stck", "MIDI"}; // TODO nILS: grab from from InstrumentType

  GUIPoint position = GUIPoint(0, 2);
  typeVarField_.emplace_back("Type", position, *&instrumentType_, tabs, 6);
  fieldList_.insert(fieldList_.end(), &typeVarField_.back());
  typeVarField_.back().AddObserver(*this);
  typeVarField_.back().SetBackgroundColor(Theme::View::inactive);
  lastFocus_ = &typeVarField_.back();
  position.x_ += 5;

  // Create the name field with the actual instrument variable
  I_Instrument *instr = getInstrument();
  if (instr) {
    // NONE dont have a name field
    if (instr->GetType() != IT_NONE) {
      position.x_ = 5;
      position.y_ = 3;
      addNameTextField(instr, position);
    }
  }

  // add ui action fields for exporting and importing instrument settings and modulation
  position = GUIPoint(23, 4);
  persistentActionField_.emplace_back(char_symbol_load_s, Token::ActionImport, position);
  persistentActionField_.back().SetBackgroundColor(Theme::View::inactive);
  persistentActionField_.back().SetFieldConfiguration(instrumentActionFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &persistentActionField_.back());
  persistentActionField_.back().AddObserver(*this);

  position = GUIPoint(26, 4);
  persistentActionField_.emplace_back(char_symbol_save_s, Token::ActionExport, position);
  persistentActionField_.back().SetBackgroundColor(Theme::View::inactive);
  persistentActionField_.back().SetFieldConfiguration(instrumentActionFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &persistentActionField_.back());
  persistentActionField_.back().AddObserver(*this);

  position = GUIPoint(29, 4);
  persistentActionField_.emplace_back(char_mod_s, Token::ActionModulation, position);
  persistentActionField_.back().SetBackgroundColor(Theme::View::inactive);
  persistentActionField_.back().SetFieldConfiguration(instrumentActionFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &persistentActionField_.back());
  persistentActionField_.back().AddObserver(*this);
}

InstrumentView::~InstrumentView() {
}

void InstrumentView::Reset() {
  lastSampleIndex_ = -1;
  suppressSampleChangeWarning_ = false;
  exportInstrument_ = nullptr;
  exportName_.clear();
  lastFocus_ = &typeVarField_.back();
  instrumentType_.SetInt(0, false);
}

void InstrumentView::updateSliceCount(SampleInstrument *instrument) {
  sliceCount_ = 0;
  if (instrument) {
    for (size_t i = 0; i < SampleInstrument::MaxSlices; ++i) {
      if (instrument->IsSliceDefined(i)) {
        sliceCount_++;
      }
    }
  }
}

void InstrumentView::addNameTextField(I_Instrument *instr, const GUIPoint &position) {
  nameVariables_.emplace_back(instr);
  Variable &nameVar = *nameVariables_.rbegin();

  // Use an empty default name - we don't want to populate with sample filename
  // The display name will still be shown on the phrase screen via
  // GetDisplayName()
  etl::string<MAX_INSTRUMENT_NAME_LENGTH> defaultName;

  nameTextField_.emplace_back(nameVar, position, "Name", Token::InstrumentName, defaultName);
  nameTextField_.back().SetFieldConfiguration(panelFieldConfiguration);
  nameTextField_.back().SetBackgroundColor(Theme::View::inactive);
  fieldList_.insert(fieldList_.end(), &nameTextField_.back());
}

I_Instrument *InstrumentView::getInstrument() {
  int id = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  return bank->GetInstrument(id);
}

void InstrumentView::onInstrumentTypeChange(bool updateUI) {
  auto nuType = (InstrumentType)instrumentType_.GetInt();
  I_Instrument *old = getInstrument();

  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();

  auto id = viewData_->currentInstrumentID_;
  // release prev instrument back to available pool
  if (old != nullptr) {
    // first check if the instrument type actually changed, because could be
    // user is at end of instrument list and just keeps pressing key combo to
    // trigger next instrument event again and again
    if (old->GetType() == nuType) {
      if (updateUI) {
        refreshInstrumentFields();
      }
      return;
    }
    bank->releaseInstrument(viewData_->currentInstrumentID_);
  }

  // now assign new instrument type to the current instrument slot id
  InstrumentAssignResult result = bank->AssignInstrumentToSlot(nuType, id);

  // lambda to clear on fail
  auto setCurrentInstrumentToNone = [&]() {
    bank->AssignInstrumentToSlot(IT_NONE, id);
    instrumentType_.SetInt(IT_NONE, false);
  };

  // initialization failed, could not initialize this instruments, let's try the
  // next type
  if (result == InstrumentAssignResult::PoolExhausted) {
    Trace::Error("Instrument pool exhausted.");
    setCurrentInstrumentToNone();
  } else if (result == InstrumentAssignResult::InitFailed) {
    Trace::Error("Failed to initialize new instrument of type %d", nuType);

    // try to find the next available instrument type
    bool found = false;
    for (int i = nuType + 1; i <= static_cast<int>(kMaxSelectableInstrumentType); i++) {
      InstrumentType nextType = static_cast<InstrumentType>(i);
      result = bank->AssignInstrumentToSlot(nextType, id);

      if (result == InstrumentAssignResult::PoolExhausted) {
        // no more instruments available
        break;
      } else if (result == InstrumentAssignResult::InitFailed) {
        // failed to initialize this type, keep going
        Trace::Error("Failed to initialize new instrument of type %d", nextType);
        continue;
      }

      Trace::Log("INSTRUMENTVIEW", "Assigned next available type: %d", nextType);
      instrumentType_.SetInt(nextType, false);
      found = true;
      // Success, stop searching
      break;
    }

    if (!found) {
      Trace::Error("Failed to initialize any instrument type, defaulting to NONE");
      setCurrentInstrumentToNone();
    }
  }

  // Get the new instrument after type change
  I_Instrument *newInstr = getInstrument();
  if (newInstr) {
    Trace::Log("INSTRUMENTVIEW", "New instrument type: %d", newInstr->GetType());
  }

  // Refresh the UI fields for the new instrument type
  refreshInstrumentFields();

  // Mark the view as dirty to ensure it gets themen
  isDirty_ = true;
}

void InstrumentView::applyProposedTypeChangeUI() {
  instrumentType_.SetInt(pendingInstrumentType_, false);
  onInstrumentTypeChange();
}

void InstrumentView::onInstrumentChange() {
  ClearFocus();

  I_Instrument *old = getInstrument();
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();

  if (getInstrument() != old) {
    getInstrument()->RemoveObserver(*this);
  };

  // update type field to match current instrument
  ((WatchedVariable *)&instrumentType_)->SetInt(getInstrument()->GetType());

  refreshInstrumentFields();
}

void InstrumentView::refreshInstrumentFields() {
  for (auto &f : intVarField_) {
    f.RemoveObserver(*this);
  }
  for (auto &f : hexVarField_) {
    f.RemoveObserver(*this);
  }
  for (auto &f : intVarOffField_) {
    f.RemoveObserver(*this);
  }
  for (auto &f : bitmaskVarField_) {
    f.RemoveObserver(*this);
  }
  for (auto &f : sampleActionField_) {
    f.RemoveObserver(*this);
  }

  fieldList_.clear();
  intVarField_.clear();
  noteVarField_.clear();
  staticField_.clear();
  hexVarField_.clear();
  intVarOffField_.clear();
  sampleActionField_.clear();
  bitmaskVarField_.clear();
  nameTextField_.clear();
  nameVariables_.clear();
  lastSampleIndex_ = -1;

  sampleInputField_ = nullptr;
  gmInputField_ = nullptr;

  // first put back the type field as its shown on *all* instrument types
  fieldList_.insert(fieldList_.end(), &typeVarField_.back());
  lastFocus_ = &typeVarField_.back();

  // Re-add the action fields for export and import only if not IT_NONE
  if (instrumentType_.GetInt() != IT_NONE) {
    for (auto &action : persistentActionField_) {
      fieldList_.insert(fieldList_.end(), &action);
      action.AddObserver(*this); // Make sure observers are re-added
    }
  } else {
    // add back only the import field for IT_NONE
    // bit of a hack !!since we just assume that import is the first action
    // field
    fieldList_.insert(fieldList_.end(), &(*persistentActionField_.begin()));
    persistentActionField_.back().AddObserver(*this);
  }

  // Create a new nameTextField_ if the instrument type supports it
  if (instrumentType_.GetInt() != IT_NONE) {
    I_Instrument *instr = getInstrument();
    if (instr) {
      addNameTextField(instr, GUIPoint(0, 4));
    }
  }

  InstrumentType it = getInstrument()->GetType();
  switch (it) {
    case IT_NONE:
      fillNoneParameters();
      break;
    case IT_MIDI:
      fillMidiParameters();
      break;
    case IT_SID:
      fillSIDParameters();
      break;
    case IT_SAMPLE:
      fillSampleParameters();
      break;
    case IT_OPAL:
      fillOpalParameters();
      break;
    case IT_CHIPTUNE:
      fillChiptuneParameters();
      break;
    case IT_DRUM:
      fillDrumParameters();
      break;
    case IT_STACK:
      fillStackParameters();
      break;
    case IT_LAST:
      // NA
      break;
  };

  for (auto field : fieldList_) {
    if (field == lastFocus_) {
      SetFocus(field);
      break;
    }
  }

  // observer all var fields so we can mark the instrument as modified
  // to be able to show confirmation dialog when switching instrument type
  for (auto &f : intVarField_) {
    f.AddObserver(*this);
  }
  for (auto &f : hexVarField_) {
    f.AddObserver(*this);
  }
  for (auto &f : intVarOffField_) {
    f.AddObserver(*this);
  }
  for (auto &f : bitmaskVarField_) {
    f.AddObserver(*this);
  }

  getInstrument()->AddObserver(*this);
}

void InstrumentView::fillNoneParameters() {
}

void InstrumentView::fillSIDParameters() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  SIDInstrument *instrument = (SIDInstrument *)bank->GetInstrument(i);
  GUIPoint position = GetAnchor();

  // offset y to account for instrument type, name and export/import fields
  position.y_ += 1;

  staticField_.emplace_back(position, instrument->GetChipName());
  fieldList_.insert(fieldList_.end(), &staticField_.back());

  position.y_ += 2;
  staticField_.emplace_back(position, "Oscillator Settings" char_line_5_s);
  fieldList_.insert(fieldList_.end(), &staticField_.back());

  position.y_ += 2;
  Variable *v = instrument->FindVariable(Token::SIDInstrumentOSCNumber);
  intVarField_.emplace_back(position, *v, "Oscillator    :%1.1X", 0, 0x2, 1, 1);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  v = instrument->FindVariable(Token::SIDInstrumentPulseWidth);
  intVarField_.emplace_back(position, *v, sub_item "Pulsewidth:%2.2X", 0, 0xFFF, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  v = instrument->FindVariable(Token::SIDInstrumentWaveform);

  intVarField_.emplace_back(position, *v, sub_item "Waveform  :%s", 0, DWF_LAST - 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  v = instrument->FindVariable(Token::SIDInstrumentVSync);
  intVarField_.emplace_back(position, *v, sub_item "Osc Sync  :%s", 0, 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  v = instrument->FindVariable(Token::SIDInstrumentRingModulator);
  intVarField_.emplace_back(position, *v, last_sub_item "Ring Mod  :%s", 0, 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 2;
  v = instrument->FindVariable(Token::SIDInstrumentADSR);
  hexVarField_.emplace_back(UIHexVarField(position, *v, 4, "Env. A/D/S/R  :%4.4X", 0, 0xFFFF, 16, true));
  fieldList_.insert(fieldList_.end(), &hexVarField_.back());

  position.y_ += 2;
  staticField_.emplace_back(position, "Chip Settings" char_line_11_s);
  fieldList_.insert(fieldList_.end(), &staticField_.back());

  position.y_ += 2;
  v = instrument->FindVariable(Token::SIDInstrumentFilterOn);
  intVarField_.emplace_back(position, *v, "Filter        :%s", 0, 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  switch (instrument->GetChip()) {
    case SID1:
      v = instrument->FindVariable(Token::SIDInstrument1FilterCut);
      break;
    case SID2:
      v = instrument->FindVariable(Token::SIDInstrument2FilterCut);
      break;
  }
  intVarField_.emplace_back(position, *v, sub_item "Cutoff    :%1.1X", 0, 0x7FF, 1, 0x10);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  switch (instrument->GetChip()) {
    case SID1:
      v = instrument->FindVariable(Token::SIDInstrument1FilterResonance);
      break;
    case SID2:
      v = instrument->FindVariable(Token::SIDInstrument2FilterResonance);
      break;
  }
  intVarField_.emplace_back(position, *v, sub_item "Resonance :%1.1X", 0, 0xF, 1, 1);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  switch (instrument->GetChip()) {
    case SID1:
      v = instrument->FindVariable(Token::SIDInstrument1FilterMode);
      break;
    case SID2:
      v = instrument->FindVariable(Token::SIDInstrument2FilterMode);
      break;
  }
  intVarField_.emplace_back(position, *v, last_sub_item "Mode      :%s", 0, DFM_LAST - 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 2;
  switch (instrument->GetChip()) {
    case SID1:
      v = instrument->FindVariable(Token::SIDInstrument1Volume);
      break;
    case SID2:
      v = instrument->FindVariable(Token::SIDInstrument2Volume);
      break;
  }
  intVarField_.emplace_back(position, *v, "Volume        :%1.1X", 0, 0xF, 1, 1);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());
}

#include "InstrumentView_Chiptune.ipp"
#include "InstrumentView_Drum.ipp"
#include "InstrumentView_MIDI.ipp"
#include "InstrumentView_Sample.ipp"
#include "InstrumentView_Stack.ipp"

void InstrumentView::fillOpalParameters() {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  OpalInstrument *instrument = (OpalInstrument *)instr;
  GUIPoint position = GetAnchor();

  // extra y spacing to allow for gap between export/import and parameters
  position.y_ += 2;
  staticField_.emplace_back(position, "General Settings" char_line_8_s);
  fieldList_.insert(fieldList_.end(), &staticField_.back());

  position.y_ += 2;
  Variable *v = instrument->FindVariable(Token::OPALInstrumentAlgorithm);
  intVarField_.emplace_back(position, *v, "Algorithm     :%s", 0, 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  v = instrument->FindVariable(Token::OPALInstrumentDeepTremeloVibrato);
  bitmaskVarField_.emplace_back(UIBitmaskVarField(position, *v, "Deep Trem/Vib :%02b", 2));
  fieldList_.insert(fieldList_.end(), &bitmaskVarField_.back());

  position.y_ += 1;
  v = instrument->FindVariable(Token::OPALInstrumentFeedback);
  intVarField_.emplace_back(UIIntVarField(position, *v, "Feedback      :%1.1X", 0, 0x07, 1, 1, 0));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 2;
  staticField_.emplace_back(position, "Operator Settings" char_line_7_s);
  fieldList_.insert(fieldList_.end(), &staticField_.back());

  // operator settings
  position.y_ += 2;
  staticField_.emplace_back(position, "               Op 1" char_border_single_vertical_s "Op 2");
  fieldList_.insert(fieldList_.end(), &staticField_.back());

  position.y_ += 1;
  staticField_.emplace_back(position, "               " char_line_4_s char_border_single_cross_s char_line_4_s);
  fieldList_.insert(fieldList_.end(), &staticField_.back());

  // vertical table separator
  GUIPoint p = position + GUIPoint(19, 1);
  for (int n = 0; n < 6; n++) {
    staticField_.emplace_back(p, char_border_single_vertical_s);
    fieldList_.insert(fieldList_.end(), &staticField_.back());
    p.y_ += 1;
  }

  position.y_ += 1;
  v = instrument->FindVariable(Token::OPALInstrumentOp1Level);
  intVarField_.emplace_back(UIIntVarField(position, *v, "Level         :%2.2X", 0, 63, 1, 1, 0));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  v = instrument->FindVariable(Token::OPALInstrumentOp2Level);
  intVarField_.emplace_back(UIIntVarField(position + GUIPoint(20, 0), *v, "%2.2X", 0, 63, 1, 1, 0));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  v = instrument->FindVariable(Token::OPALInstrumentOp1Multiplier);
  intVarField_.emplace_back(UIIntVarField(position, *v, "Multiplier    :%1.1X", 0, 15, 1, 1, 0));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  v = instrument->FindVariable(Token::OPALInstrumentOp2Multiplier);
  intVarField_.emplace_back(UIIntVarField(position + GUIPoint(20, 0), *v, "%1.1X", 0, 15, 1, 1, 0));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  v = instrument->FindVariable(Token::OPALInstrumentOp1ADSR);
  hexVarField_.emplace_back(UIHexVarField(position, *v, 4, "A/D/S/R       :%4.4X", 0, 0xFFFF, 16, true));
  fieldList_.insert(fieldList_.end(), &hexVarField_.back());

  v = instrument->FindVariable(Token::OPALInstrumentOp2ADSR);
  hexVarField_.emplace_back(UIHexVarField(position + GUIPoint(20, 0), *v, 4, "%4.4X", 0, 0xFFFF, 16, true));
  fieldList_.insert(fieldList_.end(), &hexVarField_.back());

  position.y_ += 1;
  v = instrument->FindVariable(Token::OPALInstrumentOp1WaveShape);
  intVarField_.emplace_back(UIIntVarField(position, *v, "Shape         :%s", 0, 7, 1, 1));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  v = instrument->FindVariable(Token::OPALInstrumentOp2WaveShape);
  intVarField_.emplace_back(UIIntVarField(position + GUIPoint(20, 0), *v, "%s", 0, 7, 1, 1));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  position.y_ += 1;
  v = instrument->FindVariable(Token::OPALInstrumentOp1TremVibSusKSR);
  bitmaskVarField_.emplace_back(UIBitmaskVarField(position, *v, "TR/VB/SU/KSR  :%04b", 4));
  fieldList_.insert(fieldList_.end(), &bitmaskVarField_.back());

  v = instrument->FindVariable(Token::OPALInstrumentOp2TremVibSusKSR);
  bitmaskVarField_.emplace_back(UIBitmaskVarField(position + GUIPoint(20, 0), *v, "%04b", 4));
  fieldList_.insert(fieldList_.end(), &bitmaskVarField_.back());

  position.y_ += 1;
  v = instrument->FindVariable(Token::OPALInstrumentOp1KeyScaleLevel);
  intVarField_.emplace_back(UIIntVarField(position, *v, "Keyscale      :%s", 0, 3, 1, 1));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  v = instrument->FindVariable(Token::OPALInstrumentOp2KeyScaleLevel);
  intVarField_.emplace_back(UIIntVarField(position + GUIPoint(20, 0), *v, "%s", 0, 3, 1, 1));
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  Trace::Error("OPAL fill done, total fields: %d", fieldList_.size());
}

void InstrumentView::warpToNext(int offset) {
  int instrument = viewData_->currentInstrumentID_ + offset;
  if (instrument >= MAX_INSTRUMENT_COUNT) {
    instrument = instrument - MAX_INSTRUMENT_COUNT;
  };
  if (instrument < 0) {
    instrument = MAX_INSTRUMENT_COUNT + instrument;
  };
  viewData_->currentInstrumentID_ = instrument;
  onInstrumentChange();
  isDirty_ = true;
}

void InstrumentView::ProcessButtonMask(uint16_t mask, bool pressed) {
  if (AppWindow::GetInstance()->buttonDown(BM_NAV) != mapVisible_) {
    SetDirty(true);
  }

  if (!pressed) {
    return;
  }

  if (mask == (BM_EDIT | BM_ENTER)) {
    int i = viewData_->currentInstrumentID_;
    InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
    I_Instrument *instr = bank->GetInstrument(i);
    if (GetFocus() == *fieldList_.begin()) {
      bool instrumentModified = checkInstrumentModified();
      if (instrumentModified) {
        MessageBox *mb = MessageBox::Create(*this, "Reset", "Reset all settings?", MBBF_YES | MBBF_NO);
        pendingPurgeInstrument_ = instr;
        DoModal(mb, ModalViewCallback::create<InstrumentView, &InstrumentView::onConfirmResetInstrument>(*this));
      }
      return;
    }
    if (getInstrument()->GetType() == IT_SAMPLE) {
      UIIntVarField *field =
          (UIIntVarField *)GetFocus(); // TODO nILS: this is bad, it's not necessarily an UIIntVarField
      if (field->GetVariableID() == Token::SampleInstrumentEnd) {
        Variable *var = field->GetVariable();
        SampleInstrument *instrument = (SampleInstrument *)instr;
        var->SetInt(instrument->GetSampleSize() - 1);
        isDirty_ = true;
        return;
      };
    }
  }

  // Call the parent class's implementation first to ensure action fields like
  // Export, Import work correctly
  FieldView::ProcessButtonMask(mask, pressed);

  Player *player = Player::GetInstance();

  if (mask == BM_ENTER) {
    // Get the current field to check if we're on the sample field
    UIIntVarField *currentField =
        (UIIntVarField *)GetFocus(); // TODO nILS: this is bad, it's not necessarily an UIIntVarField
    Variable *var = currentField->GetVariable();

    if (var) {
      // Only allow sample import when the sample field is selected
      if (getInstrument()->GetType() == IT_SAMPLE && currentField &&
          currentField->GetVariableID() == Token::SampleInstrumentSample) {

        if (viewMode_ == VM_NEW) {
          viewMode_ = VM_NORMAL; // clear the "enter double tap" state
          ConfirmStopPlayback(Token::ActionImport);
        } else {
          // mark as "new" mode so a 2nd following ENTER will trigger the sample
          // import above
          viewMode_ = VM_NEW;
        }
      } else if (viewMode_ == VM_NEW) {
        // If we're not on the sample field but in VM_NEW mode, reset it
        viewMode_ = VM_NORMAL;
      }

      switch (var->GetID()) {
        case Token::InstrumentParameterTable:
          {
            int next = TableHolder::GetInstance()->GetNext();
            if (next != NO_MORE_TABLE) {
              var->SetInt(next);
              isDirty_ = true;
            }
            break;
          }
        default:
          break;
      }
      mask &= ~BM_ENTER;
    }

  } else {
    // Clear the VM_NEW state if any key other than ENTER is pressed
    if (viewMode_ == VM_NEW) {
      viewMode_ = VM_NORMAL;
    }
  }

  if (viewMode_ == VM_CLONE) {
    if ((mask & BM_ENTER) && (mask & BM_ALT)) {
      UIIntVarField *field =
          (UIIntVarField *)GetFocus(); // TODO nILS: this is bad, it's not necessarily an UIIntVarField
      mask &= ~BM_ENTER;
      Variable *v = field->GetVariable();
      int current = v->GetInt();
      if (current == -1)
        return;

      if (field->GetVariableID() == Token::InstrumentParameterTable) {
        int next = TableHolder::GetInstance()->Clone(current);
        if (next != NO_MORE_TABLE) {
          v->SetInt(next);
          isDirty_ = true;
        }
      };
    }
    mask &= (0xFFFF - (BM_ENTER | BM_ALT));
  };

  // EDIT Modifier
  if (mask & BM_EDIT) {
    if (mask & BM_LEFT)
      warpToNext(-1);
    if (mask & BM_RIGHT)
      warpToNext(+1);
    if (mask & BM_DOWN)
      warpToNext(-16);
    if (mask & BM_UP)
      warpToNext(+16);
    if (mask & BM_ALT) {
      viewMode_ = VM_CLONE;
    }
  } else if (mask & BM_NAV) {
    // NAV Modifier
    if (mask & BM_LEFT) {
      // remove listening when leaving this screen
      getInstrument()->RemoveObserver(*this);
      ((WatchedVariable *)&instrumentType_)->RemoveObserver(*this);

      Navigate(VT_PHRASE, vtRevealFromLeft);
    }

    if (mask & BM_DOWN) {

      // Go to table view
      int i = viewData_->currentInstrumentID_;
      InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
      I_Instrument *instr = bank->GetInstrument(i);
      int table = instr->GetTable();
      if (table != VAR_OFF) {
        viewData_->currentTable_ = table;
      }
      Navigate(VT_TABLE2, vtRevealFromBottom);
    }

    if (mask & BM_PLAY) {
      player->OnStartButton(PM_PHRASE, viewData_->songX_, true, viewData_->chainRow_);
    }
  } else {
    // No modifier
    if (mask & BM_PLAY) {
      player->OnStartButton(PM_PHRASE, viewData_->songX_, false, viewData_->chainRow_);
    }
  }

  lastFocus_ = GetFocus();
}

void InstrumentView::DrawView() {
  Clear();

  // Draw title
  DrawTitle("Instrument %2.2X (%d/%d)", viewData_->currentInstrumentID_,
            project_->GetInstrumentBank()->UsedInstrumentCount(), MAX_INSTRUMENT_COUNT);

  // border line up top
  SetBackgroundColor(Theme::View::inactive);
  for (int n = 0; n < SCREEN_WIDTH; n++) {
    SetColor(Theme::View::Title::bg);
    DrawChar(n, 1, CHAR(char_block_top_s));

    DrawChar(n, 3, ' ');

    SetColor(Theme::View::bg);
    DrawChar(n, 5, CHAR(char_block_bottom_s));
  }

  // Draw fields
  FieldView::Redraw();

  // Draw instrument specific UI
  I_Instrument *instr = getInstrument();
  if (instr) {
    InstrumentType type = instr->GetType();

    if (type == IT_DRUM) {
      DrawViewDrum();
    } else if (type == IT_CHIPTUNE) {
      DrawViewChiptune();
    } else if (type == IT_STACK) {
      DrawViewStack();
    } else if (type == IT_SAMPLE) {
      DrawViewSample();
    } else if (type == IT_MIDI) {
      DrawViewMIDI();
    } else if (type == IT_NONE) {
      // fill to clear out the missing name, save and mod buttons
      SetBackgroundColor(Theme::View::inactive);
      DrawString(0, 4, "                      ");
      DrawString(25, 4, "       ");
    }
  }

  // draw the map
  mapVisible_ = AppWindow::GetInstance()->buttonDown(BM_NAV);
  if (mapVisible_) {
    drawMap();
  }

  ModalView *mv = GetModalView();
  if (mv) {
    mv->Redraw();
  }
}

void InstrumentView::OnFocus() {
  Trace::Log("INSTRUMENTVIEW", "onFocus");

  // Check if we're returning from a sample import and need to assign the sample
  if (viewData_->shouldAssignImportedSample && viewData_->lastImportedSampleIndex >= 0) {
    Trace::Log("INSTRUMENTVIEW", "Assigning imported sample index: %d to current instrument",
               viewData_->lastImportedSampleIndex);

    I_Instrument *instr = getInstrument();
    if (instr && instr->GetType() == IT_SAMPLE) {
      SampleInstrument *sampleInstr = static_cast<SampleInstrument *>(instr);
      sampleInstr->AssignSample(viewData_->lastImportedSampleIndex);
      isDirty_ = true;
    }

    // Reset the flag after assignment
    viewData_->shouldAssignImportedSample = false;
    viewData_->lastImportedSampleIndex = -1;
  }

  // Get latest selected instrument, ensures we display the instrument that was
  // selected in the PhraseView
  int currentID = viewData_->currentInstrumentID_;
  Trace::Debug("INSTRUMENTVIEW", "Current instrument ID from ViewData: %d", currentID);

  // Get the current instrument based on the ViewData's currentInstrumentID_
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(currentID);

  if (instr) {
    // Update the instrument type field to match the current instrument
    InstrumentType currentType = instr->GetType();

    Trace::Debug("INSTRUMENTVIEW", "Current instrument type: %d", currentType);

    // Only update if the type has changed
    if (instrumentType_.GetInt() != currentType) {
      Trace::Log("INSTRUMENTVIEW", "OnFocus instrument type changed from %d to %d", instrumentType_.GetInt(),
                 currentType);
      // Set the instrument type without triggering the observer update
      // because we dont want the observer to do its normal check for a modified
      // instrument
      instrumentType_.SetInt(currentType, false);
    }

    // Always refresh the UI fields when focusing the view in case of instrument
    // change from last time
    onInstrumentTypeChange(true);
  }
}

void InstrumentView::Update(Observable &o, I_ObservableData *data) {

  if (!hasFocus_) {
    return;
  }

  uintptr_t token = (uintptr_t)data;

  switch (token) {
    case Token::VarInstrumentType:
      {
        // Get the current instrument to determine its actual type
        I_Instrument *instr = getInstrument();
        InstrumentType currentType = instr ? instr->GetType() : IT_NONE;

        // Store the proposed instrument type BEFORE we revert the UI
        InstrumentType proposedType = (InstrumentType)instrumentType_.GetInt();

        // Revert the UI field back to the current type until confirmed
        instrumentType_.SetInt(currentType, false);

        pendingInstrumentType_ = proposedType;
        ConfirmStopPlayback(Token::VarInstrumentType);
        break;
      }
    case Token::ActionExport:
      handleInstrumentExport();
      break;
    case Token::ActionModulation:
      goToModulationPage();
      break;
    case Token::ActionImport:
      // Switch to the InstrumentImportView
      Navigate(VT_INSTRUMENT_IMPORT, vtRevealFromCenter);
      break;
    case Token::SampleInstrumentGMInstrument:
      {
        // changing the GM instrument clears the sample
        Variable *sampleVar = getInstrument()->FindVariable(Token::SampleInstrumentSample);
        if (sampleVar) {
          sampleVar->SetInt(-1);
          sampleInputField_->SetActive(false);
        }

        Variable *gmVar = getInstrument()->FindVariable(Token::SampleInstrumentGMInstrument);
        gmInputField_->SetActive(gmVar->GetInt() != NO_GM_INSTRUMENT);
        // TODO nILS: warn about losing slices when changing GM instrument if sample was assigned and slices exist
        // like below

        scrollStartTime_ = System::GetInstance()->Millis();
        isDirty_ = true;
        break;
      }
    case Token::SampleInstrumentSample:
      {
        // changing the sample clears the GM Instrument
        Variable *gmVar = getInstrument()->FindVariable(Token::SampleInstrumentGMInstrument);
        if (gmVar) {
          gmVar->SetInt(NO_GM_INSTRUMENT);
          // find the intput field and set it inactive
          gmInputField_->SetActive(false);
        }

        I_Instrument *instr = getInstrument();
        if (!instr || instr->GetType() != IT_SAMPLE) {
          break;
        }

        Variable *sampleVar = getInstrument()->FindVariable(Token::SampleInstrumentSample);
        sampleInputField_->SetActive(sampleVar->GetInt() != NO_GM_INSTRUMENT);

        SampleInstrument *sampleInstr = static_cast<SampleInstrument *>(instr);
        int newIndex = sampleInstr->GetSampleIndex();

        if (suppressSampleChangeWarning_) {
          suppressSampleChangeWarning_ = false;
          lastSampleIndex_ = newIndex;
          break;
        }

        if (newIndex == lastSampleIndex_) {
          break;
        }

        if (!sampleInstr->HasSlicesForWarning()) {
          sampleInstr->ClearSlices();
          lastSampleIndex_ = newIndex;
          updateSliceCount(sampleInstr);
          isDirty_ = true;
          break;
        }

        MessageBox *mb = MessageBox::Create(*this, "Sample", "Change sample &", "clear slices?", MBBF_YES | MBBF_NO);
        pendingSampleChangeInstrument_ = sampleInstr;
        pendingSampleChangeNewIndex_ = newIndex;
        DoModal(mb, ModalViewCallback::create<InstrumentView, &InstrumentView::onConfirmSampleChange>(*this));
        break;
      }
    case Token::ActionShowSampleSlices:
      {
        I_Instrument *instr = getInstrument();
        if (!instr || instr->GetType() != IT_SAMPLE) {
          break;
        }
        SampleInstrument *sampleInstr = static_cast<SampleInstrument *>(instr);
        if (sampleInstr->GetSampleIndex() < 0) {
          MessageBox *mb = MessageBox::Create(*this, "Sample", "Assign a sample first", MBBF_OK);
          DoModal(mb);
          break;
        }
        Navigate(VT_SAMPLE_SLICES, vtRevealFromCenter);
        break;
      }
    case Token::MidiInstrumentProgram:
      {
        // When program value changes, send a MIDI Program Change message during
        // playback
        if (!Player::GetInstance()->IsRunning()) {
          break;
        }

        I_Instrument *instr = getInstrument();
        if (instr && instr->GetType() == IT_MIDI) {
          MidiInstrument *midiInstr = (MidiInstrument *)instr;

          // Get the channel and program values
          Variable *channelVar = midiInstr->FindVariable(Token::MidiInstrumentChannel);
          Variable *programVar = midiInstr->FindVariable(Token::MidiInstrumentProgram);

          if (channelVar && programVar) {
            int channel = channelVar->GetInt();
            int program = programVar->GetInt();

            // Send Program Change message
            midiInstr->SendProgramChange(channel, program);
          }
        }
        break;
      }
    default:
      break;
  }
}

bool InstrumentView::checkInstrumentModified() {
  // Get current instrument
  I_Instrument *instrument = getInstrument();
  if (!instrument) {
    return false;
  }

  // Get the list of variables for this instrument
  etl::ilist<Variable *> *variables = instrument->Variables();
  if (!variables) {
    return false;
  }

  // Check if any variable has been modified from its default value
  for (auto it = variables->begin(); it != variables->end(); ++it) {
    Variable *var = *it;
    if (var && var->IsModified()) {
      return true;
    }
  }

  // No variables have been modified
  return false;
}

void InstrumentView::goToModulationPage() {
  // TODO
}

void InstrumentView::goToInstrumentPage() {
  // TODO
}

void InstrumentView::resetInstrumentToDefaults() {
  // Get current instrument
  I_Instrument *instrument = getInstrument();
  if (!instrument) {
    return;
  }

  // Get the list of variables for this instrument
  etl::ilist<Variable *> *variables = instrument->Variables();
  if (!variables) {
    return;
  }

  // Reset all variables to their default values
  for (auto it = variables->begin(); it != variables->end(); ++it) {
    Variable *var = *it;
    if (var) {
      var->Reset();
    }
  }
}

void InstrumentView::handleInstrumentExport() {
  // Get current instrument using its id
  I_Instrument *instrument = viewData_->project_->GetInstrumentBank()->GetInstrument(viewData_->currentInstrumentID_);

  // Check if the instrument has a name set
  etl::string<MAX_INSTRUMENT_NAME_LENGTH> name = instrument->GetDisplayName();
  // Check if the name is empty, the default value, or matches the default
  // instrument type name
  etl::string<MAX_INSTRUMENT_NAME_LENGTH> defaultTypeName = instrument->GetDefaultName();

  if (name.empty() || name == defaultTypeName) {
    // Show error message if no name is set
    MessageBox *mb = MessageBox::Create(*this, "Export", "Please set a name", "before exporting", MBBF_OK);
    DoModal(mb);
  } else {
    // Export the instrument using the name field
    PersistencyResult result = PersistencyService::GetInstance()->ExportInstrument(instrument, name);

    if (result == PERSIST_EXISTS) {
      // File already exists, ask user if they want to override it
      etl::string<64> confirmMsg = "Overwrite existing file?";
      MessageBox *mb = MessageBox::Create(*this, "Export", confirmMsg.c_str(), name.c_str(), MBBF_YES | MBBF_NO);

      exportInstrument_ = instrument;
      exportName_ = name;
      DoModal(mb, ModalViewCallback::create<InstrumentView, &InstrumentView::onConfirmExportOverwrite>(*this));
    } else {
      // Create a message with the instrument name
      etl::string<64> successMsg = "Exported: ";
      successMsg += name;

      const char *message = result == PERSIST_SAVED ? successMsg.c_str() : "Failed to export instrument";
      // Show export result message
      MessageBox *mb = MessageBox::Create(*this, "Export", message, MBBF_OK);
      DoModal(mb);
    }
  }
}

void InstrumentView::onConfirmInstrumentTypeChange(View &, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    applyProposedTypeChangeUI();
  }
}

void InstrumentView::onConfirmResetInstrument(View &, ModalView &dialog) {
  I_Instrument *instr = pendingPurgeInstrument_;
  pendingPurgeInstrument_ = nullptr;

  if (dialog.GetReturnCode() != MBL_YES || !instr) {
    return;
  }

  instr->Purge();
  isDirty_ = true;
}

void InstrumentView::onConfirmSampleChange(View &, ModalView &dialog) {
  SampleInstrument *sampleInstr = pendingSampleChangeInstrument_;

  if (!sampleInstr) {
    return;
  }

  int newIndex = pendingSampleChangeNewIndex_;
  pendingSampleChangeInstrument_ = nullptr;
  pendingSampleChangeNewIndex_ = -1;

  if (dialog.GetReturnCode() == MBL_YES) {
    sampleInstr->ClearSlices();
    lastSampleIndex_ = newIndex;
    updateSliceCount(sampleInstr);
    isDirty_ = true;
    return;
  }

  suppressSampleChangeWarning_ = true;
  if (Variable *sampleVar = sampleInstr->FindVariable(Token::SampleInstrumentSample)) {
    sampleVar->SetInt(lastSampleIndex_);
  }
  isDirty_ = true;
}

void InstrumentView::onConfirmExportOverwrite(View &, ModalView &dialog) {
  I_Instrument *instrument = exportInstrument_;
  etl::string<MAX_INSTRUMENT_NAME_LENGTH> name = exportName_;
  exportInstrument_ = nullptr;
  exportName_.clear();

  if (dialog.GetReturnCode() != MBL_YES || !instrument) {
    return;
  }

  PersistencyService::GetInstance()->ExportInstrument(instrument, name, true);
  Trace::Log("INSTRUMENTVIEW", "Instrument '%s' exported with overwrite", name.c_str());
}

void InstrumentView::goToImport() {
  // First check if the samplelib exists
  bool samplelibExists = FileSystem::GetInstance()->exists(SAMPLES_LIB_DIR);

  if (!samplelibExists) {
    MessageBox *mb = MessageBox::Create(*this, "Error", "Can't access the samplelib", MBBF_OK);
    DoModal(mb);
  } else {
    SampleImportView::SetSourceViewType(VT_INSTRUMENT);
    // set browser into sample import mode in top level samples dir
    viewData_->importViewStartDir = SAMPLES_LIB_DIR;

    // Go to import sample
    viewData_->shouldAssignImportedSample = true;
    Navigate(VT_IMPORT, vtRevealFromCenter);
  }
}

void InstrumentView::changeInstrumentType() {
  // Check if any instrument field has been modified
  bool instrumentModified = checkInstrumentModified();
  if (instrumentModified) {
    MessageBox *mb =
        MessageBox::Create(*this, "Instrument", "Change the instrument and", "lose the settings?", MBBF_YES | MBBF_NO);
    DoModal(mb, ModalViewCallback::create<InstrumentView, &InstrumentView::onConfirmInstrumentTypeChange>(*this));
  } else {
    // Apply the proposed type change immediately if not modified
    instrumentType_.SetInt(pendingInstrumentType_, false);
    onInstrumentTypeChange();
  }
}

void InstrumentView::ConfirmedStop(Token source) {
  switch (source) {
    case Token::ActionImport:
      goToImport();
      break;
    case Token::VarInstrumentType:
      changeInstrumentType();
      break;
  }
}

// override to synchronize the position selection for DrumInstruments
void InstrumentView::SetFocus(UIField *field) {
  // Drum instrument, currently on a field and moving to a field?
  UIField *focus = GetFocus();
  I_Instrument *instr = getInstrument();

  if (focus && field && instr && instr->GetType() == IT_DRUM) {
    // is current field a hex field?
    int sourceColumn = focus->GetColumn();
    if (sourceColumn != -1) {
      field->SetColumn(sourceColumn);
    }
  }

  // call parent implementation
  FieldView::SetFocus(field);
}

void InstrumentView::AnimationUpdate() {
  // First call the parent class implementation to draw the battery gauge
  ScreenView::AnimationUpdate();

  I_Instrument *instr = getInstrument();
  if (instr) {
    InstrumentType type = instr->GetType();

    if (type == IT_SAMPLE) {
      AnimationUpdateSample();
    }
  }
}

void InstrumentView::AddVolumeRow(int y) {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  GUIPoint position = GUIPoint(1, y);

  Variable *v = instr->FindVariable(Token::InstrumentParameterVolume);
  intVarField_.emplace_back(UIIntVarField(position, *v, "Volume  : %2.2X", 0, 0xFF, 1, 0x10));
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  v = instr->FindVariable(Token::InstrumentParameterPan);
  position.x_ = 17;
  intVarField_.emplace_back(UIIntVarField(position, *v, "Pan     : %2.2X", 0, 0xFF, 1, 0x10));
  intVarField_.back().SetLabelColor(Theme::SemanticColors::volume);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  addIndexToLine(0, y, true);
  addIndexToLine(1, y, false);
}

void InstrumentView::AddTableRow(int y) {
  int i = viewData_->currentInstrumentID_;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  I_Instrument *instr = bank->GetInstrument(i);
  GUIPoint position = GUIPoint(1, y);

  Variable *v = instr->FindVariable(Token::InstrumentParameterTable);
  intVarOffField_.emplace_back(position, *v, "Table   : %2.2X", 0x00, TABLE_COUNT - 1, 1, 0x10);
  intVarOffField_.back().SetLabelColor(Theme::SemanticColors::table);
  intVarOffField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarOffField_.back());

  position.x_ = 17;
  v = instr->FindVariable(Token::InstrumentParameterTableAutomation);
  intVarField_.emplace_back(position, *v, "Automate: %1s ", 0, 1, 1, 1);
  intVarField_.back().SetLabelColor(Theme::SemanticColors::table);
  intVarField_.back().SetFieldConfiguration(instrumentFieldConfiguration);
  fieldList_.insert(fieldList_.end(), &intVarField_.back());

  addIndexToLine(2, y, true);
  addIndexToLine(3, y, false);
}

const GUIRect InstrumentView::GetFocusRect() {
  ModalView *mv = GetModalView();

  if (mv) {
    const GUIRect rect = mv->GetFocusRect();
    return rect;
  }

  return FieldView::GetFocusRect();
}
