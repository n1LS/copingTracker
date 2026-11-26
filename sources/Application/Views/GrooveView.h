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

#ifndef _GROOVE_VIEW_H_
#define _GROOVE_VIEW_H_

#include "ScreenView.h"
#include "ViewData.h"

class GrooveView : public ScreenView {
public:
  GrooveView(GUIWindow &w, ViewData *viewData);
  ~GrooveView();
  void Reset();
  virtual void ButtonDown(uint16_t newButton, uint16_t mask);
  bool navigationDown(uint16_t direction) override;
  virtual void DrawView();
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int tick = 0);
  virtual void OnFocus();
  bool SupportsUndo() const override {
    return true;
  }
  ContextLegend currentContextLegend() const override;
  void AfterUndoRedo() override;
  virtual void ButtonUp(uint16_t button, uint16_t mask) override;

protected:
  void updateCursorValue(int val, bool sync = false);
  void updateCursor(int dir);
  // the focus is the cell of the cursor
  void updateFocusRect();
  void initCursorValue();
  void clearCursorValue();
  void warpGroove(int dir);
  void processNormalButtonMask(uint16_t newButton, uint16_t mask);
  void processSelectionButtonMask(uint16_t newButton, uint16_t mask);

  // the selection starts at the cursor
  void beginSelection();
  // the rows of the selection, the first and the last
  void selectionRows(int &first, int &last) const;
  void fillClipboardData();
  void cutPosition();
  void copySelection();
  void cutSelection();
  void pasteClipboard();
  // ALT + EDIT: all rows (a second time it flips to the other end)
  void extendSelection();
  // ENTER + EDIT + UP / DOWN: moves the selected rows one row, the row in the way jumps to the other end
  void nudgeSelection(int delta);

private:
  int position_;
  int lastPosition_;

  // the copied values, and the row the selection started at
  struct {
    int row_;
    int height_;
    unsigned char data_[16];
    bool active_;
  } clipboard_;
  int saveRow_;
};
#endif
