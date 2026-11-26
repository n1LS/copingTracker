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
#include "PhraseEditingView.h"
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

class PhraseView : public PhraseEditingView {

public:
  PhraseView(GUIWindow &w, ViewData *viewData);
  ~PhraseView();
  void Reset();
  virtual void ButtonDown(uint16_t newButton, uint16_t mask);
  virtual void ButtonUp(uint16_t button, uint16_t mask);
  virtual void DrawView();
  ContextLegend currentContextLegend() const override;
  void DrawEditingValue();
  virtual void OnFocus();
  bool SupportsUndo() const override {
    return true;
  }
  void AfterUndoRedo() override;
  virtual void AnimationUpdate();
  void onCommandPicked(Token command) override;

protected:
  void updateCursor(int dx, int dy);
  void updateCursorValue(ViewUpdateDirection offset, int xOffset = 0, int yOffset = 0);
  void updateSelectionValue(ViewUpdateDirection direction);
  // ENTER + EDIT + UP / DOWN: moves the content of the selection one step, the step in the way jumps to the other end
  void nudgeSelection(int delta);

  // Column-specific value update handlers
  void updateNoteValue(ViewUpdateDirection direction, int yOffset = 0);
  void updateInstrumentValue(ViewUpdateDirection direction, int yOffset = 0);
  void updateVolumeValue(ViewUpdateDirection direction, int yOffset = 0);
  void updateCommandValue(PhraseColumn col, ViewUpdateDirection direction, int yOffset = 0);
  void updateCommandParam(PhraseColumn col, ViewUpdateDirection direction, int yOffset = 0);
  void cutPosition();
  void pasteLast();

  void extendSelection();
  // starts a selection at the cursor, if there is none
  void beginSelection();

  GUIRect getSelectionRect();
  void fillClipboardData();
  void copySelection();
  void cutSelection();
  void pasteClipboard();

  // PhraseEditingView
  ViewType otherView() const override {
    return VT_PIANOROLL;
  }
  int cursorStep() const override {
    return row_;
  }
  void onPhraseSwitched() override;

  void processNormalButtonMask(uint16_t newButton, uint16_t mask);
  void processSelectionButtonMask(uint16_t newButton, uint16_t mask);

  void setTextProps(int row, int col, Color textColor, int limit);

private:
  int phraseDiv_ = 4;
  int row_;
  PhraseColumn col_;
  Phrase *phrase_;
  int lastPlayingPos_;

  struct clipboard {
    int col_;
    int row_;
    int width_;
    int height_;
    bool active_;
  } clipboard_;

  PhraseColumn saveCol_;
  int saveRow_;
  int expandFrame_ = 0;

  // (needsUIUpdate_, the single flag that keeps core1 from drawing, is in PhraseEditingView)

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
