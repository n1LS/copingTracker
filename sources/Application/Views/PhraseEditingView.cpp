/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "PhraseEditingView.h"
#include "Application/Instruments/CommandList.h"
#include "Application/Instruments/I_Instrument.h"
#include "Application/Instruments/InstrumentBank.h"
#include "Application/Instruments/SampleInstrument.h"
#include "Application/Model/Config.h"
#include "Application/Model/Scale.h"
#include "Application/Player/Player.h"
#include "Application/Views/CommandView.h"
#include <algorithm>

static const int16_t stepOffsets_[4] = {-1, 1, 16, -16};

static void SetCommandCallback(View &v, ModalView &dialog) {
  static_cast<PhraseEditingView &>(v).onCommandPicked(Token::enum_type(dialog.GetReturnCode()));
}

Variable PhraseEditingView::cmdEdit_(Token::ActionEdit, 0);
GUIPoint PhraseEditingView::cmdEditPos_(0, 10);
UIHexVarField PhraseEditingView::cmdEditField_(cmdEditPos_, cmdEdit_, 4, "%4.4X", 0, 0xFFFF, 16, true);

PhraseEditingView::PhraseEditingView(GUIWindow &w, ViewData *viewData) : ScreenView(w, viewData) {
}

PhraseEditingView::~PhraseEditingView() {
}

int PhraseEditingView::cursorStep() const {
  return viewData_->phraseCurPos_;
}

void PhraseEditingView::OnFocus() {
  viewData_->phraseViewType_ = viewType_;
}

void PhraseEditingView::startAudition(bool startIfNotRunning) {
  // no instrument -> no audition
  if (getEffectiveInstrumentForStep(cursorStep()) == NO_INSTRUMENT) {
    return;
  }

  Player *player = Player::GetInstance();
  if (player->IsRunning()) {
    // now also update if in auditioning mode
    if (viewData_->playMode_ == PM_AUDITION) {
      player->Stop();
      player->OnStartButton(PM_AUDITION, viewData_->songX_, false, viewData_->chainRow_);
    }
  } else if (startIfNotRunning) {
    player->OnStartButton(PM_AUDITION, viewData_->songX_, false, viewData_->chainRow_);
  }
}

void PhraseEditingView::stopAudition() {
  Player *player = Player::GetInstance();
  if (viewData_->playMode_ == PM_AUDITION) {
    player->Stop();
  }
}

bool PhraseEditingView::navigationDown(uint16_t direction) {
  switch (direction) {
    case BM_LEFT:
      Navigate(VT_CHAIN, vtRevealFromLeft);
      return true;
    case BM_RIGHT:
      enterInstrumentUnderCursor();
      return true;
    case BM_DOWN:
      // Go to table view
      viewData_->SelectTableOfPhraseStep(cursorStep());
      Navigate(VT_TABLE, vtRevealFromBottom);
      return true;
    case BM_UP:
      // Go to groove view
      stopAudition();

      Navigate(VT_GROOVE, vtRevealFromTop);
      return true;
  }
  return false;
}

bool PhraseEditingView::OnNavDoubleTap() {
  // NAV + NAV switches to the alternative view of the same phrase
  stopAudition();
  Navigate(otherView(), vtDissolve);
  return true;
}

bool PhraseEditingView::processPlaybackInput(uint16_t newButton, uint16_t mask) {
  if (IsChord(newButton, mask, BM_PLAY, BM_NAV)) {
    playPhrase(true);
    return true;
  }
  if (IsChord(newButton, mask, BM_PLAY)) {
    playPhrase(false);
    return true;
  }
  return false;
}

bool PhraseEditingView::processPhraseSwitchInput(uint16_t newButton, uint16_t mask) {
  if (mask != BM_EDIT) {
    return false;
  }

  switch (newButton) {
    case BM_LEFT:
      warpToNeighbour(-1);
      return true;
    case BM_RIGHT:
      warpToNeighbour(1);
      return true;
    case BM_UP:
      warpInChain(-1);
      return true;
    case BM_DOWN:
      warpInChain(1);
      return true;
  }
  return false;
}

uint8_t PhraseEditingView::getEffectiveInstrumentForStep(int step) {
  if (step < 0) {
    return NO_INSTRUMENT;
  }

  const PhraseStep *steps = currentSteps();
  for (int i = step; i >= 0; --i) {
    if (steps[i].getInstrument() != NO_INSTRUMENT) {
      return steps[i].getInstrument();
    }
  }
  return NO_INSTRUMENT;
}

void PhraseEditingView::changeNote(int step, ViewUpdateDirection direction) {
  const bool smallChange = (direction == VUD_LEFT || direction == VUD_RIGHT);
  const int sign = (direction == VUD_LEFT || direction == VUD_DOWN) ? -1 : 1;
  changeNote(step, sign, smallChange);
}

void PhraseEditingView::changeNote(int step, int sign, bool smallChange) {
  // the note is a bit-field of the step, so it is changed here and written back
  PhraseStep &data = currentSteps()[step];
  unsigned char note = data.note;
  unsigned char *c = &note;
  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  uint8_t instrumentId = getEffectiveInstrumentForStep(step);
  I_Instrument *instrument = bank->GetInstrument(instrumentId);

  if (instrument) {
    // some instruments want to manually set the change to the note
    if (instrument->AffectNote(c, sign, smallChange)) {
      data.note = note;
      viewData_->phraseLastNote_ = *c;
      return;
    }
  }

  // Get the offset based on the change (one step in the scale or an octave)
  int offset = smallChange ? 1 : 12;
  if (sign < 0) {
    offset *= -1;
  }

  // when changing notes from note off, always start from C3
  if (*c == NOTE_OFF) {
    *c = NOTE_C3;
    offset = 0;
  }

  uint8_t sliceFirst = 0;
  uint8_t sliceLast = 0;

  // attempt to get the sample instrument for slice display
  SampleInstrument *sampleInstrument = nullptr;
  if (instrument && instrument->GetType() == IT_SAMPLE) {
    sampleInstrument = (SampleInstrument *)instrument;
  }

  // TODO nILS: this should also happen in SampleInstrument's AffectNote instead of getting special treatmnt here
  if (sampleInstrument && sampleInstrument->GetSliceNoteRange(sliceFirst, sliceLast)) {
    // Apply scale or slice range constraints
    int newNote = *c + offset;

    if (newNote < sliceFirst) {
      newNote = sliceFirst;
    } else if (newNote > sliceLast) {
      newNote = sliceLast;
    }
    *c = static_cast<unsigned char>(newNote);
  } else {
    int scale = viewData_->project_->GetScale();
    int scaleRoot = viewData_->project_->GetScaleRoot();

    // Calculate the new note with the offset
    int newNote = *c + offset;

    // Check if the note is in the scale (adjusted for root)
    if (instrument && instrument->SupportsScales()) {
      // Add/remove from offset to match selected scale
      while (newNote >= 0 && !scaleSteps[scale][(newNote + 12 - scaleRoot) % 12]) {
        offset > 0 ? offset++ : offset--;
        newNote = *c + offset;
      }
    }

    updateData(c, offset, instrument ? instrument->GetHighestNote() : HIGHEST_NOTE, true);
  }

  data.note = note;
  viewData_->phraseLastNote_ = *c;
}

void PhraseEditingView::changeInstrument(int step, ViewUpdateDirection direction) {
  PhraseStep &data = currentSteps()[step];
  unsigned char instrument = data.getInstrument();
  updateData(&instrument, stepOffsets_[direction], MAX_INSTRUMENT_COUNT - 1, false);
  data.setInstrument(instrument);
  viewData_->phraseLastInstrument_ = data.getInstrument();
}

void PhraseEditingView::changeVolume(int step, ViewUpdateDirection direction) {
  PhraseStep &data = currentSteps()[step];
  // the volume is 1..15, or NO_VOLUME
  uint8_t vol = data.getVolume();

  switch (direction) {
    case VUD_UP:
      vol = 0x0F;
      break;
    case VUD_DOWN:
      vol = NO_VOLUME;
      break;
    case VUD_LEFT:
      // NO_VOLUME is 0: below the quietest is no value, and no value stays
      if (vol > NO_VOLUME) {
        vol--;
      }
      break;
    case VUD_RIGHT:
      // ... and one up from no value is the quietest
      vol = std::min(0x0F, vol + 1);
      break;
  }
  data.setVolume(vol);
  viewData_->phraseLastVolume_ = vol;
}

void PhraseEditingView::changeCommand(int step, bool second, ViewUpdateDirection direction) {
  PhraseStep &data = currentSteps()[step];
  Token token = second ? Token::enum_type(data.getCmd2()) : Token::enum_type(data.getCmd1());

  switch (direction) {
    case VUD_LEFT:
      token = CommandList::GetPrev(token);
      break;
    case VUD_RIGHT:
      token = CommandList::GetNext(token);
      break;
    case VUD_UP:
      token = CommandList::GetNextAlpha(token);
      break;
    case VUD_DOWN:
      token = CommandList::GetPrevAlpha(token);
      break;
  }

  if (second) {
    data.setCmd2(static_cast<uint8_t>(static_cast<char>(token)));
  } else {
    data.setCmd1(static_cast<uint8_t>(static_cast<char>(token)));
  }
  viewData_->phraseLastCmd_ = token;
}

bool PhraseEditingView::openCommandPicker(int step, bool second) {
  if (!Config::GetInstance()->FindVariable(Token::VarConfigCommandPicker)->GetInt()) {
    return false;
  }

  const PhraseStep &data = currentSteps()[step];
  uint8_t cmd = second ? data.getCmd2() : data.getCmd1();

  // use lastCmd if we hit an empty command field
  if (cmd == Token::InstrumentCommandNone) {
    cmd = viewData_->phraseLastCmd_;
  }

  CommandView *cv = CommandView::Create(*this, Token::enum_type(cmd));
  DoModal(cv, ModalViewCallback::create<&SetCommandCallback>());
  return true;
}

void PhraseEditingView::setCommand(int step, bool second, Token command) {
  PhraseStep &data = currentSteps()[step];

  const uint8_t raw = static_cast<uint8_t>(command.raw());
  if (second) {
    data.setCmd2(raw);
  } else {
    data.setCmd1(raw);
  }
  viewData_->phraseLastCmd_ = raw;

  if (command == Token::InstrumentCommandNone) {
    // clear the parameter as well
    uint16_t *value = second ? &data.param2 : &data.param1;
    *value = 0;
  }
}

void PhraseEditingView::loadParamEditor(int step, bool second) {
  const PhraseStep &data = currentSteps()[step];
  cmdEdit_.SetInt(second ? data.param2 : data.param1);
}

void PhraseEditingView::changeCommandParam(int step, bool second, ViewUpdateDirection direction) {
  PhraseStep &data = currentSteps()[step];

  // pick correct input and output for the selected column
  Token currentCmd = second ? Token::enum_type(data.getCmd2()) : Token::enum_type(data.getCmd1());
  uint16_t *target = second ? &data.param2 : &data.param1;

  cmdEditField_.ProcessArrow(DirectionalButtons[direction]);
  uint16_t paramValue = cmdEdit_.GetInt();
  paramValue = CommandList::RangeLimitCommandParam(currentCmd, paramValue);
  cmdEdit_.SetInt(paramValue);

  *target = paramValue;
  viewData_->phraseLastParam_ = paramValue;
}

bool PhraseEditingView::pasteLastValue(int step, StepField field) {
  PhraseStep &data = currentSteps()[step];

  switch (field) {
    case StepField::Note:
      if (data.note == NO_NOTE) {
        data.note = viewData_->phraseLastNote_;
        data.setInstrument(viewData_->phraseLastInstrument_);
        return true;
      }
      viewData_->phraseLastNote_ = data.note;
      viewData_->phraseLastInstrument_ = data.getInstrument();
      break;
    case StepField::Instrument:
      if (data.getInstrument() == NO_INSTRUMENT) {
        data.setInstrument(viewData_->phraseLastInstrument_);
        return true;
      }
      viewData_->phraseLastInstrument_ = data.getInstrument();
      break;
    case StepField::Volume:
      if (data.getVolume() == NO_VOLUME) {
        if (viewData_->phraseLastVolume_ != NO_VOLUME) {
          data.setVolume(viewData_->phraseLastVolume_);
          return true;
        }
      } else {
        viewData_->phraseLastVolume_ = data.getVolume();
      }
      break;
    case StepField::Command1:
    case StepField::Command2:
      {
        const bool second = (field == StepField::Command2);
        Token token = Token::enum_type(second ? data.getCmd2() : data.getCmd1());
        if (token == Token::InstrumentCommandNone) {
          const uint8_t last = static_cast<uint8_t>(static_cast<char>(viewData_->phraseLastCmd_));
          if (second) {
            data.setCmd2(last);
          } else {
            data.setCmd1(last);
          }
          return true;
        }
        viewData_->phraseLastCmd_ = token;
        break;
      }
    case StepField::Param1:
    case StepField::Param2:
      // a parameter has no last value to paste
      break;
  }
  return false;
}

void PhraseEditingView::copyStepsToClipboard(int first, int count) {
  const PhraseStep *steps = currentSteps();
  for (int i = 0; i < count; i++) {
    viewData_->clipboardSteps_[i] = steps[first + i];
  }
  viewData_->clipboardCol_ = static_cast<uint8_t>(StepField::Note);
  viewData_->clipboardWidth_ = static_cast<uint8_t>(StepField::Param2) + 1;
  viewData_->clipboardHeight_ = static_cast<uint8_t>(count);
}

// copies one field of a step
static void copyStepField(PhraseStep &target, const PhraseStep &source, StepField field) {
  switch (field) {
    case StepField::Note:
      target.note = source.note;
      break;
    case StepField::Instrument:
      target.setInstrument(source.getInstrument());
      break;
    case StepField::Volume:
      target.setVolume(source.getVolume());
      break;
    case StepField::Command1:
      target.setCmd1(source.getCmd1());
      break;
    case StepField::Param1:
      target.param1 = source.param1;
      break;
    case StepField::Command2:
      target.setCmd2(source.getCmd2());
      break;
    case StepField::Param2:
      target.param2 = source.param2;
      break;
  }
}

int PhraseEditingView::pasteClipboardAt(int step) {
  PhraseStep *steps = currentSteps();
  const int height = viewData_->clipboardHeight_;

  for (int i = 0; i < viewData_->clipboardWidth_; i++) {
    const StepField field = static_cast<StepField>(i + viewData_->clipboardCol_);
    for (int j = 0; j < height; j++) {
      copyStepField(steps[(j + step) % STEPS_PER_PHRASE], viewData_->clipboardSteps_[j], field);
    }
  }
  return height;
}

bool PhraseEditingView::nudgeSteps(int first, int last, StepField firstField, StepField lastField, int delta) {
  // the first / last step of the phrase
  if (first + delta < 0 || last + delta >= STEPS_PER_PHRASE) {
    return false;
  }

  PhraseStep *steps = currentSteps();
  const PhraseStep jumped = steps[delta > 0 ? last + 1 : first - 1];

  auto copyFields = [&](PhraseStep &target, const PhraseStep &source) {
    for (int field = static_cast<int>(firstField); field <= static_cast<int>(lastField); field++) {
      copyStepField(target, source, static_cast<StepField>(field));
    }
  };

  if (delta > 0) {
    for (int step = last; step >= first; --step) {
      copyFields(steps[step + 1], steps[step]);
    }
    copyFields(steps[first], jumped);
  } else {
    for (int step = first; step <= last; ++step) {
      copyFields(steps[step - 1], steps[step]);
    }
    copyFields(steps[last], jumped);
  }
  return true;
}

void PhraseEditingView::clearSteps(int first, int count) {
  for (int i = 0; i < count; i++) {
    for (int field = static_cast<int>(StepField::Note); field <= static_cast<int>(StepField::Param2); field++) {
      clearStepField(first + i, static_cast<StepField>(field));
    }
  }
}

void PhraseEditingView::clearStepField(int step, StepField field) {
  static const uint8_t kNone = static_cast<uint8_t>(static_cast<char>(Token::InstrumentCommandNone));
  PhraseStep &data = currentSteps()[step];

  switch (field) {
    case StepField::Note:
      data.note = NO_NOTE;
      break;
    case StepField::Instrument:
      data.setInstrument(NO_INSTRUMENT);
      break;
    case StepField::Volume:
      data.setVolume(NO_VOLUME);
      break;
    case StepField::Command1:
      // the parameter goes with its command
      data.setCmd1(kNone);
      data.param1 = 0;
      break;
    case StepField::Param1:
      data.param1 = 0;
      break;
    case StepField::Command2:
      data.setCmd2(kNone);
      data.param2 = 0;
      break;
    case StepField::Param2:
      data.param2 = 0;
      break;
  }
}

bool PhraseEditingView::findPlayingStep(int &step, bool &muted) {
  Player *player = Player::GetInstance();
  if (!player || !player->IsRunning() || viewData_->playMode_ == PM_AUDITION) {
    return false;
  }

  // Loop on all channels to see if one of them is playing the current phrase
  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    if (player->IsChannelPlaying(i) && viewData_->currentPlayPhrase_[i] == viewData_->currentPhrase_) {
      step = std::clamp(viewData_->phrasePlayPos_[i], 0, STEPS_PER_PHRASE - 1);
      muted = player->IsChannelMuted(i);
      return true;
    }
  }
  return false;
}

const char *PhraseEditingView::findLiveIndicator() {
  Player *player = Player::GetInstance();
  if (!player || player->GetSequencerMode() != SM_LIVE) {
    return nullptr;
  }

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    if (player->GetQueueingMode(i) != QM_NONE) {
      // find the chain queued in channel
      unsigned char songPos = player->GetQueuePosition(i);
      unsigned char *chain = viewData_->song_->rows_[songPos].chains + i;
      if (*chain == viewData_->currentChain_) {
        return player->GetLiveIndicator(i);
      }
    }
  }
  return nullptr;
}

void PhraseEditingView::playPhrase(bool startFromPrevious) {
  Player::GetInstance()->OnStartButton(PM_PHRASE, viewData_->songX_, startFromPrevious, viewData_->chainRow_);
}

void PhraseEditingView::enterInstrumentUnderCursor() {
  // A step without an instrument plays the one in effect from the steps before it (the shadow instrument), that
  // is the one to go to. Without one the last instrument used in the phrase editor is used.
  const uint8_t effective = getEffectiveInstrumentForStep(cursorStep());
  if (effective != NO_INSTRUMENT) {
    viewData_->currentInstrumentID_ = effective;
    Navigate(VT_INSTRUMENT, vtRevealFromRight);
    return;
  }

  if (viewData_->SelectInstrumentOfPhraseStep(cursorStep())) {
    Navigate(VT_INSTRUMENT, vtRevealFromRight);
  }
}

void PhraseEditingView::warpInChain(int offset) {

  int currentRow = viewData_->chainRow_;
  viewData_->chainRow_ += offset;
  if ((viewData_->chainRow_ < 16) && (viewData_->chainRow_ >= 0)) {
    unsigned char *p = viewData_->GetCurrentChainPointer();
    if (*p != 0xFF) {
      viewData_->currentPhrase_ = *p;
      onPhraseSwitched();
    } else { // rollback
      viewData_->chainRow_ = currentRow;
    }
  } else { // rollback
    viewData_->chainRow_ = currentRow;
  }
  isDirty_ = true;
}

void PhraseEditingView::warpToNeighbour(int offset) {
  const int oldX = viewData_->songX_;
  const unsigned char oldChain = viewData_->currentChain_;

  // keep going in the direction of offset over empty channels (no chain, or no
  // phrase on the current chain row) until a phrase is found, else stay put
  for (int x = oldX + offset; x >= 0 && x < SONG_CHANNEL_COUNT; x += offset) {
    viewData_->songX_ = x;

    unsigned char *c = viewData_->GetCurrentSongPointer();
    if (*c == 0xFF) {
      continue;
    }

    viewData_->currentChain_ = *c;

    unsigned char *p = viewData_->GetCurrentChainPointer();
    if (*p == 0xFF) {
      continue;
    }

    viewData_->currentPhrase_ = *p;
    onPhraseSwitched();
    isDirty_ = true;
    return;
  }

  viewData_->songX_ = oldX;
  viewData_->currentChain_ = oldChain;
}
