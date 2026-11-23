/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#ifndef _GRAPH_FIELD_H_
#define _GRAPH_FIELD_H_

#include "Application/Instruments/SampleInstrument.h"
#include "BaseClasses/UIField.h"
#include "BaseClasses/View.h"
#include "WaveformBuilder.h"
#include <cstdint>

class GraphField : public UIField {
public:
  static constexpr int32_t BitmapWidth = 320;
  static constexpr int32_t BitmapHeight = 80;

  static constexpr int32_t CacheSize = BitmapWidth;
  static_assert(CacheSize == WaveformBuilder::Columns, "one column of the waveform per pixel");
  static constexpr size_t MaxMarkers = SampleInstrument::MaxSlices + 3;

  GraphField(GUIPoint &position, int32_t width, int32_t height);
  ~GraphField() override = default;

  void Draw(GUIWindow &w, int offset = 0) override;
  void OnClick() override {};
  void ProcessArrow(uint16_t) override {};

  void Reset();
  void SetShowBaseline(bool show);
  void SetBorderColors(Color normal, Color focused);
  void SetSampleSize(uint32_t sampleSize);

  int GetFocusWidth() {
    return 0;
  }
  int GetFocusOffset() {
    return 0;
  }

  uint32_t SampleSize() const;
  uint8_t ZoomLevel() const;
  uint8_t MaxZoomLevel() const;
  uint32_t ViewStart() const;
  uint32_t ViewEnd() const;
  bool WaveformValid() const;

  bool UpdateZoomWindow(uint32_t centerSample);
  bool AdjustZoom(int32_t delta, uint32_t centerSample);

  // Builds the waveform of the view window: Begin, then every sample of the window in order, then Finalize
  void BeginWaveformBuild();
  void AccumulateSample(uint32_t sampleIndex, int16_t sampleValue);
  void FinalizeWaveformBuild();

  void InvalidateWaveform();
  void SetWaveformValid(bool valid);
  void RequestFullRedraw();

  void SetMarker(size_t index, uint32_t sample, bool visible);
  void SetMarkerCount(size_t count);

  int32_t SampleToPixel(uint32_t sample) const;
  void DrawGraph(View &view);
  void DrawMarker(View &view, int index);
  void DrawMarkers(View &view);

  Color colorForIndex(int i);

private:
  void redrawWaveformColumn(View &view, int32_t x);
  // the rows of a column of the waveform, false if there is nothing to draw
  bool columnRows(int32_t column, int32_t &top, int32_t &bottom) const;
  void drawWaveform(View &view);
  // the number of samples the view window shows at a zoom level
  uint32_t spanForLevel(uint8_t level) const;
  int32_t columns() const;
  void drawMarkersAt(View &view, int32_t x);
  void resetMarkerCache();
  bool hasValidWindow() const;

  int32_t width_;
  int32_t height_;
  bool showBaseline_;
  Color borderNormal_;
  Color borderFocused_;

  static WaveformBuilder builder_;
  bool waveformValid_;
  bool needsFullRedraw_;

  uint32_t sampleSize_;
  uint8_t zoomLevel_;
  uint8_t maxZoomLevel_;
  uint32_t viewStart_;
  uint32_t viewEnd_;

  struct Marker {
    uint32_t sample = 0;
    int32_t x;
    bool visible = false;
  };

  Marker markers_[MaxMarkers];
  size_t markerCount_;
  bool markerVisibleCache_[MaxMarkers];
};

#endif
