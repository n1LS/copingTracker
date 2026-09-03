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

#include "DeviceView.h"
#include "Application/AppWindow.h"
#include "Application/Commands/EventDispatcher.h"
#include "Application/Model/Scale.h"
#include "Application/Persistency/PersistencyService.h"
#include "Application/Utils/char.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "Application/Views/SampleEditorView.h"
#include "BaseClasses/UIActionField.h"
#include "BaseClasses/UIIntVarField.h"
#include "BaseClasses/UITempoField.h"
#include "Services/Audio/Audio.h"
#include "Services/Midi/MidiService.h"
#include "System/System/System.h"
#include "UIFramework/SimpleBaseClasses/EventManager.h"
#include <nanoprintf.h>

static void BootselCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    System *sys = System::GetInstance();
    sys->SystemBootloader();
  }
}

static void MassStorageCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    System *sys = System::GetInstance();
    sys->SystemMassStorage();
  }
}

DeviceView::DeviceView(GUIWindow &w, ViewData *data) : FieldView(w, data) {
  GUIPoint position = GetAnchor();

  auto config = Config::GetInstance();

  Variable *v;

  v = config->FindVariable(Token::VarMidiDevice);
  intVarField_.emplace_back(position, *v, "MIDI device  :%s", 0, 3, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  v = config->FindVariable(Token::VarMidiSync);
  // just hardcode max of 1, as only settings are "off" & "send"
  intVarField_.emplace_back(position, *v, "MIDI sync    :%s", 0, 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  v = config->FindVariable(Token::VarLineOut);
  intVarField_.emplace_back(position, *v, "Line Out Mode:%s", 0, 2, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  v = config->FindVariable(Token::VarMirrorUI);
  intVarField_.emplace_back(position, *v, "mirrorUI     :%s", 0, 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  v = config->FindVariable(Token::VarKeyDelay);
  intVarField_.emplace_back(position, *v, "Key delay/rep:%3d", 250, 750, 1, 100);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  v = config->FindVariable(Token::VarKeyRepeat);
  intVarField_.emplace_back(position + GUIPoint(18, 0), *v, "/:%d", 10, 200, 1, 10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 2;
  v = config->FindVariable(Token::VarPreviewVolume);
  intVarField_.emplace_back(position, *v, "Preview volume    :%2d", 0, 99, 1, 10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  v = config->FindVariable(Token::VarBacklightLevel);
  // MIN brightness is 0xF (15)
  intVarField_.emplace_back(position, *v, "Display brightness:%2.2X", 0xF, 0xFF, 1, 16);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  v = config->FindVariable(Token::VarImportResampler);
  intVarField_.emplace_back(position, *v, "Import resampler  :%s", 0, v->GetListSize() - 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  v = config->FindVariable(Token::VarConfigCommandPicker);
  intVarField_.emplace_back(position, *v, "Command input mode:%s", 0, v->GetListSize() - 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 2;
  actionField_.emplace_back("Theme settings", Token::ActionShowTheme, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.y_ += 2;
  actionField_.emplace_back("Firmware update", Token::ActionBootSelect, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

#ifndef ADV
  position.y_ += 1;
  actionField_.emplace_back(char_symbols_usb_s " USB Storage", Token::ActionMassStorage, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);
#endif
}

DeviceView::~DeviceView() {
}

void DeviceView::ProcessButtonMask(uint16_t mask, bool pressed) {
  if (!pressed) {
    FieldView::ProcessButtonMask(mask, pressed);
    return;
  }

  FieldView::ProcessButtonMask(mask, pressed);

  if (mask & BM_NAV) {
    if (mask & BM_DOWN) {
      Navigate(VT_PROJECT, vtRevealFromBottom);
    } else if (mask & BM_RIGHT) {
      Navigate(VT_HELP, vtRevealFromRight);
    }
  } else if (mask & BM_PLAY) {
    Player *player = Player::GetInstance();
    player->OnStartButton(PM_SONG, viewData_->songX_, false, viewData_->songX_);
  };
}

void DeviceView::DrawView() {
  Clear();

  // Draw title

  DrawTitle("Device");

  // redraw fields

  FieldView::Redraw();
  drawMap();

  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::fg);
  DrawString(9, SCREEN_HEIGHT - 3, VERSION_STRING);
  DrawString(9, SCREEN_HEIGHT - 2, "props-north.com/one");
  DrawString(9, SCREEN_HEIGHT - 1, "props-north.com/discord");

  // Draw Logo

  DrawString(5, SCREEN_HEIGHT - 3, char_logo_1);
  DrawString(5, SCREEN_HEIGHT - 2, char_logo_2);
  DrawString(5, SCREEN_HEIGHT - 1, char_logo_3);
}

void DeviceView::Update(Observable &, I_ObservableData *data) {
  if (!hasFocus_) {
    return;
  }

  uintptr_t token = (uintptr_t)data;

  UIField *focus = GetFocus();
  focus->ClearFocus();
  focus->Draw(w_);
  focus->SetFocus();

  // Handle brightness changes directly
  if (token == Token::VarBacklightLevel) {
    Config *config = Config::GetInstance();
    Variable *v = config->FindVariable(Token::VarBacklightLevel);
    if (v) {
      unsigned char brightness = (unsigned char)v->GetInt();
      System::GetInstance()->SetDisplayBrightness(brightness);
    }
    configDirty_ = true;
  }

  Player *player = Player::GetInstance();
  Config *config = Config::GetInstance();

  switch (token) {
    case Token::VarKeyRepeat:
    case Token::VarKeyDelay:
      {
        int repeat = config->FindVariable(Token::VarKeyRepeat)->GetInt();
        int delay = config->FindVariable(Token::VarKeyDelay)->GetInt();
        EventDispatcher::GetInstance()->SetKeyRepeatAndDelay(repeat, delay);
        EventManager::Instance()->SetKeyRepeatAndDelay(repeat, delay);
        Trace::Log("Settings", "Delay %d, Repeat %d", delay, repeat);
        break;
      }
    case Token::ActionBootSelect:
      ConfirmStopPlayback(Token::ActionBootSelect);
      return;
    case Token::ActionMassStorage:
      ConfirmStopPlayback(Token::ActionMassStorage);
      return;
    case Token::ActionShowTheme:
      Navigate(VT_THEME, vtRevealFromCenter);
      return;
    case Token::VarLineOut:
      {
        Variable *lv = config->FindVariable(Token::VarLineOut);
        if (lv) {
          Audio *audio = Audio::GetInstance();
          if (audio) {
            audio->SetAudioLevel(lv->GetInt());
          }
        }
        if (!config->Save()) {
          Trace::Error("DEVICEVIEW", "Failed to save device config after line out change");
          configDirty_ = true;
        } else {
          Trace::Log("DEVICEVIEW", "Saved device config after line out change");
          configDirty_ = false;
        }
        break;
      }
    case Token::VarMidiDevice:
    case Token::VarMidiSync:
    case Token::VarMirrorUI:
    case Token::VarImportResampler:
      configDirty_ = true;
      break;
    case Token::VarOutputVolume:
      {
        Config *config = Config::GetInstance();
        Variable *v = config->FindVariable(Token::VarOutputVolume);
        if (v) {
          Audio *audio = Audio::GetInstance();
          if (audio) {
            // This unfortunate name may get confused with the actual audio pipeline
            // mixer. It's not, this sets the driver output volume
            audio->SetMixerVolume(v->GetInt());
          }
        }
        configDirty_ = true;
        break;
      }
    default:
      NInvalid;
      break;
  };
  focus->Draw(w_);
  isDirty_ = true;
}

void DeviceView::addSwatchField(Color color, GUIPoint position) {
  position.x_ -= 5;
  swatchField_.emplace_back(position, color);
  fieldList_.insert(fieldList_.end(), &(*swatchField_.rbegin()));
}

void DeviceView::OnFocusLost() {
  if (configDirty_) {
    Config *config = Config::GetInstance();
    if (!config->Save()) {
      Trace::Error("DEVICEVIEW", "Failed to save device config on focus lost");
      return;
    }
    Trace::Log("DEVICEVIEW", "Saved device config on focus lost");
    configDirty_ = false;
  }
}

void DeviceView::ConfirmMassStorage() {
  MessageBox *mb = MessageBox::Create(*this, "USB", "Reboot to USB storage?", MBBF_YES | MBBF_NO);
  DoModal(mb, ModalViewCallback::create<&MassStorageCallback>());
}

void DeviceView::ConfirmReboot() {
  MessageBox *mb = MessageBox::Create(*this, "Reboot", "Reboot and lose changes?", MBBF_YES | MBBF_NO);
  DoModal(mb, ModalViewCallback::create<&BootselCallback>());
}

void DeviceView::ConfirmedStop(Token sender) {
  switch (sender) {
    case Token::ActionBootSelect:
      ConfirmReboot();
      return;
    case Token::ActionMassStorage:
      ConfirmMassStorage();
      return;
  }
}