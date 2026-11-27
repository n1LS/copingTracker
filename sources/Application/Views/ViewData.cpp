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

#include "ViewData.h"
#include "Application/Model/Table.h"
#include "BaseClasses/View.h"

ViewData::ViewData(Project *project) {
  Load(project);
}

ViewData::~ViewData() {
}

void ViewData::Load(Project *project) {
  project_ = project;
  song_ = &(project->song_);
  currentChain_ = 0;
  currentPhrase_ = 0;
  songX_ = 0;
  songY_ = 0;
  songOffset_ = 0;
  chainCol_ = 0;
  chainRow_ = 0;
  currentTable_ = 0;
  currentInstrumentID_ = 0;
  currentGroove_ = 0;
  playMode_ = PM_SONG;
  phraseCurPos_ = 0;
  phraseLastInstrument_ = 0;
  phraseLastNote_ = NOTE_C3;
  phraseLastCmd_ = Token::InstrumentCommandNone;
  phraseLastParam_ = 0;
  phraseLastVolume_ = NO_VOLUME;
  clipboardCol_ = 0;
  clipboardWidth_ = 0;
  clipboardHeight_ = 0;
  for (int i = 0; i < STEPS_PER_PHRASE; i++) {
    // an empty step (all zero is the note 0)
    clipboardSteps_[i] = PhraseStep{};
    clipboardSteps_[i].note = NO_NOTE;
  }
  phraseViewType_ = VT_PHRASE;

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    songPlayPos_[i] = 0;
    chainPlayPos_[i] = 0;
    phrasePlayPos_[i] = 0;
    currentPlayChain_[i] = EMPTY_SONG_VALUE;
    currentPlayPhrase_[i] = EMPTY_CHAIN_VALUE;
  }

  sampleEditorFilename.clear();
  isShowingSampleEditorProjectPool = false;
  importViewStartDir = nullptr;
}

bool ViewData::SelectInstrumentOfPhraseStep(int step) {
  const uint8_t instrument = song_->phrase_.Step(currentPhrase_, step).getInstrument();
  currentInstrumentID_ = (instrument != NO_INSTRUMENT) ? instrument : phraseLastInstrument_;
  return currentInstrumentID_ != NO_INSTRUMENT;
}

void ViewData::SelectTableOfPhraseStep(int step) {
  const PhraseStep &phraseStep = song_->phrase_.Step(currentPhrase_, step);
  if (Token::enum_type(phraseStep.getCmd1()) == Token::InstrumentCommandTable) {
    currentTable_ = phraseStep.param1 & (TABLE_COUNT - 1);
  } else if (Token::enum_type(phraseStep.getCmd2()) == Token::InstrumentCommandTable) {
    currentTable_ = phraseStep.param2 & (TABLE_COUNT - 1);
  }
}

unsigned char ViewData::UpdateSongChain(int offset) {
  unsigned char *c = &song_->EditCell(songOffset_ + songY_, songX_);
  updateData(c, offset, CHAIN_COUNT - 1, false);
  return *c;
}

void ViewData::SetSongChain(unsigned char value) {
  song_->EditCell(songOffset_ + songY_, songX_) = value;
}

void ViewData::UpdateSongOffset(int offset) {
  songOffset_ += offset;
  checkSongBoundaries();
}

void ViewData::UpdateSongCursor(int dx, int dy) {
  songX_ += dx;
  songY_ += dy;
  checkSongBoundaries();
}

void ViewData::checkSongBoundaries() {
  if (songX_ > SONG_CHANNEL_COUNT - 1)
    songX_ = SONG_CHANNEL_COUNT - 1;
  if (songX_ < 0)
    songX_ = 0;
  if (songY_ < 0) {
    songOffset_ += songY_;
    songY_ = 0;
  };
  if (songY_ > View::songRowCount_ - 1) {
    songOffset_ += songY_ - View::songRowCount_ + 1;
    songY_ = View::songRowCount_ - 1;
  };
  if (songOffset_ > SONG_ROW_COUNT - View::songRowCount_) {
    songOffset_ = SONG_ROW_COUNT - View::songRowCount_;
  }
  if (songOffset_ < 0) {
    songOffset_ = 0;
  }
}

const unsigned char *ViewData::GetCurrentSongPointer() {
  return &song_->Row(songOffset_ + songY_).chains[songX_];
}

unsigned char *ViewData::EditCurrentSongPointer() {
  return &song_->EditCell(songOffset_ + songY_, songX_);
}

int ViewData::UpdateChainCursorValue(int offset, int dx, int dy) {
  unsigned char *c = 0;
  unsigned char limit = 0;
  bool wrap = false;

  switch (chainCol_ + dx) {
    // TODO nILS: figure out how to separate the column numbers from the enum, so that this does not depend on the order of the enum values
    case 0:
      c = &song_->chain_.EditStep(currentChain_, chainRow_ + dy).phrase;
      limit = PHRASE_COUNT - 1;
      wrap = false;
      break;
    case 2:
      c = &song_->chain_.EditStep(currentChain_, chainRow_ + dy).transpose;
      limit = 0xFF;
      wrap = false;
      break;
    case 1:
      {
        // the scale-aware flag: ENTER alone (offset 0) toggles it, ENTER + RIGHT / UP set it and ENTER + LEFT / DOWN
        // clear it (ENTER toggles when it is pressed, a second toggle by the arrow would undo that)
        const int row = chainRow_ + dy;
        const bool on = offset == 0 ? !song_->chain_.IsScaleAware(currentChain_, row) : offset > 0;
        song_->chain_.SetScaleAware(currentChain_, row, on);
        return on ? 1 : 0;
      }
    default:
      return -1;
  }
  updateData(c, offset, limit, wrap);

  return *c;
}

void ViewData::UpdateChainCursor(int dx, int dy) {
  chainCol_ += dx;
  chainRow_ += dy;

  if (chainCol_ > 2)
    chainCol_ = 2;
  if (chainCol_ < 0)
    chainCol_ = 0;
  if (chainRow_ > 15)
    chainRow_ = 15;
  if (chainRow_ < 0)
    chainRow_ = 0;
}

void ViewData::SetChainPhrase(unsigned char value) {
  song_->chain_.EditStep(currentChain_, chainRow_).phrase = value;
}

const unsigned char *ViewData::GetCurrentChainPointer() {
  return &song_->chain_.Step(currentChain_, chainRow_).phrase;
}

unsigned char *ViewData::EditCurrentChainPointer() {
  return &song_->chain_.EditStep(currentChain_, chainRow_).phrase;
}
