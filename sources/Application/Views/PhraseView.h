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

#ifndef _PHRASE_VIEW_H_
#define _PHRASE_VIEW_H_

#include "Application/Model/Phrase.h"
#include "BaseClasses/UIBigHexVarField.h"
#include "ScreenView.h"
#include "ViewData.h"

typedef enum PhraseColumn {
  colNote,
  colInstrument,
  colVolume,
  colCmd1,
  colCmdVal1,
  colCmd2,
  colCmdVal2,
} PhraseColumn;

inline PhraseColumn operator+(PhraseColumn lhs, int rhs) {
  int value = static_cast<int>(lhs);
  value = (value + (int)colCmdVal2 + 1 + rhs) % ((int)colCmdVal2 + 1);
  return static_cast<PhraseColumn>(value);
}

inline PhraseColumn &operator+=(PhraseColumn &lhs, int rhs) {
  lhs = lhs + rhs;
  return lhs;
}

class PhraseView : public ScreenView {

public:
  PhraseView(GUIWindow &w, ViewData *viewData);
  ~PhraseView();
  void Reset();
  virtual void ProcessButtonMask(uint16_t mask, bool pressed);
  virtual void DrawView();
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int tick = 0);
  virtual void OnFocus();
  virtual void AnimationUpdate();
  void setCurrentlySelectedCommand(Token command);

protected:
  void updateCursor(int dx, int dy);
  void updateCursorValue(ViewUpdateDirection offset, int xOffset = 0, int yOffset = 0);
  void updateSelectionValue(ViewUpdateDirection direction);

  // Column-specific value update handlers
  void updateNoteValue(ViewUpdateDirection direction, int yOffset = 0);
  void updateInstrumentValue(ViewUpdateDirection direction, int yOffset = 0);
  void updateVolumeValue(ViewUpdateDirection direction, int yOffset = 0);
  void updateCommandValue(PhraseColumn col, ViewUpdateDirection direction, int yOffset = 0);
  void updateCommandParam(PhraseColumn col, ViewUpdateDirection direction, int yOffset = 0);
  void warpToNeighbour(int offset);
  void warpInChain(int offset);
  void cutPosition();
  void pasteLast();

  void extendSelection();

  GUIRect getSelectionRect();
  void fillClipboardData();
  void copySelection();
  void cutSelection();
  void pasteClipboard();

  void startAudition(bool startIfNotRunning);
  void stopAudition();
  void unMuteAll();
  void toggleMute();
  void switchSoloMode();

  void processNormalButtonMask(uint16_t mask);
  void processSelectionButtonMask(uint16_t mask);

  void setTextProps(int row, int col, Color textColor);
  bool getEffectiveInstrumentForRow(int row, uint8_t &instrumentId) const;

private:
  int row_;
  PhraseColumn col_;
  int lastNote_;
  int lastInstr_;
  int lastCmd_;
  int lastParam_;
  uint8_t lastVolume_;
  Phrase *phrase_;
  int lastPlayingPos_;
  Variable cmdEdit_;
  GUIPoint cmdEditPos_;
  UIBigHexVarField cmdEditField_;

  struct clipboard {
    bool active_;
    int col_;
    int row_;
    int width_;
    int height_;
    PhraseStep steps_[16];
  } clipboard_;

  PhraseColumn saveCol_;
  int saveRow_;

  // Flags to track which UI elements need updating
  // These prevent core1 from directly updating the UI
  bool needsUIUpdate_ = false; // Single flag for all UI updates (notes, VU
                               // meter, positions, live indicators)

  bool needsLiveIndicatorUpdate_ = false;

  // Info area draw mode - ensures drawNotes() and drawHelpLegend() are mutually exclusive
  InfoAreaDrawMode infoAreaMode_ = InfoAreaDrawMode::Notes;

#ifdef PICO_DEOPTIMIZED_DEBUG
  // These variables are specifically for thread synchronization in debug builds
  // They create memory barriers between cores when manipulated in a specific
  // pattern DO NOT REMOVE - they are critical for performance in debug builds
  bool syncVar1_ = false;
  bool syncVar2_ = false;
  bool syncVar3_ = false;
#endif

  // Memory barrier function that uses the sync variables in debug DEOPTIMISED
  // builds only
  inline void createMemoryBarrier() {
#ifdef PICO_DEOPTIMIZED_DEBUG
    // This specific pattern of operations was found to be necessary
    // for proper thread synchronization in debug builds
    syncVar1_ = false;
    syncVar2_ = true;
    syncVar3_ = false;
#endif
  }
};

#endif
