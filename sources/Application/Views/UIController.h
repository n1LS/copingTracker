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

#ifndef _UI_CONTROLLER_H
#define _UI_CONTROLLER_H

#include "Foundation/T_Singleton.h"
#include "ViewData.h"

class UIController : public T_Singleton<UIController> {
private: // Singleton
  friend class etl::singleton<UIController>;
  UIController();

public:
  void Init(Project *, ViewData *);
  void Reset();

  // Muting functions

  void UnMuteAll();

  // Mute and solo are chords that work while they are held: EDIT + NAV toggles the mute of the channels, ENTER + NAV
  // solos them (a second ENTER + NAV, or UnMuteAll, ends a solo that was kept). Letting go of the second button
  // (EDIT / ENTER) while NAV is still held takes it back, letting go of NAV first keeps it. Other buttons that are
  // pressed or let go in between change nothing, so the views do not need to know about any of it.
  void BeginMute(int from, int to);
  void BeginSolo(int from, int to);
  // every button release, from View::ProcessButton
  void OnButtonUp(uint16_t button, uint16_t mask);

private:
  void toggleMute(int from, int to);
  void solo(int from, int to);
  void endSolo();

  enum class Armed : uint8_t { None, Mute, Solo };

  Project *project_;
  ViewData *viewData_;

  // the chord that is held at the moment, and what it works on
  Armed armed_ = Armed::None;
  int armedFrom_ = 0;
  int armedTo_ = 0;

  // the channels were soloed, soloMask_ holds what was muted before
  bool soloActive_ = false;
  bool soloMask_[SONG_CHANNEL_COUNT];
};
#endif
