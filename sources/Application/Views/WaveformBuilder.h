/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _WAVEFORM_BUILDER_H_
#define _WAVEFORM_BUILDER_H_

#include <algorithm>
#include <cstdint>
#include <cstring>

// Turns the samples of a view window into what is drawn for every column of the graph: the lowest and highest
// value of its samples (the envelope), in 8 bit (-128..127).
//
// A window of `span` samples is shown in `width` columns, a sample belongs to the column
// floor(rel * width / span), where rel is its position in the window. That is the same mapping the markers use.
//  - With more samples than columns (span > width) every column has at least one sample, the envelope is the
//    range of its samples.
//  - With fewer samples than columns (span <= width) a sample is held over the columns up to the next sample,
//    so there are no gaps however far it is zoomed in. At one sample per column that is the signal itself.
// The envelope of a column also includes the sample before it, so neighbouring columns touch and a steep
// signal is a line and not a row of dots.
//
// The samples have to be added in order: Begin(), Add() for every sample of the window from rel 0 up, Finish().
// Only the column that is built is kept as a running value, so the two arrays below are all there is.
class WaveformBuilder {
public:
  static constexpr int32_t Columns = 320;

  void Begin(uint32_t span, int32_t width) {
    span_ = span;
    width_ = std::clamp<int32_t>(width, 0, Columns);
    std::memset(min_, 0, sizeof(min_));
    std::memset(max_, 0, sizeof(max_));
    active_ = span_ > 0 && width_ > 0;
    hasData_ = false;
    column_ = -1;
  }

  void Add(uint32_t rel, int16_t sample) {
    if (!active_ || rel >= span_) {
      return;
    }

    // 16 bit to 8 bit, arithmetic shift: -32768 is -128, 32767 is 127
    const int8_t value = static_cast<int8_t>(sample >> 8);
    const int32_t column = ColumnOf(rel, span_, width_);

    if (column != column_) {
      flushColumn();
      // the columns between the samples (more columns than samples) hold the last sample
      if (hasData_) {
        for (int32_t c = column_ + 1; c < column; ++c) {
          setColumn(c, last_, last_);
        }
      }
      column_ = column;
      // the envelope starts at the sample before, so that the columns touch
      accMin_ = accMax_ = hasData_ ? last_ : value;
    }

    accMin_ = std::min(accMin_, value);
    accMax_ = std::max(accMax_, value);
    last_ = value;
    hasData_ = true;
  }

  void Finish() {
    flushColumn();
    if (hasData_) {
      // the last sample is held to the end of the window
      for (int32_t c = column_ + 1; c < width_; ++c) {
        setColumn(c, last_, last_);
      }
    }
    column_ = -1;
    active_ = false;
  }

  // whether there was anything to build the waveform of
  bool HasData() const {
    return hasData_;
  }

  int8_t Min(int32_t column) const {
    return min_[column];
  }
  int8_t Max(int32_t column) const {
    return max_[column];
  }

  // the column a sample of a window of `span` samples is in, shown in `width` columns
  static uint32_t ColumnOf(uint32_t rel, uint32_t span, int32_t width) {
    return static_cast<uint32_t>((static_cast<uint64_t>(rel) * static_cast<uint32_t>(width)) / span);
  }

private:
  void setColumn(int32_t column, int8_t low, int8_t high) {
    if (column < 0 || column >= width_) {
      return;
    }
    min_[column] = low;
    max_[column] = high;
  }

  // a column that was started always has a sample
  void flushColumn() {
    if (column_ >= 0) {
      setColumn(column_, accMin_, accMax_);
    }
  }

  int8_t min_[Columns];
  int8_t max_[Columns];

  uint32_t span_ = 0;
  int32_t width_ = 0;
  bool active_ = false;
  bool hasData_ = false;

  // the column that is built and its running values
  int32_t column_ = -1;
  int8_t accMin_ = 0;
  int8_t accMax_ = 0;
  int8_t last_ = 0;
};

#endif
