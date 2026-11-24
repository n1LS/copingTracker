/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef _SAMPLE_SLICES_VIEW_H_
#define _SAMPLE_SLICES_VIEW_H_

#include "BaseClasses/UIActionField.h"
#include "BaseClasses/UIHexVarField.h"
#include "BaseClasses/UIIntVarField.h"
#include "BaseClasses/UIStaticField.h"
#include "FieldView.h"
#include "Foundation/Observable.h"
#include "Foundation/Variables/Variable.h"
#include "Foundation/Variables/WatchedVariable.h"
#include "GraphField.h"
#include "System/System/System.h"
#include "ViewData.h"
#include <cstdint>

class SampleInstrument;

class SampleSlicesView : public FieldView, public I_Observer {
public:
  SampleSlicesView(GUIWindow &w, ViewData *data);
  ~SampleSlicesView() override;
  void Reset();

  void ButtonDown(uint16_t newButton, uint16_t mask) override;
  bool navigationDown(uint16_t direction) override;
  void ButtonUp(uint16_t button, uint16_t mask) override;
  void DrawView() override;
  void OnPlayerUpdate(PlayerEventType, unsigned int) override {};
  void OnFocus() override;
  void LoseFocus() override;
  void AnimationUpdate() override;
  void Update(Observable &o, I_ObservableData *d) override;

private:
  static void AutoSliceConfirmCallback(View &v, ModalView &dialog);
  static void ResetSlicesConfirmCallback(View &v, ModalView &dialog);

  void clearAfterModal(View &view);
  void buildFieldLayout();
  void rebuildWaveform();
  void drawWaveform();
  void drawStatusLabels();
  void drawHelpLegend();
  void clearWaveformRegion();
  bool refreshSampleSize();
  SampleInstrument *currentInstrument();
  void updateSliceSelectionFromInstrument();
  void applySliceStart(uint32_t start);
  void autoSliceEvenly();
  void updateZoomLimits();
  bool updateZoomWindow();
  void adjustZoom(int32_t delta);
  void startPreview();
  void stopPreview();
  void handleSliceSelectionChange();
  uint32_t sliceEndForIndex(size_t index, uint32_t start) const;
  uint32_t selectedSliceStart();
  // EDIT + ENTER + LEFT / RIGHT: moves the selected slice point to the next zero crossing in the direction. The
  // slice points it passes pile up on it, to the right as well as to the left. Without another zero crossing it
  // moves to the end of the sample on that side.
  void moveSliceToZeroCrossing(int direction);
  // The slice points from the maximum number of slices on (the field below the graph) are undefined, 0
  void limitSliceCount();
  bool hasInstrumentSample() const;

  void clearSliceMarkers();
  void updateSliceMarkers();
  int32_t definedSliceCount() const;
  void resetSlices();

  WatchedVariable sliceIndexVar_;
  WatchedVariable sliceStartVar_;
  Variable sliceCountVar_;

  etl::vector<UIIntVarField, 2> intVarField_;
  etl::vector<UIActionField, 2> actionField_;

  SampleInstrument *instrument_;
  int32_t instrumentIndex_;
  uint32_t sampleSize_;
  GUIPoint graphFieldPos_;
  GraphField graphField_;
  bool initialDraw_;
  bool hadModal_;

  bool playKeyHeld_;
  bool previewActive_;
  uint8_t previewNote_;
  System *sys_;
  uint32_t previewStartMs_;
  uint32_t previewStartSample_;
  uint32_t previewEndSample_;
  float previewDurationMs_;
  uint32_t previewPlayheadSample_;
  bool previewCursorVisible_;
};

#endif
