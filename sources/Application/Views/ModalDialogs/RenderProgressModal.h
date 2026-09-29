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

#ifndef _RENDER_PROGRESS_MODAL_H_
#define _RENDER_PROGRESS_MODAL_H_

#include "Application/Player/Player.h"
#include "Application/Views/BaseClasses/ModalView.h"
#include <cstdint>
#include <etl/string.h>

// Forward declarations
struct GUIPoint;

// Progress message box with render progress display
class RenderProgressModal : public ModalView {
public:
  enum class ProgressDisplayMode { pdmElapsedTime, pdmPercentage };

  static RenderProgressModal *Create(View &view, const char *title, const char *message,
                                     ProgressDisplayMode progressDisplayMode = ProgressDisplayMode::pdmElapsedTime);

  // Constructor taking a view, title and message
  RenderProgressModal(View &view, const char *title, const char *message, ProgressDisplayMode progressDisplayMode);

  // Virtual destructor
  virtual ~RenderProgressModal();
  virtual void Destroy() override;

  // ModalView overrides
  virtual void DrawView();
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int currentTick);
  virtual void OnFocus();
  virtual void ProcessButtonMask(uint16_t mask, bool pressed);
  virtual void AnimationUpdate();

private:
  RenderProgressModal(const RenderProgressModal &) = delete;
  RenderProgressModal &operator=(const RenderProgressModal &) = delete;

  // Helper method to draw the render progress
  void drawRenderProgress(int x, int y);
  int calculateSongRenderPercent() const;
  int getCurrentRenderedSongRow(bool *hasActive = nullptr) const;
  int getChainPhraseCount(int songRow, int channel) const;
  int calculateChannelTotalRenderUnits(int channel, int startSongRow) const;
  int calculateChannelRenderedUnits(int channel, int startSongRow) const;
  void initializeSongProgressTracking();

  // Title and message strings
  etl::string<16> title_;
  etl::string<16> message_;

  // Track total rendered samples (calculated from player updates)
  float totalSamples_;
  bool renderComplete_ = false;
  bool renderStarted_ = false;

  ProgressDisplayMode progressDisplayMode_;
  const uint32_t dialogWidth_ = 20;
  const uint32_t dialogHeight_ = 13;
  int startSongRow_ = 0;
  int renderedUnits_ = 0;
  int totalRenderUnits_ = 1;
  int progressChannel_ = -1;
  bool startSongRowCaptured_ = false;

    uint8_t percentDone_;

  unsigned int animationFrame_ = 0;

  static bool inUse_;
  static void *storage_;
};

#endif // _RENDER_PROGRESS_MODAL_H_
