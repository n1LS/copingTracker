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

#ifndef _TABLE_VIEW_H_
#define _TABLE_VIEW_H_

#include "Application/Model/Table.h"
#include "BaseClasses/UIBigHexVarField.h"
#include "ScreenView.h"
#include "ViewData.h"

namespace {

typedef enum TableColumn {
  colCmd1,
  colCmdVal1,
  colCmd2,
  colCmdVal2,
  colCmd3,
  colCmdVal3,
} TableColumn;

};
class TableView : public ScreenView {
public:
  TableView(GUIWindow &w, ViewData *viewData);
  ~TableView();
  void Reset();
  virtual void ProcessButtonMask(uint16_t mask, bool pressed);
  virtual void DrawView();
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int tick = 0);
  virtual void OnFocus();
  virtual void AnimationUpdate();
  void setCurrentlySelectedCommand(Token command);

protected:
  void processNormalButtonMask(uint16_t mask);
  void processSelectionButtonMask(uint16_t mask);

  void cutPosition();
  void pasteLast();
  void copySelection();
  void cutSelection();
  void pasteClipboard();

  void fillClipboardData();
  void extendSelection();

  void updateCursor(int dx, int dy);
  void updateCursorValue(int offset);
  void setTextProps(int row, int col, Color color);
  void warpToNeighbour(int dir);

  GUIRect getSelectionRect();

private:
  int8_t row_;
  int8_t col_;
  uint8_t lastVol_;
  uint8_t lastTick_;
  uint8_t lastTsp_;
  int lastCmd_;
  int lastParam_;

  Variable cmdEdit_;
  GUIPoint cmdEditPos_;
  UIBigHexVarField cmdEditField_;

  struct clipboard {
    bool active_;
    int col_;
    int row_;
    int width_;
    int height_;
    TableStep steps_[16];
  } clipboard_;

  int saveCol_;
  int saveRow_;

  // Flags to track which UI elements need updating
  // These prevent core1 from directly updating the UI
  bool needsUIUpdate_ = false; // Single flag for all UI updates (notes, VU meter, positions)

  // Keep these for backward compatibility
  bool needsPlayPositionUpdate_ = false;
  bool needsNotesUpdate_ = false;

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

  uint8_t lastPosition_[3];
};

#endif
