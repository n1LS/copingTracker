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

#include "ProjectView.h"
#include "Application/Model/Scale.h"
#include "Application/Persistency/PersistencyService.h"
#include "Application/Utils/randomnames.h"
#include "Application/Views/ImportView.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "Application/Views/ModalDialogs/RenderProgressModal.h"
#include "Application/Views/SampleEditorView.h"
#include "BaseClasses/UIActionField.h"
#include "BaseClasses/UIIntVarField.h"
#include "BaseClasses/UIStaticField.h"
#include "BaseClasses/UITempoField.h"
#include "BaseClasses/View.h"
#include "BaseClasses/ViewEvent.h"
#include "Services/Midi/MidiService.h"
#include <nanoprintf.h>

static void CreateNewProjectCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    // first clear out any existing "unnamed" project
    PersistencyService::GetInstance()->PurgeUnnamedProject();

    ViewEvent ve(VET_NEW_PROJECT);
    ((ProjectView &)v).SetChanged();
    ((ProjectView &)v).NotifyObservers(&ve);
  }
}

static void BootselCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    System *sys = System::GetInstance();
    sys->SystemBootloader();
  }
}

static void SaveAsOverwriteCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_CANCEL) {
    return;
  }

  PersistencyService *persist = PersistencyService::GetInstance();
  const char *projName = ((ProjectView &)v).getProjectName().c_str();
  const char *oldProjName = ((ProjectView &)v).getOldProjectName().c_str();

  if (persist->Save(projName, oldProjName, true) != PERSIST_SAVED) {
    Trace::Error("failed to save renamed project %s [old: %s]", projName, oldProjName);
    MessageBox *mb = MessageBox::Create(((ProjectView &)v), "Failed to save project", MBBF_OK | MBBF_CANCEL);
    ((ProjectView &)v).DoModal(mb, ModalViewCallback::create<&SaveAsOverwriteCallback>());
    return;
  }
  if (persist->SaveProjectState(projName) != PERSIST_SAVED) {
    Trace::Error("Failed to save project state");
  } else {
    Trace::Log("PROJECTVIEW-STATIC", "OVERWROTE:%s", projName);
    ((ProjectView &)v).clearSaveAsFlag(); // clear flag after saving
  }
}

static void PurgeCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    ((ProjectView &)v).OnPurge();
  }
}

static void PurgeInstrumentsCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    ((ProjectView &)v).OnPurgeInstruments();
  }
}

static void RenderStopCallback(View &v, ModalView &dialog) {
  // If the user clicked OK, stop the rendering
  if (dialog.GetReturnCode() == MBL_OK) {
    Player *player = Player::GetInstance();
    if (player->IsRunning()) {
      player->Stop();

      // Show cancellation message
      MessageBox *cancelDialog = MessageBox::Create(((ProjectView &)v), "Rendering Stopped", MBBF_OK);
      ((ProjectView &)v).DoModal(cancelDialog);
    }
  }
}

bool ProjectView::canRenderFromFirstSongRow() const {
  if (project_ == nullptr) {
    return false;
  }

  const unsigned char *songRow = project_->song_.rows_[0].chains;
  for (int channel = 0; channel < SONG_CHANNEL_COUNT; channel++) {
    const unsigned char chain = songRow[channel];
    if (chain == EMPTY_SONG_VALUE) {
      continue;
    }

    for (int phrase = 0; phrase < PHRASES_PER_CHAIN; phrase++) {
      const unsigned char phraseId = project_->song_.chain_.steps_[chain][phrase].phrase;
      if (phraseId != EMPTY_SONG_VALUE) {
        return true;
      }
    }
  }

  return false;
}

ProjectView::ProjectView(GUIWindow &w, ViewData *data) : FieldView(w, data) {

  lastClock_ = 0;
  lastTick_ = 0;

  project_ = data->project_;

  GUIPoint position = GetAnchor();

  Variable *v = project_->FindVariable(FourCC::VarTempo);
  tempoField_.emplace_back(FourCC::ActionTempoChanged, position, *v, "Tempo     :%d [%2.2X]  ", MIN_TEMPO, MAX_TEMPO, 1,
                           10);
  fieldList_.insert(fieldList_.end(), &(*tempoField_.rbegin()));
  (*tempoField_.rbegin()).AddObserver(*this);

  v = project_->FindVariable(FourCC::VarMasterVolume);
  position.y_ += 1;
  intVarField_.emplace_back(position, *v, "Master vol:%d%%", 0, 100, 1, 5);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  v = project_->FindVariable(FourCC::VarTranspose);
  position.y_ += 1;
  intVarField_.emplace_back(position, *v, "Transpose :%3.2d", -48, 48, 0x1, 0xC);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  v = project_->FindVariable(FourCC::VarScale);
  // if scale name is not found, set the default chromatic scale
  if (v->GetInt() < 0) {
    v->SetInt(0);
  }
  position.y_ += 1;
  intVarField_.emplace_back(position, *v, "Scale     :%s", 0, numScales - 1, 1, 10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  // Add Scale Root field
  position.y_ += 1;
  v = project_->FindVariable(FourCC::VarScaleRoot);
  intVarField_.emplace_back(position, *v, "Scale root:%s", 0, 11, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position.y_ += 2;
  actionField_.emplace_back("Sample Pool", FourCC::ActionImport, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  actionField_.emplace_back("Remove Unused Samples", FourCC::ActionPurge, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  actionField_.emplace_back("Remove Unused Instruments", FourCC::ActionPurgeInstrument, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.y_ += 2;

  // save existing fields horizontal alignment
  int xalign = position.x_;

  v = project_->FindVariable(FourCC::VarProjectName);
  auto label = etl::make_string_with_capacity<MAX_UITEXTFIELD_LABEL_LENGTH>("Project   :");
  auto defaultName = etl::make_string_with_capacity<MAX_PROJECT_NAME_LENGTH>(UNNAMED_PROJECT_NAME);
  textField_.emplace_back(*v, position, label, FourCC::ActionProjectRename, defaultName);
  nameField_ = &(*textField_.rbegin());

  nameField_->AddObserver(*this);
  fieldList_.insert(fieldList_.end(), nameField_);

  position.y_ += 2;
  actionField_.emplace_back("Browse", FourCC::ActionBrowse, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.x_ += 7;
  actionField_.emplace_back("Save", FourCC::ActionSave, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.x_ += 5;
  actionField_.emplace_back("New", FourCC::ActionNewProject, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.x_ += 4;
  actionField_.emplace_back("Random", FourCC::ActionRandomName, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);
  position.x_ = xalign;

  // Add rendering action fields
  position.y_ += 2;

  // Add a static field as a label for the render actions
  staticField_.emplace_back(position, "Render:");
  fieldList_.insert(fieldList_.end(), &(*staticField_.rbegin()));

  // Position the Mixdown action field to the right of the label
  position.x_ += 8;
  actionField_.emplace_back("Mixdown", FourCC::ActionRenderMixdown, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.x_ += 8;
  actionField_.emplace_back("Stems", FourCC::ActionRenderStems, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);
  position.x_ = xalign;
}

ProjectView::~ProjectView() {
}

void ProjectView::ProcessButtonMask(uint16_t mask, bool pressed) {

  if (!pressed)
    return;

  FieldView::ProcessButtonMask(mask, pressed);

  if (mask & BM_NAV) {
    if (mask & BM_DOWN || mask & BM_UP) {
      if (!CanExit()) {
        return;
      }
    }

    if (mask & BM_DOWN) {
      Navigate(VT_SONG);
    } else if (mask & BM_RIGHT) {
      Navigate(VT_GROOVE);
    } else if (mask & BM_UP) {
      Navigate(VT_DEVICE);
    }
  } else if (mask & BM_PLAY) {
    Player *player = Player::GetInstance();
    player->OnStartButton(PM_SONG, viewData_->songX_, false, viewData_->songX_);
  }
}

void ProjectView::Reset() {
  lastClock_ = 0;
  lastTick_ = 0;
  saveAsFlag_ = false;
  oldProjName_ = getProjectName();
}

void ProjectView::DrawView() {
  Clear();

  // Draw title

  Variable *v = viewData_->project_->FindVariable(FourCC::VarProjectName);
  DrawTitle("Project %s", v->GetString().c_str());

  // Draw fields and map

  FieldView::Redraw();
  drawMap();
}

void ProjectView::Update(Observable &, I_ObservableData *data) {

  if (!hasFocus_) {
    return;
  }

  uintptr_t fourcc = (uintptr_t)data;

  UIField *focus = GetFocus();
  if (fourcc != FourCC::ActionTempoChanged) {
    focus->ClearFocus();
    focus->Draw(w_);
    w_.Flush();
    focus->SetFocus();
  } else {
    focus = &tempoField_[0];
  }
  Player *player = Player::GetInstance();

  switch (fourcc) {
    case FourCC::ActionPurge:
      {
        MessageBox *mb = MessageBox::Create(*this, "Remove unused samples?", MBBF_YES | MBBF_NO);
        DoModal(mb, ModalViewCallback::create<&PurgeCallback>());
        break;
      }
    case FourCC::ActionPurgeInstrument:
      {
        MessageBox *mb = MessageBox::Create(*this, "Remove unused instruments?", MBBF_YES | MBBF_NO);
        DoModal(mb, ModalViewCallback::create<&PurgeInstrumentsCallback>());
        break;
      }
    case FourCC::ActionRandomName:
      {
        char name[10];
        getRandomName(name, 10);
        project_->SetProjectName(name);
        saveAsFlag_ = true;
        break;
      }
    case FourCC::ActionSave:
      {
        PersistencyService *persist = PersistencyService::GetInstance();
        char projName[MAX_PROJECT_NAME_LENGTH + 1];
        project_->GetProjectName(projName);

        if (saveAsFlag_) {
          // first need to check if project with this name already exists
          if (persist->Exists(projName)) {
            Trace::Error("project already exists ask user to confirm overwrite");
            MessageBox *mb = MessageBox::Create(*this, "Overwrite EXISTING project?", MBBF_OK | MBBF_CANCEL);
            DoModal(mb, ModalViewCallback::create<&SaveAsOverwriteCallback>());
            return;
          }
          if (persist->Save(projName, oldProjName_.c_str(), saveAsFlag_) != PERSIST_SAVED) {
            Trace::Error("failed to save project state");
            MessageBox *mb = MessageBox::Create(*this, "Error saving Project", MBBF_OK);
            DoModal(mb);
            return;
          }
          clearSaveAsFlag();
        } else {
          if (persist->Save(projName, oldProjName_.c_str(), saveAsFlag_) != PERSIST_SAVED) {
            Trace::Error("failed to save project state");
            MessageBox *mb = MessageBox::Create(*this, "Error saving Project", MBBF_OK);
            DoModal(mb);
            return;
          }
        }
        // all good so now persist the new project name in project state
        persist->SaveProjectState(projName);
        break;
      }
    case FourCC::ActionProjectRename:
      Trace::Log("PROJECTVIEW", "Project renamed! prev name:%s", nameField_->GetString().c_str());
      saveAsFlag_ = true;
      break;
    case FourCC::ActionBrowse:
      {
        if (CanExit()) {
          Navigate(VT_SELECTPROJECT);
        }
        break;
      }
    case FourCC::ActionNewProject:
      {
        MessageBox *mb =
            MessageBox::Create(*this, "Create a new project and", "   lose all changes?", MBBF_YES | MBBF_NO);
        DoModal(mb, ModalViewCallback::create<&CreateNewProjectCallback>());
        break;
      }
    case FourCC::ActionBootSelect:
      {
        if (!player->IsRunning()) {
          MessageBox *mb = MessageBox::Create(*this, "Reboot and lose changes?", MBBF_YES | MBBF_NO);
          DoModal(mb, ModalViewCallback::create<&BootselCallback>());
        } else {
          MessageBox *mb = MessageBox::Create(*this, "Not while playing", MBBF_OK);
          DoModal(mb);
        }
        break;
      }

    case FourCC::ActionTempoChanged:
      break;
    case FourCC::ActionRenderMixdown:
      if (!player->IsRunning()) {
        if (!canRenderFromFirstSongRow()) {
          MessageBox *mb = MessageBox::Create(*this, "      Render failed", "Song row 00 has no phrases", MBBF_OK);
          DoModal(mb);
          break;
        }
        // Show a dialog with a Stop button during rendering
        RenderProgressModal *renderDialog =
            RenderProgressModal::Create(*this, "Rendering", "", RenderProgressModal::ProgressDisplayMode::SongPercent);
        DoModal(renderDialog, ModalViewCallback::create<&RenderStopCallback>());

        // Start playback in rendering mode with MSM_FILE
        player->Start(PM_SONG, true, MSM_FILE, true);
      }
      break;
    case FourCC::ActionRenderStems:
      if (!player->IsRunning()) {
        if (!canRenderFromFirstSongRow()) {
          MessageBox *mb = MessageBox::Create(*this, "      Render failed", "Song row 00 has no phrases", MBBF_OK);
          DoModal(mb);
          break;
        }
        // Show a dialog with a Stop button during rendering
        RenderProgressModal *renderDialog = RenderProgressModal::Create(
            *this, "Stems Rendering", "", RenderProgressModal::ProgressDisplayMode::SongPercent);
        DoModal(renderDialog, ModalViewCallback::create<&RenderStopCallback>());

        // Start playback in rendering mode with MSM_FILESPLIT
        player->Start(PM_SONG, true, MSM_FILESPLIT, true);
      }
      break;
    case FourCC::ActionImport:
      // Switch to the ImportView **BUT** to show the Project Pool by default
      if (!player->IsRunning()) {
        // First check if the samplelib exists
        bool samplelibExists = FileSystem::GetInstance()->exists(SAMPLES_LIB_DIR);

        if (!samplelibExists) {
          MessageBox *mb = MessageBox::Create(*this, "Can't access the samplelib", MBBF_OK);
          DoModal(mb);
        } else {
          ImportView::SetSourceViewType(VT_PROJECT);
          // Set to show project pool dir in ImportView
          viewData_->isShowingSampleEditorProjectPool = true;

          // Go to import sample
          Navigate(VT_IMPORT);
        }
      } else {
        MessageBox *mb = MessageBox::Create(*this, "Not while playing", MBBF_OK);
        DoModal(mb);
      }
      break;
    default:
      NInvalid;
      break;
  };
  focus->Draw(w_);
  isDirty_ = true;
}

void ProjectView::OnPurge() {
  project_->PurgeSamples();
}

void ProjectView::OnPurgeInstruments() {
  project_->PurgeInstruments();
}

void ProjectView::OnQuit() {
  ViewEvent ve(VET_QUIT_APP);
  SetChanged();
  NotifyObservers(&ve);
}

void ProjectView::OnFocus() {
  // only store current project name for use in a "save as" operation if it's
  // not already been modified by the user pending a save which is indicated
  // by the saveAsFlag_ flag being set
  if (!saveAsFlag_) {
    oldProjName_ = getProjectName();
  }
}

bool ProjectView::CanExit() {
  if (saveAsFlag_) {
    MessageBox *mb = MessageBox::Create(*this, "Save project rename first", MBBF_OK);
    DoModal(mb);
    return false;
  }
  return true;
}
