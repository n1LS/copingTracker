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

#ifndef _VIEW_DATA_
#define _VIEW_DATA_

#include "Application/Model/Project.h"
#include "Application/Utils/updateData.h"
#include "Foundation/Types/ViewType.h"
#include "System/Console/Trace.h"

enum PlayMode { PM_SONG, PM_CHAIN, PM_PHRASE, PM_LIVE, PM_AUDITION };

class ViewData {

public:
  ViewData(Project *project);
  ~ViewData();
  void Load(Project *project);

  unsigned char UpdateSongChain(int offset);
  void UpdateSongOffset(int offset);
  void UpdateSongCursor(int dx, int dy);
  void SetSongChain(unsigned char);
  // the chain under the cursor of the song view, Edit... records it in the undo history first
  const unsigned char *GetCurrentSongPointer();
  unsigned char *EditCurrentSongPointer();

  void UpdateChainCursor(int dx, int dy);
  int UpdateChainCursorValue(int offset, int dx, int dy);
  void SetChainPhrase(unsigned char value);
  // the phrase under the cursor of the chain view, Edit... records it in the undo history first
  const unsigned char *GetCurrentChainPointer();
  unsigned char *EditCurrentChainPointer();

  // Shared by the phrase view and the piano roll view. `step` is the cursor row in the phrase.
  // Makes the instrument of the phrase step the current instrument. A step without an
  // instrument uses the last instrument used in the phrase editor. Returns false if there is none.
  bool SelectInstrumentOfPhraseStep(int step);
  // Makes the table of the first TABLE command of the phrase step the current table,
  // the current table stays as it is if the step has none.
  void SelectTableOfPhraseStep(int step);

  Project *GetProject() {
    return project_;
  }

protected:
  void checkSongBoundaries();

  inline void updateData(unsigned char *c, int offset, unsigned char limit, bool wrap) {
    updateDataValue(c, offset, limit, wrap);
  }

public:
  // Data

  Project *project_;
  Song *song_;

  // Editor settings

  int songX_;      // Current song screen position in the editor
  int songY_;      //
  int songOffset_; // Current song offset (top screen row) in editor

  int chainRow_; // Current row in chain editor
  int chainCol_; // Current column in chain editor

  int currentChain_;  // Current edited chain
  int currentPhrase_; // Current edited phrase

  int currentInstrumentID_; // Current edited instrument

  int currentTable_; // Current edited table

  int currentGroove_; // Current edited groove

  // Player Settings

  PlayMode playMode_;
  int songPlayPos_[SONG_CHANNEL_COUNT];                 // Play position of each channel
  unsigned char currentPlayChain_[SONG_CHANNEL_COUNT];  // Current played chain
                                                        // for each channel
  int chainPlayPos_[SONG_CHANNEL_COUNT];                // Play position in chain for each
                                                        // channel
  unsigned char currentPlayPhrase_[SONG_CHANNEL_COUNT]; // Current played phrase for each
                                                        // channel
  int phrasePlayPos_[SONG_CHANNEL_COUNT];               // Play position in phrase for each
                                                        // channel
  int phraseCurPos_;                                    // current UI cursor row position
  int phraseLastInstrument_;                            // last instrument used in the phrase editor
  int phraseLastNote_;                                  // last note used in the phrase editor
  int phraseLastCmd_;                                   // last command used in the phrase editor
  int phraseLastParam_;                                 // last command parameter used in the phrase editor
  uint8_t phraseLastVolume_;                            // last volume used in the phrase editor
  // The clipboard of the phrase view and the piano roll: the columns clipboardCol_ ... + clipboardWidth_ - 1 (in the
  // order of the phrase view columns / StepField) of clipboardHeight_ steps. Nothing to paste if the height is 0.
  uint8_t clipboardCol_;
  uint8_t clipboardWidth_;
  uint8_t clipboardHeight_;
  PhraseStep clipboardSteps_[STEPS_PER_PHRASE];
  ViewType phraseViewType_; // the view last used in the slot of the phrase view: VT_PHRASE or VT_PIANOROLL

  // Sample Editor
  etl::string<MAX_INSTRUMENT_FILENAME_LENGTH> sampleEditorFilename;
  bool isShowingSampleEditorProjectPool = false;

  // Import File browser
  const char *importViewStartDir = nullptr;
  bool importViewShowProjectPool = false; // Track user's preference for project pool vs library

  // Track last imported sample index for auto-assignment
  int lastImportedSampleIndex = -1;
  bool shouldAssignImportedSample = false;
};
#endif
