/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "GraphField.h"

#include "Application/AppWindow.h"
#include <algorithm>
#include <cmath>
#include <cstring>

uint8_t GraphField::waveformCache_[CacheSize];
int32_t GraphField::rmsSumSquares_[CacheSize];
uint16_t GraphField::rmsCounts_[CacheSize];

GraphField::GraphField(GUIPoint &position, int32_t width, int32_t height)
    : UIField(position), width_(width), height_(height), showBaseline_(false), borderNormal_(Theme::View::bg),
      borderFocused_(Theme::View::fg), waveformValid_(false), needsFullRedraw_(true), sampleSize_(0), zoomLevel_(0),
      maxZoomLevel_(0), viewStart_(0), viewEnd_(0), markerCount_(0) {
  std::memset(waveformCache_, 0, sizeof(waveformCache_));
  resetMarkerCache();
}

void GraphField::Draw(GUIWindow &w, int offset) {
}

Color GraphField::colorForIndex(int i) {
  return 1 + (8 + i) % 15;
}

void GraphField::Reset() {
  showBaseline_ = false;
  borderNormal_ = Theme::View::bg;
  borderFocused_ = Theme::View::fg;
  waveformValid_ = false;
  needsFullRedraw_ = true;
  sampleSize_ = 0;
  zoomLevel_ = 0;
  maxZoomLevel_ = 0;
  viewStart_ = 0;
  viewEnd_ = 0;
  markerCount_ = 0;
  std::memset(waveformCache_, 0, sizeof(waveformCache_));
  resetMarkerCache();
}

void GraphField::SetShowBaseline(bool show) {
  showBaseline_ = show;
}

void GraphField::SetBorderColors(Color normal, Color focused) {
  borderNormal_ = normal;
  borderFocused_ = focused;
}

void GraphField::SetSampleSize(uint32_t sampleSize) {
  sampleSize_ = sampleSize;
  maxZoomLevel_ = 0;
  uint32_t span = sampleSize_;
  while (span > static_cast<uint32_t>(CacheSize) && maxZoomLevel_ < 16) {
    span = (span + 1) / 2;
    maxZoomLevel_++;
  }
  if (zoomLevel_ > maxZoomLevel_) {
    zoomLevel_ = maxZoomLevel_;
  }
}

uint32_t GraphField::SampleSize() const {
  return sampleSize_;
}

uint8_t GraphField::ZoomLevel() const {
  return zoomLevel_;
}

uint8_t GraphField::MaxZoomLevel() const {
  return maxZoomLevel_;
}

uint32_t GraphField::ViewStart() const {
  return viewStart_;
}

uint32_t GraphField::ViewEnd() const {
  return viewEnd_;
}

uint8_t *GraphField::WaveformCache() {
  return waveformCache_;
}

bool GraphField::WaveformValid() const {
  return waveformValid_;
}

bool GraphField::UpdateZoomWindow(uint32_t centerSample) {
  if (sampleSize_ == 0) {
    bool changed = (viewStart_ != 0 || viewEnd_ != 0);
    viewStart_ = 0;
    viewEnd_ = 0;
    return changed;
  }

  uint32_t zoomFactor = (zoomLevel_ < 31) ? (static_cast<uint32_t>(1) << zoomLevel_) : 0;
  if (zoomFactor == 0) {
    zoomFactor = 1;
  }
  uint32_t viewSpan = sampleSize_ / zoomFactor;
  if (viewSpan == 0) {
    viewSpan = 1;
  }
  if (viewSpan >= sampleSize_) {
    viewSpan = sampleSize_;
  }

  uint32_t center = std::min(centerSample, sampleSize_ > 0 ? sampleSize_ - 1 : 0U);

  uint32_t start = 0;
  if (viewSpan < sampleSize_) {
    uint32_t half = viewSpan / 2;
    if (center > half) {
      start = center - half;
    }
    if (start + viewSpan > sampleSize_) {
      start = sampleSize_ - viewSpan;
    }
  }
  uint32_t end = start + viewSpan;

  bool changed = (start != viewStart_) || (end != viewEnd_);
  viewStart_ = start;
  viewEnd_ = end;
  if (changed) {
    resetMarkerCache();
  }
  return changed;
}

bool GraphField::AdjustZoom(int32_t delta, uint32_t centerSample) {
  if (sampleSize_ == 0) {
    return false;
  }
  int32_t newLevel = static_cast<int32_t>(zoomLevel_) + static_cast<int32_t>(delta);
  if (newLevel < 0) {
    newLevel = 0;
  }
  if (newLevel > static_cast<int32_t>(maxZoomLevel_)) {
    newLevel = static_cast<int32_t>(maxZoomLevel_);
  }
  if (newLevel == static_cast<int32_t>(zoomLevel_)) {
    return false;
  }
  zoomLevel_ = static_cast<uint8_t>(newLevel);
  return UpdateZoomWindow(centerSample);
}

void GraphField::BeginRmsBuild() {
  waveformValid_ = false;
  std::memset(waveformCache_, 0, sizeof(waveformCache_));
  std::fill_n(rmsSumSquares_, CacheSize, int32_t{0});
  std::fill_n(rmsCounts_, CacheSize, uint16_t{0});
}

void GraphField::AccumulateRmsSample(uint32_t sampleIndex, int16_t sampleValue) {
  if (!hasValidWindow()) {
    return;
  }
  if (sampleIndex < viewStart_ || sampleIndex >= viewEnd_) {
    return;
  }
  uint32_t rel = sampleIndex - viewStart_;
  uint32_t viewSpan = viewEnd_ - viewStart_;
  uint32_t pixel = static_cast<uint32_t>((static_cast<uint64_t>(rel) * CacheSize) / viewSpan);
  if (pixel >= static_cast<uint32_t>(CacheSize)) {
    pixel = CacheSize - 1;
  }
  int16_t quant = static_cast<int16_t>(sampleValue >> 8);
  rmsSumSquares_[pixel] += static_cast<int32_t>(quant) * quant;
  rmsCounts_[pixel]++;
}

void GraphField::FinalizeRmsBuild() {
  if (!hasValidWindow()) {
    waveformValid_ = false;
    return;
  }
  for (int32_t i = 0; i < CacheSize; ++i) {
    if (rmsCounts_[i] == 0) {
      waveformCache_[i] = 0;
      continue;
    }
    float meanSquare = static_cast<float>(rmsSumSquares_[i]) / rmsCounts_[i];
    float rms = std::sqrt(meanSquare) / 128.0f;
    uint8_t height = static_cast<uint8_t>(std::min<float>(rms * height_, static_cast<float>(height_)));
    waveformCache_[i] = height;
  }
  waveformValid_ = true;
  needsFullRedraw_ = true;
}

void GraphField::ClearWaveformCache() {
  std::memset(waveformCache_, 0, sizeof(waveformCache_));
}

void GraphField::InvalidateWaveform() {
  waveformValid_ = false;
  needsFullRedraw_ = true;
}

void GraphField::SetWaveformValid(bool valid) {
  waveformValid_ = valid;
  if (valid) {
    needsFullRedraw_ = true;
  }
}

void GraphField::RequestFullRedraw() {
  needsFullRedraw_ = true;
}

void GraphField::SetMarker(size_t index, uint32_t sample, bool visible) {
  if (index >= MaxMarkers) {
    return;
  }

  markers_[index].sample = sample;
  markers_[index].visible = true; // visible;
}

int32_t GraphField::SampleToPixel(uint32_t sample) const {
  if (!hasValidWindow() || sample < viewStart_ || sample >= viewEnd_) {
    return -1;
  }

  uint32_t clamped = std::min(sample, sampleSize_ > 0 ? sampleSize_ - 1 : 0U);
  uint32_t viewSpan = viewEnd_ - viewStart_;
  uint32_t rel = clamped - viewStart_;
  int32_t local = static_cast<int32_t>((static_cast<uint64_t>(rel) * width_) / viewSpan);
  
  return static_cast<int32_t>(x_) + 1 + local;
}

void GraphField::DrawGraph(View &view) {
  if (needsFullRedraw_) {
    // clear area

    GUIRect area(static_cast<int32_t>(x_), static_cast<int32_t>(y_), static_cast<int32_t>(x_) + width_,
                 static_cast<int32_t>(y_) + height_);
    view.DrawRect(area, Theme::View::bg);

    // baseline

    if (showBaseline_) {
      int32_t centerY = static_cast<int32_t>(y_) + height_ / 2;
      GUIRect baseline(static_cast<int32_t>(x_) + 1, centerY, static_cast<int32_t>(x_) + width_ - 1, centerY + 1);
      view.DrawRect(baseline, Theme::Waveform::baseline);
    }

    // draw waveform

    if (waveformValid_ && hasValidWindow()) {
      int32_t centerY = static_cast<int32_t>(y_) + height_ / 2;
      int32_t maxColumns = std::min<int32_t>(width_, static_cast<int32_t>(CacheSize));
      for (int32_t x = 0; x < maxColumns; ++x) {
        uint8_t amplitude = waveformCache_[x];
        if (amplitude == 0) {
          continue;
        }
        int32_t startY = centerY - amplitude / 2;
        int32_t endY = startY + amplitude;
        if (startY < static_cast<int32_t>(y_) + 1) {
          startY = static_cast<int32_t>(y_) + 1;
        }
        if (endY > static_cast<int32_t>(y_) + height_) {
          endY = static_cast<int32_t>(y_) + height_;
        }
        GUIRect column(static_cast<int32_t>(x_) + 1 + x, startY, static_cast<int32_t>(x_) + 2 + x, endY);
        view.DrawRect(column, Theme::View::fg);
      }
    }

    // draw markers
    if (sampleSize_ > 0) {
      for (size_t i = 0; i < markerCount_; ++i) {
        if (!markers_[i].visible) {
          markers_[i].x = -1;
          markerVisibleCache_[i] = false;
          continue;
        }
        int32_t markerX = SampleToPixel(markers_[i].sample);
        if (markerX < 0) {
          markers_[i].x = -1;
          markerVisibleCache_[i] = false;
          continue;
        }
        GUIRect marker(markerX, static_cast<int32_t>(y_) + 2, markerX + 1, static_cast<int32_t>(y_) + height_);
        view.DrawRect(marker, colorForIndex(i));
        markers_[i].x = static_cast<int16_t>(markerX);
        markerVisibleCache_[i] = true;
      }
    } else {
      resetMarkerCache();
    }

    needsFullRedraw_ = false;
    return;
  }

  if (!hasValidWindow()) {
    return;
  }

  for (size_t i = 0; i < 16; ++i) {
    if (markers_[i].x >= 0) {
      redrawWaveformColumn(view, markers_[i].x);
    }
    
    view.SetColor(Theme::View::fg);
    int32_t x = SampleToPixel(markers_[i].sample);
    if (x >= 0) {
      GUIRect marker(x, static_cast<int32_t>(y_) + 2, x + 1, static_cast<int32_t>(y_) + height_);
      view.DrawRect(marker, colorForIndex(i));
    }

    markers_[i].x = x;
  }
  return;
}

void GraphField::redrawWaveformColumn(View &view, int32_t x) {
  int32_t left = static_cast<int32_t>(x_);
  int32_t right = static_cast<int32_t>(x_) + width_;
  if (x <= left || x > right) {
    return;
  }
  GUIRect clearRect(x, static_cast<int32_t>(y_) + 1, x + 1, static_cast<int32_t>(y_) + height_);
  view.DrawRect(clearRect, Theme::View::bg);

  if (showBaseline_) {
    int32_t centerY = static_cast<int32_t>(y_) + height_ / 2;
    GUIRect baseline(x, centerY, x + 1, centerY + 1);
    view.DrawRect(baseline, Theme::Waveform::baseline);
  }

  if (!waveformValid_ || !hasValidWindow()) {
    return;
  }
  int32_t cacheIndex = x - static_cast<int32_t>(x_) - 1;
  if (cacheIndex < 0 || cacheIndex >= CacheSize) {
    return;
  }
  uint8_t amplitude = waveformCache_[cacheIndex];
  if (amplitude == 0) {
    return;
  }
  int32_t centerY = static_cast<int32_t>(y_) + height_ / 2;
  int32_t startY = centerY - amplitude / 2;
  int32_t endY = startY + amplitude;
  
  startY = std::max(startY, static_cast<int32_t>(y_) + 1);
  endY = std::min(endY, static_cast<int32_t>(y_) + height_ + 1);
  
  GUIRect column(x, startY, x + 1, endY);
  view.DrawRect(column, Theme::View::fg);
}

void GraphField::drawMarkersAt(View &view, int32_t x) {
  if (!hasValidWindow()) {
    return;
  }

  for (size_t i = 0; i < markerCount_; ++i) {
    if (!markers_[i].visible) {
      continue;
    }
    int32_t markerX = SampleToPixel(markers_[i].sample);
    if (markerX != x) {
      continue;
    }
    GUIRect marker(x, static_cast<int32_t>(y_) + 2, x + 1, static_cast<int32_t>(y_) + height_);
    view.DrawRect(marker, colorForIndex(i));
  }
}

void GraphField::resetMarkerCache() {
  for (size_t i = 0; i < MaxMarkers; ++i) {
    markers_[i].x = -1;
    markerVisibleCache_[i] = false;
    markers_[i].sample = 0;
    markers_[i].visible = false;
  }
}

bool GraphField::hasValidWindow() const {
  return sampleSize_ > 0 && viewEnd_ > viewStart_;
}

void GraphField::SetMarkerCount(size_t count) {
  if (count > MaxMarkers) {
    count = MaxMarkers;
  }

  if (count != markerCount_) {
    markerCount_ = count;
    resetMarkerCache();
    needsFullRedraw_ = true;
  }
}