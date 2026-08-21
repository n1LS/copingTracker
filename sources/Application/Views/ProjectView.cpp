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
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "Application/Views/ModalDialogs/RenderProgressModal.h"
#include "Application/Views/SampleEditorView.h"
#include "Application/Views/SampleImportView.h"
#include "Application/Views/ToastView.h"
#include "BaseClasses/UIActionField.h"
#include "BaseClasses/UIIntVarField.h"
#include "BaseClasses/UIStaticField.h"
#include "BaseClasses/UITempoField.h"
#include "BaseClasses/View.h"
#include "BaseClasses/ViewEvent.h"
#include "Services/Midi/MidiService.h"
#include <nanoprintf.h>

#define TOAST_SAVE_SUCCESS()                                                                                           \
  {                                                                                                                    \
    ToastView::GetInstance()->Show("Project saved successfully.", &ttSuccess, ToastDuration::regular);                 \
  }
#define TOAST_SAVE_FAILURE()                                                                                           \
  {                                                                                                                    \
    ToastView::GetInstance()->Show("Failed to save project.", &ttError, ToastDuration::regular);                       \
  }

static void CreateNewProjectCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_YES) {
    // first clear out any existing "unnamed" project
    PersistencyService::GetInstance()->PurgeUnnamedProject();

    ViewEvent ve(VET_NEW_PROJECT);
    ((ProjectView &)v).SetChanged();
    ((ProjectView &)v).NotifyObservers(&ve);
  }
}

static bool Save(const char *projName, const char *oldName, bool saveAs) {
  PersistencyService *persist = PersistencyService::GetInstance();

  if (persist->Save(projName, oldName, true) != PERSIST_SAVED) {
    return false;
  }

  if (persist->SaveProjectState(projName) != PERSIST_SAVED) {
    return false;
  }

  return true;
}

static void SaveAsOverwriteCallback(View &v, ModalView &dialog) {
  if (dialog.GetReturnCode() == MBL_CANCEL) {
    return;
  }

  PersistencyService *persist = PersistencyService::GetInstance();
  const char *projName = ((ProjectView &)v).getProjectName().c_str();
  const char *oldProjName = ((ProjectView &)v).getOldProjectName().c_str();

  if (Save(projName, oldProjName, true)) {
    ((ProjectView &)v).clearSaveAsFlag(); // clear flag after saving
    ((ProjectView &)v).getProject()->SetProjectName(projName);
    TOAST_SAVE_SUCCESS();
  } else {
    TOAST_SAVE_FAILURE();
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
      MessageBox *cancelDialog = MessageBox::Create(((ProjectView &)v), "Render", "Rendering Stopped", MBBF_OK);
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

  Variable *v = project_->FindVariable(Token::VarTempo);
  tempoField_.emplace_back(Token::ActionBPMChanged, position, *v, "Tempo     :" char_symbol_bpm_s " %d", MIN_TEMPO,
                           MAX_TEMPO, 1, 10);
  fieldList_.insert(fieldList_.end(), &(*tempoField_.rbegin()));
  (*tempoField_.rbegin()).AddObserver(*this);

  v = project_->FindVariable(Token::VarMasterVolume);
  position.y_ += 1;
  intVarField_.emplace_back(position, *v, "Master vol:%d%%", 0, 100, 1, 5);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  v = project_->FindVariable(Token::VarTranspose);
  position.y_ += 1;
  intVarField_.emplace_back(position, *v, "Transpose :%3.2d", -48, 48, 0x1, 0xC);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  v = project_->FindVariable(Token::VarScale);
  // if scale name is not found, set the default chromatic scale
  if (v->GetInt() < 0) {
    v->SetInt(0);
  }
  position.y_ += 1;
  intVarField_.emplace_back(position, *v, "Scale     :%s", 0, numScales - 1, 1, 10);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  // Add Scale Root field
  position.y_ += 1;
  v = project_->FindVariable(Token::VarScaleRoot);
  intVarField_.emplace_back(position, *v, "Scale root:%s", 0, 11, 1, 1);
  fieldList_.insert(fieldList_.end(), &(*intVarField_.rbegin()));

  position.y_ += 2;
  actionField_.emplace_back("Sample Pool", Token::ActionImport, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  actionField_.emplace_back("Remove Unused Samples", Token::ActionPurge, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.y_ += 1;
  actionField_.emplace_back("Remove Unused Instruments", Token::ActionPurgeInstrument, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.y_ += 2;

  // save existing fields horizontal alignment
  int xalign = position.x_;

  v = project_->FindVariable(Token::VarProjectName);
  auto label = etl::make_string_with_capacity<MAX_UITEXTFIELD_LABEL_LENGTH>("Project   :");
  auto defaultName = etl::make_string_with_capacity<MAX_PROJECT_NAME_LENGTH>(UNNAMED_PROJECT_NAME);
  textField_.emplace_back(*v, position, label, Token::ActionProjectRename, defaultName);
  nameField_ = &(*textField_.rbegin());

  nameField_->AddObserver(*this);
  fieldList_.insert(fieldList_.end(), nameField_);

  position.y_ += 2;
  actionField_.emplace_back("Browse", Token::ActionBrowse, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.x_ += 8;
  actionField_.emplace_back("Save", Token::ActionSave, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.x_ += 6;
  actionField_.emplace_back("New", Token::ActionNewProject, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.x_ += 5;
  actionField_.emplace_back("Random", Token::ActionRandomName, position);
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
  actionField_.emplace_back("Mixdown", Token::ActionRenderMixdown, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);

  position.x_ += 9;
  actionField_.emplace_back("Stems", Token::ActionRenderStems, position);
  fieldList_.insert(fieldList_.end(), &(*actionField_.rbegin()));
  (*actionField_.rbegin()).AddObserver(*this);
  position.x_ = xalign;
}

ProjectView::~ProjectView() {
}

void ProjectView::ProcessButtonMask(uint16_t mask, bool pressed) {
  if (!pressed) {
    return;
  }

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
  Variable *v = viewData_->project_->FindVariable(Token::VarProjectName);
  DrawTitle("Workspace %s", v->GetString().c_str());

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
  if (fourcc != Token::ActionBPMChanged) {
    focus->ClearFocus();
    focus->Draw(w_);
    focus->SetFocus();
  } else {
    focus = &tempoField_[0];
  }
  Player *player = Player::GetInstance();

  switch (fourcc) {
    case Token::ActionPurge:
      {
        MessageBox *mb = MessageBox::Create(*this, "Purge", "Remove unused samples?", MBBF_YES | MBBF_NO);
        DoModal(mb, ModalViewCallback::create<&PurgeCallback>());
        break;
      }
    case Token::ActionPurgeInstrument:
      {
        MessageBox *mb = MessageBox::Create(*this, "Purge", "Remove unused instruments?", MBBF_YES | MBBF_NO);
        DoModal(mb, ModalViewCallback::create<&PurgeInstrumentsCallback>());
        break;
      }
    case Token::ActionRandomName:
      {
        char name[17];
        getRandomName(name, 17);
        project_->SetProjectName(name);
        saveAsFlag_ = true;
        break;
      }
    case Token::ActionSave:
      {
        PersistencyService *persist = PersistencyService::GetInstance();
        char projName[MAX_PROJECT_NAME_LENGTH + 1];
        project_->GetProjectName(projName);

        if (saveAsFlag_) {
          // first need to check if project with this name already exists
          if (persist->Exists(projName)) {
            Trace::Error("project already exists ask user to confirm overwrite");
            MessageBox *mb = MessageBox::Create(*this, "Save", "Overwrite EXISTING project?", MBBF_OK | MBBF_CANCEL);
            DoModal(mb, ModalViewCallback::create<&SaveAsOverwriteCallback>());
            return;
          }

          if (Save(projName, oldProjName_.c_str(), saveAsFlag_)) {
            clearSaveAsFlag();
            TOAST_SAVE_SUCCESS();
          } else {
            TOAST_SAVE_FAILURE();
          }
        }
        // all good so now persist the new project name in project state
        if (persist->SaveProjectState(projName) == PERSIST_SAVED) {
          TOAST_SAVE_SUCCESS();
        } else {
          TOAST_SAVE_FAILURE();
        }
        break;
      }
    case Token::ActionProjectRename:
      Trace::Log("PROJECTVIEW", "Project renamed! prev name:%s", nameField_->GetString().c_str());
      saveAsFlag_ = true;
      break;
    case Token::ActionBrowse:
      {
        if (CanExit()) {
          Navigate(VT_SELECTPROJECT);
        }
        break;
      }
    case Token::ActionNewProject:
      {
        MessageBox *mb = MessageBox::Create(*this, "New Project", "Create a new project and", "   lose all changes?",
                                            MBBF_YES | MBBF_NO);
        DoModal(mb, ModalViewCallback::create<&CreateNewProjectCallback>());
        break;
      }
    case Token::ActionBPMChanged:
      break;
    case Token::ActionRenderMixdown:
      if (!player->IsRunning()) {
        if (!canRenderFromFirstSongRow()) {
          MessageBox *mb =
              MessageBox::Create(*this, "Render", "      Render failed", "Song row 00 has no phrases", MBBF_OK);
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
    case Token::ActionRenderStems:
      if (!player->IsRunning()) {
        if (!canRenderFromFirstSongRow()) {
          MessageBox *mb =
              MessageBox::Create(*this, "Render", "      Render failed", "Song row 00 has no phrases", MBBF_OK);
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
    case Token::ActionImport:
      // Switch to the SampleImportView **BUT** to show the Project Pool by default
      ConfirmStopPlayback(Token::ActionImport);
      break;
    default:
      NInvalid;
      break;
  };
  focus->Draw(w_);
  isDirty_ = true;
}

void ProjectView::OnPurge() {
  int removed = project_->PurgeSamples();
  char buffer[32];
  npf_snprintf(buffer, sizeof(buffer), "Removed %d sample%s.", removed, removed == 1 ? "" : "s");
  MessageBox *mb = MessageBox::Create(*this, "Done", buffer, MBBF_OK);
}

void ProjectView::OnPurgeInstruments() {
  int removed = project_->PurgeInstruments();
  char buffer[32];
  npf_snprintf(buffer, sizeof(buffer), "Removed %d instrument%s.", removed, removed == 1 ? "" : "s");
  MessageBox *mb = MessageBox::Create(*this, "Done", buffer, MBBF_OK);
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
    MessageBox *mb = MessageBox::Create(*this, "Save", "Save project rename first", MBBF_OK);
    DoModal(mb);
    return false;
  }
  return true;
}

void ProjectView::goToSampleImport() {
  // First check if the samplelib exists
  bool samplelibExists = FileSystem::GetInstance()->exists(SAMPLES_LIB_DIR);

  if (!samplelibExists) {
    MessageBox *mb = MessageBox::Create(*this, "Error", "Can't access the samplelib", MBBF_OK);
    DoModal(mb);
  } else {
    SampleImportView::SetSourceViewType(VT_PROJECT);
    // Set to show project pool dir in SampleImportView
    viewData_->isShowingSampleEditorProjectPool = true;

    // Go to import sample
    Navigate(VT_IMPORT);
  }
}

void ProjectView::ConfirmedStop(Token source) {
  switch (source) {
    case Token::ActionImport:
      goToSampleImport();
      break;
  }
}