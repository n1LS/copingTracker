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

  v = config->FindVariable(FourCC::VarMidiDevice);
  intVarField_.emplace_back(position, *v, "MIDI device  :%s", 0, 3, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  v = config->FindVariable(FourCC::VarMidiSync);
  // just hardcode max of 1, as only settings are "off" & "send"
  intVarField_.emplace_back(position, *v, "MIDI sync    :%s", 0, 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  v = config->FindVariable(FourCC::VarLineOut);
  intVarField_.emplace_back(position, *v, "Line Out Mode:%s", 0, 2, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  v = config->FindVariable(FourCC::VarMirrorUI);
  intVarField_.emplace_back(position, *v, "Remote UI    :%s", 0, 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 2;
  v = config->FindVariable(FourCC::VarBacklightLevel);
  // MIN brightness is 0xF (15)
  intVarField_.emplace_back(position, *v, "Display brightness: %2.2X", 0xF, 0xFF, 1, 16);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  v = config->FindVariable(FourCC::VarImportResampler);
  intVarField_.emplace_back(position, *v, "Import resampler  :%s", 0, v->GetListSize() - 1, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));
  (*intVarField_.rbegin()).AddObserver(*this);

  position.y_ += 2;
  actionField_.emplace_back("Theme settings", FourCC::ActionShowTheme, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.y_ += 2;
  actionField_.emplace_back("Update firmware", FourCC::ActionBootSelect, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

#ifndef ADV
  position.y_ += 2;
  actionField_.emplace_back("USB Storage", FourCC::ActionMassStorage, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);
#endif
}

DeviceView::~DeviceView() {
}

void DeviceView::ProcessButtonMask(uint16_t mask, bool pressed) {

  if (!pressed)
    return;

  FieldView::ProcessButtonMask(mask, pressed);

  if (mask & BM_NAV) {
    if (mask & BM_DOWN) {
      Navigate(VT_PROJECT);
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

  // todo: also merge this with the other 2 instances in nullview and the other one
  char buffer[33];
  npf_snprintf(buffer, sizeof(buffer), "Build %s%s_%s", PROJECT_NUMBER, PROJECT_RELEASE, BUILD_COUNT);
  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::fg);
  DrawString(SCREEN_MAP_WIDTH + 1, SCREEN_HEIGHT - 1, buffer);
}

void DeviceView::Update(Observable &, I_ObservableData *data) {
  if (!hasFocus_) {
    return;
  }

  uintptr_t fourcc = (uintptr_t)data;

  UIField *focus = GetFocus();
  focus->ClearFocus();
  focus->Draw(w_);
  w_.Flush();
  focus->SetFocus();

  // Handle brightness changes directly
  if (fourcc == FourCC::VarBacklightLevel) {
    Config *config = Config::GetInstance();
    Variable *v = config->FindVariable(FourCC::VarBacklightLevel);
    if (v) {
      unsigned char brightness = (unsigned char)v->GetInt();
      System::GetInstance()->SetDisplayBrightness(brightness);
    }
    configDirty_ = true;
  }

  Player *player = Player::GetInstance();

  switch (fourcc) {
    case FourCC::ActionBootSelect:
      {
        if (!player->IsRunning()) {
          MessageBox *mb = MessageBox::Create(*this, "Reboot and lose changes?", MBBF_YES | MBBF_NO);
          DoModal(mb, ModalViewCallback::create<&BootselCallback>());
        } else {
          MessageBox *mb = MessageBox::Create(*this, "Not while playing", MBBF_OK);
          DoModal(mb);
        }
        return;
      }
    case FourCC::ActionMassStorage:
      {
        if (!player->IsRunning()) {
          MessageBox *mb = MessageBox::Create(*this, "Reboot to USB storage?", MBBF_YES | MBBF_NO);
          DoModal(mb, ModalViewCallback::create<&MassStorageCallback>());
        } else {
          MessageBox *mb = MessageBox::Create(*this, "Not while playing", MBBF_OK);
          DoModal(mb);
        }
        return;
      }
    case FourCC::ActionShowTheme:
      {
        Navigate(VT_THEME);
        return;
      }
    case FourCC::VarLineOut:
      {
        Config *config = Config::GetInstance();
        Variable *lv = config->FindVariable(FourCC::VarLineOut);
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
    case FourCC::VarMidiDevice:
    case FourCC::VarMidiSync:
    case FourCC::VarMirrorUI:
    case FourCC::VarImportResampler:
      {
        configDirty_ = true;
        break;
      }
    case FourCC::VarOutputVolume:
      {
        Config *config = Config::GetInstance();
        Variable *v = config->FindVariable(FourCC::VarOutputVolume);
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
