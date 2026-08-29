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

#include "GUIWindow.h"
#include "UIFramework/Interfaces/I_GUIWindowFactory.h"
#include "UIFramework/Interfaces/I_GUIWindowImp.h"

// Constructor: We wrap the window around an implementation that
// will be used to provide system functionalities

GUIWindow::GUIWindow(I_GUIWindowImp &imp) {
  _imp = &imp;
  _imp->SetWindow(*this); // We tell the imp on which windows it works so
                          // we can be notified of system events
}

// Destructor

GUIWindow::~GUIWindow() {
  delete _imp;
}

// I_GUIGraphics Implementation: We rely on the imp window to provide
// core graphics on the window

void GUIWindow::SetColor(Color color) {
  _imp->SetColor(color);
}

void GUIWindow::SetBackgroundColor(Color color) {
  _imp->SetBackgroundColor(color);
}

void GUIWindow::ClearTextRect(GUIRect &r) {
  _imp->ClearTextRect(r);
}

void GUIWindow::DrawString(int x, int y, const char *string) {
  _imp->DrawString(x, y, string);
}

void GUIWindow::SetCurrentRectColor(Color color) {
  _imp->SetColor(color);
}

void GUIWindow::DrawRect(const GUIRect &r) {
  _imp->DrawRect(r);
}

void GUIWindow::DrawChar(int x, int y, const char c, bool transparent) {
  _imp->DrawChar(x, y, c, transparent);
}

void GUIWindow::Clear() {
  _imp->Clear();
}

GUIRect GUIWindow::GetRect() {
  return _imp->GetRect();
}

void GUIWindow::Invalidate() {
  _imp->Invalidate();
}

void GUIWindow::Flush() {
  _imp->Flush();
}

void GUIWindow::Lock() {
  _imp->Lock();
}
void GUIWindow::Unlock() {
  _imp->Unlock();
}

void GUIWindow::Update(bool redraw) {
  onUpdate(redraw);
}

void GUIWindow::ClockTick() {
  AnimationUpdate();
}

I_GUIGraphics *GUIWindow::GetGraphics() {
  return this;
}

I_GUIGraphics *GUIWindow::GetDC() {
  return this;
}

// Redifine the event Dispatcher to handle focused controll

bool GUIWindow::DispatchEvent(GUIEvent &event) {
  return onEvent(event);
  ;
}

void GUIWindow::PushEvent(GUIEvent &event) {
  _imp->PushEvent(event);
}

void GUIWindow::SetFocusRect(const GUIRect &rect) {
  focusRect_ = rect;
}

const GUIRect &GUIWindow::GetFocusRect() const {
  return focusRect_;
}