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

#ifndef _VIEW_H_
#define _VIEW_H_

#include <stdint.h>

#include "Application/Model/Config.h"
#include "Application/Model/Project.h"
#include "Application/Player/Player.h"
#include "Application/Utils/TintChar.h"
#include "Application/Utils/mathutils.h"
#include "Application/Utils/updateData.h"
#include "Externals/etl/include/etl/delegate.h"
#include "Foundation/Types/Colors.h"
#include "Foundation/Types/ViewType.h"
#include "I_Action.h"
#include "UIFramework/Interfaces/I_GUIGraphics.h"
#include "UIFramework/SimpleBaseClasses/GUIWindow.h"
#include "ViewEvent.h"

#define VU_METER_HEIGHT 16
#define VU_METER_MAX 159
#define VU_METER_CLIP_LEVEL 15
#define VU_METER_WARN_LEVEL 8
#define ALT_ROW_NUMBER 4 // for now const vs a user setting

#define NUM_COLORS 16

typedef enum ButtonMask : uint16_t {
  BM_LEFT = 1 << 0,  // LEFT button.
  BM_DOWN = 1 << 1,  // DOWN button.
  BM_RIGHT = 1 << 2, // RIGHT button.
  BM_UP = 1 << 3,    // UP button.
  BM_ALT = 1 << 4,   // ALT button.
  BM_EDIT = 1 << 5,  // EDIT button.
  BM_ENTER = 1 << 6, // ENTER button.
  BM_NAV = 1 << 7,   // NAV button.
  BM_PLAY = 1 << 8,  // PLAY button.

  BM_DIRECTIONAL = BM_LEFT | BM_RIGHT | BM_UP | BM_DOWN,
} ButtonMask;

// Info area draw mode - ensures drawNotes() and drawHelpLegend() are mutually exclusive
enum class InfoAreaDrawMode { Notes, HelpLegend };

enum ViewMode { VM_NORMAL, VM_NEW, VM_CLONE, VM_SELECTION, VM_MUTEON, VM_SOLOON };

#define SWITCHABLE(Y, A, B)                                                                                            \
  static constexpr Color Y(bool selected) {                                                                            \
    return selected ? A : B;                                                                                           \
  }
#define FIXED(Y, A) static constexpr Color Y = A;
struct Theme {

  struct Notes {
    SWITCHABLE(bg, LIGHT_BLUE, BLUE)
    SWITCHABLE(fg, BLACK, BLACK)
  };

  struct VU {
    FIXED(clip, LIGHT_RED)
    FIXED(warn, LIGHT_YELLOW)
    FIXED(normal, GREEN)
  };

  struct Data {
    FIXED(negative, LIGHT_RED)
    FIXED(positive, LIGHT_GREEN)
  };

  struct Waveform {
    FIXED(normal, LIGHT_GRAY)
    FIXED(baseline, DARK_GRAY)
    SWITCHABLE(marker, LIGHT_CYAN, CYAN)
    SWITCHABLE(border, LIGHT_BLUE, BLUE)
  };

  struct Input {
    FIXED(cursor, WHITE)
    FIXED(placeholder, LIGHT_GRAY)
    SWITCHABLE(label, WHITE, LIGHT_GRAY)
    SWITCHABLE(bg, LIGHT_GREEN, BLACK)
    SWITCHABLE(fg, BLACK, GREEN)
  };

  struct Button {
    SWITCHABLE(bg, LIGHT_BLUE, BLACK)
    SWITCHABLE(fg, WHITE, LIGHT_BLUE)
  };

  struct View {
    FIXED(bg, BLACK)
    FIXED(fg, WHITE)

    FIXED(inactive, LIGHT_GRAY)
    FIXED(scrollbar, LIGHT_GRAY)

    SWITCHABLE(index, LIGHT_CYAN, CYAN)
    SWITCHABLE(help, WHITE, LIGHT_GRAY)

    // states
    FIXED(warning, LIGHT_YELLOW)
    FIXED(error, LIGHT_RED)
    FIXED(info, YELLOW)
    FIXED(charging, GREEN)

    struct Map {
      SWITCHABLE(bg, LIGHT_GREEN, BLACK)
      SWITCHABLE(fg, BLACK, WHITE)
    };

    struct Selection {
      FIXED(bg, LIGHT_GREEN)
      FIXED(fg, BLACK)
    };

    struct Title {
      FIXED(bg, BLUE)
      FIXED(fg, WHITE)
    };

    struct Button {
      SWITCHABLE(fg, BLACK, WHITE)
      SWITCHABLE(bg, LIGHT_GREEN, DARK_GRAY)
    };

    struct Tab {
      SWITCHABLE(fg, BLACK, BLACK)
      SWITCHABLE(bg, LIGHT_YELLOW, LIGHT_GRAY)
    };
  };

  struct FileList {
    FIXED(directory, LIGHT_YELLOW)
    FIXED(file, Theme::View::fg)
    FIXED(icon, LIGHT_GRAY)
  };

  struct Dialog {
    FIXED(bg, LIGHT_GRAY)
    FIXED(fg, BLACK)

    struct Icon {
      FIXED(info, LIGHT_YELLOW);
      FIXED(error, LIGHT_RED)
      FIXED(success, LIGHT_GREEN)
      FIXED(warning, YELLOW)
    };

    struct Title {
      FIXED(fg, WHITE)
      FIXED(bg, LIGHT_BLUE)
    };

    struct Button {
      SWITCHABLE(fg, BLACK, WHITE)
      SWITCHABLE(bg, LIGHT_GREEN, DARK_GRAY)
    };
  };

  struct Phrase {
    SWITCHABLE(note, WHITE, LIGHT_GRAY)
    SWITCHABLE(instrument, LIGHT_BLUE, BLUE)
    SWITCHABLE(volume, LIGHT_RED, RED)
    SWITCHABLE(command1, LIGHT_YELLOW, YELLOW)
    SWITCHABLE(command2, LIGHT_GREEN, GREEN)
    SWITCHABLE(command3, LIGHT_CYAN, CYAN)
  };

  struct Song {
    struct Playback {
      FIXED(active, LIGHT_GREEN)
      FIXED(muted, LIGHT_RED)
      FIXED(live, LIGHT_YELLOW)
    };

    FIXED(placeholder, DARK_GRAY)
    SWITCHABLE(fg, WHITE, LIGHT_GRAY)
    SWITCHABLE(preview, LIGHT_GRAY, DARK_GRAY)
  };
};

enum ViewUpdateDirection : int { VUD_LEFT = 0, VUD_RIGHT = 1, VUD_UP = 2, VUD_DOWN = 3 };
const ButtonMask DirectionalButtons[4] = {BM_LEFT, BM_RIGHT, BM_UP, BM_DOWN};

class View;
class ModalView;

using ModalViewCallback = etl::delegate<void(View &v, ModalView &d)>;

class View : public Observable {
public:
  View(GUIWindow &w, ViewData *viewData);
  View(View &v);

  void SetFocus(ViewType vt) {
    viewType_ = vt;
    hasFocus_ = true;
    OnFocus();
  };

  virtual void LoseFocus() {
    if (!hasFocus_) {
      return;
    }
    hasFocus_ = false;
    OnFocusLost();
  };

  void Clear();

  void ProcessButton(uint16_t mask, bool pressed);

  void Redraw();

  bool isDirty() {
    return isDirty_;
  };

  // Override in subclasses

  virtual void DrawView() = 0;
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int currentTick) = 0;
  virtual void OnFocus() = 0;
  virtual void OnFocusLost() {};
  virtual void AnimationUpdate() = 0;

  void SetDirty(bool dirty);

  // Methods to access modal view
  bool HasModalView() const {
    return modalView_ != nullptr;
  }
  ModalView *GetModalView() const {
    return modalView_;
  }

  // Primitive locking mechanism

  bool Lock();
  void WaitForObject();
  void Unlock();

  // Char based draw routines

  virtual void SwapColors();
  virtual void SetColor(Color cd);
  virtual void SetBackgroundColor(Color cd);
  virtual void ClearTextRect(int x, int y, int w, int h);
  virtual void DrawString(int x, int y, const char *text);
  virtual void DrawTintString(int x, int y, const TintChar *data);
  virtual void DrawChar(int x, int y, const char character, bool transparent = false);
  virtual void DrawRect(const GUIRect &r, Color color);

  virtual void ConfirmedStop(Token sender);
  void OnConfirmStopDialog(View &v, ModalView &dialog);
  bool ConfirmStopPlayback(Token source);

  void DoModal(ModalView *view, ModalViewCallback cb = ModalViewCallback());
  void DismissModal();

protected:
  virtual void ProcessButtonMask(uint16_t mask, bool pressed) = 0;

  void DrawTitle(const char *format, ...);

  // to remove once everything got to viewdata

  inline void updateData(unsigned char *c, int offset, unsigned char limit, bool wrap) {
    updateDataValue(c, offset, limit, wrap);
  }

  GUIPoint GetAnchor();
  GUIPoint GetTitlePosition();

  void drawMap();
  void drawRegularNote(const GUIPoint &pos, uint8_t channel);
  void drawNotes();
  void drawRowNumbers(int x, int y, int start, int numRows);
  void drawCommandLegend(uint8_t x, uint8_t y, Token command);
  void drawScrollBar(uint16_t x, uint16_t y, uint16_t height, uint16_t index, uint16_t total);
  void drawBattery();
  void drawPlaybackIndicator();
  void drawMasterVuMeter(Player *player, bool forceRedraw = false, uint8_t xoffset = 24);
  void drawPlayTime(Player *player, GUIPoint pos);
  void drawVUMeter(int32_t leftBars, int32_t rightBars, GUIPoint pos, int vuIndex, bool forceRedraw = false);
  void DrawBorder(int32_t x, int32_t y, int32_t width, int32_t height, bool thick);
  void DrawFilledBorder(int32_t x, int32_t y, int32_t width, int32_t height, Color fill, bool half);
  void DrawWindow(int32_t x, int32_t y, int32_t width, int32_t height, const char *title);
  int DrawButton(int x, int y, const char *title, bool selected); // returns width of the drawn button
  int DrawTab(int x, int y, const char *title, bool selected);    // returns width of the drawn tab

  static int32_t amplitudeToBar(uint16_t level) {
    int dB = amplitudeToDb(level);
    // Map dB to bar levels  -60dB to 0dB range mapped to 0-159 bars
    // Optimized 159/60 ≈ 2.65 = (2.65 * 256) / 256 = 678 / 256
    // Using fixed-point: multiply by 678, then right-shift by 8 (divide by 256)
    return std::clamp<int32_t>(((dB + 60) * 678) >> 8, 0, VU_METER_MAX);
  }

  static inline void amplitudeToBars(stereosample level, int32_t *left, int32_t *right) {
    // Extract both channels
    uint16_t leftAmp = (level >> 16) & 0xFFFF;
    uint16_t rightAmp = level & 0xFFFF;
    *left = amplitudeToBar(leftAmp);
    *right = amplitudeToBar(rightAmp);
  }

public: // temp hack for modal window constructors
  GUIWindow &w_;
  ViewData *viewData_;
  bool needsRedraw_;
  bool isVisible_;

  int vuMeterCount_;
  ViewMode viewMode_;
  bool isDirty_; // .Do we need to redraw screeen
  ViewType viewType_;
  bool hasFocus_;

  // Previous VU meter values for optimization (one pair per channel + master)
  int32_t prevLeftVU_[SONG_CHANNEL_COUNT + 1];
  int32_t prevRightVU_[SONG_CHANNEL_COUNT + 1];

  Token stopPlaybackSource_;

private:
  uint16_t mask_;
  bool locked_;
  static bool initPrivate_;
  ModalView *modalView_;
  ModalViewCallback modalViewCallback_;

public:
  static int margin_;
  static int songRowCount_;
  static BatteryState batteryState_;

private:
  static BatteryState latestBatteryState_;
  static uint32_t lastBatteryDisplayFrame_;
  static bool batteryDisplayInitialized_;
};

#endif
