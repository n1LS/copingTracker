#pragma once

#include "Application/AppWindow.h"
#include "Application/Views/BaseClasses/View.h"
#include "System/System/System.h"
#include <string.h>

constexpr int maxLineWidth = (SCREEN_WIDTH - 6);
constexpr int maxLines = 8;

struct ToastType {
  const char *symbol;
  Color color;
};

enum ToastDuration {
  regular = 1500,
};

constexpr ToastType ttInfo = {"i", Theme::Dialog::Icon::info};
constexpr ToastType ttError = {"X", Theme::Dialog::Icon::error};
constexpr ToastType ttSuccess = {"I", Theme::Dialog::Icon::success};
constexpr ToastType ttWarning = {"!", Theme::Dialog::Icon::warning};

class ToastView : public View {
public:
  virtual ~ToastView();

  static void Init(GUIWindow &w, ViewData *viewData);
  static ToastView *GetInstance();

  void Show(const char *text, const ToastType *type, uint32_t msTime);
  void Draw(GUIWindow &w);
  void UpdateTimer();

private:
  static ToastView *instance_;
  char lines_[maxLines][SCREEN_WIDTH + 1];
  ToastType type_ = ttInfo;
  uint32_t dismissTime_ = 0;
  uint32_t animationStartTime_ = 0;
  int32_t lineCount_ = 0;
  int animationOffset_ = 0;
  bool visible_ = false;

  ToastView(GUIWindow &w, ViewData *viewData);
  void WrapText(const char *message);

  // view virtual methods
  virtual void OnFocus() override {};
  virtual void DrawView() override {};
  virtual void AnimationUpdate() override {};
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int tick) override {};
  virtual void ProcessButtonMask(unsigned short mask, bool pressed) override {};
};