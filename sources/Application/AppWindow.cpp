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

#include "AppWindow.h"
#include "Application/Commands/ApplicationCommandDispatcher.h"
#include "Application/Commands/EventDispatcher.h"
#include "Application/Instruments/SamplePool.h"
#include "Application/Mixer/MixerService.h"
#include "Application/Model/Mixer.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Application/Persistency/PersistencyService.h"
#include "Application/Player/TablePlayback.h"
#include "Application/Utils/char.h"
#include "Application/Views/ChainView.h"
#include "Application/Views/DeviceView.h"
#include "Application/Views/GrooveView.h"
#include "Application/Views/ImportView.h"
#include "Application/Views/InstrumentImportView.h"
#include "Application/Views/InstrumentView.h"
#include "Application/Views/MixerView.h"
#include "Application/Views/ModalDialogs/FullScreenBox.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "Application/Views/NullView.h"
#include "Application/Views/PhraseView.h"
#include "Application/Views/ProjectView.h"
#include "Application/Views/SampleEditorView.h"
#include "Application/Views/SampleSlicesView.h"
#include "Application/Views/SelectProjectView.h"
#include "Application/Views/SongView.h"
#include "Application/Views/TableView.h"
#include "Application/Views/ThemeImportView.h"
#include "Application/Views/ThemeView.h"
#include "BaseClasses/View.h"
#include "Foundation/Variables/WatchedVariable.h"
#include "Player/Player.h"
#include "Services/Midi/MidiService.h"
#include "System/Console/Trace.h"
#include "System/FileSystem/FileSystem.h"
#include "System/System/System.h"
#include "UIFramework/Interfaces/I_GUIWindowFactory.h"
#include "Views/UIController.h"
#include <nanoprintf.h>
#include <string.h>

const uint16_t AUTOSAVE_INTERVAL_IN_SECONDS = 1 * 60;

#define MINIMUM_ALLOWED_BATTERY_PERCENTAGE 2

#define MIN_BATT_POWEROFF_SEC 15

AppWindow *instance = 0;

unsigned char AppWindow::_charScreen[SCREEN_CHARS];
color_t AppWindow::_screenColor[SCREEN_CHARS];
unsigned char AppWindow::_preScreen[SCREEN_CHARS];
color_t AppWindow::_preScreenColor[SCREEN_CHARS];

GUIColor AppWindow::colorPalette_[NUM_COLORS] = {
    GUIColor(0x00, 0x00, 0x00), // 0: black
    GUIColor(0x80, 0x00, 0x00), // 1: dark red
    GUIColor(0x00, 0x80, 0x00), // 2: dark green
    GUIColor(0x80, 0x80, 0x00), // 3: dark yellow
    GUIColor(0x00, 0x00, 0x80), // 4: dark blue
    GUIColor(0x80, 0x00, 0x80), // 5: dark magenta
    GUIColor(0x00, 0x80, 0x80), // 6: dark cyan
    GUIColor(0x80, 0x80, 0x80), // 7: gray
    GUIColor(0xc6, 0xc6, 0xc6), // 8: light gray
    GUIColor(0xFF, 0x00, 0x00), // 9: red
    GUIColor(0x00, 0xFF, 0x00), // 10: green
    GUIColor(0xFF, 0xFF, 0x00), // 11: yellow
    GUIColor(0x00, 0x00, 0xFF), // 12: blue
    GUIColor(0xFF, 0x00, 0xFF), // 13: magenta
    GUIColor(0x00, 0xFF, 0xFF), // 14: cyan
    GUIColor(0xFF, 0xFF, 0xFF), // 15: white
};

// Initialize the animation frame counter
uint32_t AppWindow::animationFrameCounter_ = 0;

struct AppWindowViews {
  SongView songView;
  ChainView chainView;
  PhraseView phraseView;
  DeviceView deviceView;
  ThemeView themeView;
  ThemeImportView themeImportView;
  ProjectView projectView;
  ImportView importView;
  InstrumentImportView instrumentImportView;
  InstrumentView instrumentView;
  TableView tableView;
  GrooveView grooveView;
  SelectProjectView selectProjectView;
  MixerView mixerView;
  SampleEditorView sampleEditorView;
  SampleSlicesView sampleSlicesView;
  NullView nullView;

  AppWindowViews(GUIWindow &w, ViewData &viewData)
      : songView(w, &viewData), chainView(w, &viewData), phraseView(w, &viewData), deviceView(w, &viewData),
        themeView(w, &viewData), themeImportView(w, &viewData), projectView(w, &viewData), importView(w, &viewData),
        instrumentImportView(w, &viewData), instrumentView(w, &viewData), tableView(w, &viewData),
        grooveView(w, &viewData), selectProjectView(w, &viewData), mixerView(w, &viewData),
        sampleEditorView(w, &viewData), sampleSlicesView(w, &viewData), nullView(w, &viewData) {
  }
};

void AppWindow::defineColor(FourCC colorCode, GUIColor &color, int paletteIndex) {
  Config *config = Config::GetInstance();
  auto rgbVar = config->FindVariable(colorCode);
  if (rgbVar) {
    const int32_t rgbValue = rgbVar->GetInt();
    uint16_t r, g, b;
    r = (rgbValue >> 16) & 0xFF;
    g = (rgbValue >> 8) & 0xFF;
    b = rgbValue & 0xFF;
    // Always preserve the palette index when updating colors
    color = GUIColor(r, g, b, paletteIndex);
  } else {
    // Even if we don't update the RGB values, ensure the palette index is
    // correct
    color.paletteIndex_ = paletteIndex;
  }
}

AppWindow::AppWindow(I_GUIWindowImp &imp, const char *projectName)
    : GUIWindow(imp), project_(projectName), viewData_(&project_), views_(nullptr), _currentView(nullptr) {

  instance = this;

  // Init all members

  _statusLine[0] = 0;

  _currentView = nullptr;
  _closeProject = false;
  _lastA = 0;
  _lastB = 0;
  _mask = 0;
  lowBatteryMessageShown_ = false;
  sdCardMissing_ = false;
  sdCardMessageShown_ = false;

  npf_snprintf(projectName_, sizeof(projectName_), "%s", projectName);

  EventDispatcher *ed = EventDispatcher::GetInstance();
  ed->SetWindow(this);

  Status::Install(this);

  // Init midi services
  MidiService::GetInstance()->Init();

  UpdateColorsFromConfig();

  GUIWindow::Clear(colorPalette_[Theme::View::bg]);

  static AppWindowViews views(*this, viewData_);
  views_ = &views;

  _currentView = &views_->nullView;
  views_->nullView.SetDirty(true);

  views_->songView.AddObserver(*this);
  views_->chainView.AddObserver(*this);
  views_->phraseView.AddObserver(*this);
  views_->deviceView.AddObserver(*this);
  views_->themeView.AddObserver(*this);
  views_->themeImportView.AddObserver(*this);
  views_->projectView.AddObserver(*this);
  views_->importView.AddObserver(*this);
  views_->instrumentImportView.AddObserver(*this);
  views_->instrumentView.AddObserver(*this);
  views_->tableView.AddObserver(*this);
  views_->grooveView.AddObserver(*this);
  views_->selectProjectView.AddObserver(*this);
  views_->mixerView.AddObserver(*this);
  views_->sampleEditorView.AddObserver(*this);
  views_->sampleSlicesView.AddObserver(*this);

  memset(_charScreen, ' ', SCREEN_CHARS);
  memset(_preScreen, ' ', SCREEN_CHARS);
  memset(_screenColor, 0, SCREEN_CHARS);
  memset(_preScreenColor, 0, SCREEN_CHARS);

  Redraw();

  // there is some sort of race that if we call LoadProject() from here directly
  // causes audio init to fail, so instead set this flag which will then cause
  // LoadProject() to be called from within the next time that AnimationUpdate()
  // is called
  loadProject_ = true;
}

AppWindow::~AppWindow() {
  MidiService::GetInstance()->Close();
}

void AppWindow::SetSdCardPresent(bool present) {
  sdCardMissing_ = !present;
  SetDirty();
}

void appwindow_set_sdcard_present(bool present) {
  if (instance) {
    instance->SetSdCardPresent(present);
  }
}

void AppWindow::DrawString(const char *string, const GUIPoint &pos) {
  // Safety check for null string
  if (!string) {
    return;
  }

  // DrawString and DrawChar share the same cache staging logic.
  int x = pos.x_;
  for (const char *current = string; *current; ++current, ++x) {
    GUIPoint charPos(x, pos.y_);
    DrawChar(*current, charPos);
  }
}

void AppWindow::DrawChar(const char c, const GUIPoint &pos) {
  if (pos.y_ < 0 || pos.y_ >= SCREEN_HEIGHT || pos.x_ < 0 || pos.x_ >= SCREEN_WIDTH) {
    return;
  }

  int index = pos.x_ + SCREEN_WIDTH * pos.y_;
  _charScreen[index] = c;

  // Ensure color index is masked to prevent overlap with inversion bit.
  _screenColor[index] = color_;
}

void AppWindow::Clear(bool all) {
  color_t base = (color_t){.fg = Theme::View::fg, .bg = Theme::View::bg};

  memset(_charScreen, ' ', SCREEN_CHARS);
  memset(_screenColor, base.byte, SCREEN_CHARS);

  if (all) {
    memset(_preScreen, '\0', SCREEN_CHARS);
    memset(_preScreenColor, base.byte, SCREEN_CHARS);
  };
}

void AppWindow::ClearTextRect(GUIRect &r) {
  int x = r.Left();
  int y = r.Top();
  int w = r.Width();
  int h = r.Height();

  // Clamp rectangle to screen bounds.
  if (x < 0) {
    w += x; // Reduce width by the amount we're off-screen
    x = 0;
  }
  if (y < 0) {
    h += y; // Reduce height by the amount we're off-screen
    y = 0;
  }
  if (x + w > SCREEN_WIDTH) {
    w = SCREEN_WIDTH - x;
  }
  if (y + h > SCREEN_HEIGHT) {
    h = SCREEN_HEIGHT - y;
  }

  // Only clear if there's a valid region.
  if (w <= 0 || h <= 0) {
    return;
  }

  unsigned char *st = _charScreen + x + (SCREEN_WIDTH * y);
  color_t *pr = _screenColor + x + (SCREEN_WIDTH * y);
  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      *st++ = ' ';
      *pr++ = {.byte = 0};
    }
    st += (SCREEN_WIDTH - w);
    pr += (SCREEN_WIDTH - w);
  }
}

void AppWindow::InvalidateTextCache() {
  // Force the next text flush to resend all cells without changing the current
  // text buffer contents.
  memset(_preScreen, 0xFF, SCREEN_CHARS);
  memset(_preScreenColor, 0xFF, SCREEN_CHARS);
}

//
// Flush current screen to display
//
void AppWindow::Flush() {

  Lock();

  GUIPoint pos;

  // Start with an invalid color to force color setting on first character
  Color currentFG = (Color)-1;
  Color currentBG = (Color)-1;
  pos.x_ = 0;
  pos.y_ = 0;

  int count = 0;

  unsigned char *current = _charScreen;
  unsigned char *previous = _preScreen;
  color_t *currentColor = _screenColor;
  color_t *previousColor = _preScreenColor;

  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      if ((*current != *previous) || (currentColor->byte != previousColor->byte)) {
        // Extract invert flag from properties
        Color fg = (Color)currentColor->fg;
        Color bg = (Color)currentColor->bg;

        // Extract color index from properties and check if it's different from
        // current color
        if (fg != currentFG) {
          currentFG = fg;
          GUIWindow::SetColor(colorPalette_[fg]);
        }

        if (bg != currentBG) {
          currentBG = bg;
          GUIWindow::SetBackgroundColor(colorPalette_[bg]);
        }

        GUIWindow::DrawChar(*current, pos);
        count++;
      }

      current++;
      previous++;
      currentColor++;
      previousColor++;
      pos.x_++;
    }
    pos.y_++;
    pos.x_ = 0;
  }
  GUIWindow::Flush();
  Unlock();
  memcpy(_preScreen, _charScreen, SCREEN_CHARS);
  memcpy(_preScreenColor, _screenColor, SCREEN_CHARS);
}

AppWindow::LoadProjectResult AppWindow::LoadProject(const char *projectName) {

  _closeProject = false;

  PersistencyService *persist = PersistencyService::GetInstance();

  Player *player = Player::GetInstance();
  if (player->IsRunning()) {
    player->Stop();
  }

  TablePlayback::Reset();
  TableHolder::GetInstance()->Reset();
  Mixer::GetInstance()->Clear();

  SamplePool *pool = SamplePool::GetInstance();
  pool->Reset();

  project_.Load(projectName);
  Project *project = &project_;

  if (createProjectOnLoad_) {
    PersistencyResult created = persist->CreateProject();
    if (created != PERSIST_SAVED) {
      Trace::Error("Failed to create new project '%s'", projectName);
      return LoadProjectResult::LOAD_FAILED;
    }
    createProjectOnLoad_ = false;
  }

  // load the projects samples
  pool->Load(projectName);

  bool succeeded = (persist->Load(projectName) == PERSIST_LOADED);
  if (!succeeded) {
    Trace::Error("Failed to load project '%s'", projectName);
    pool->Reset();
    TableHolder::GetInstance()->Reset();
    return LoadProjectResult::LOAD_FAILED;
  };

  // Project

  WatchedVariable::Disable();

  // Register as an observer of the project name variable to get notified of
  // changes
  Variable *projectNameVar = project->FindVariable(FourCC::VarProjectName);
  if (projectNameVar) {
    WatchedVariable *watchedVar = (WatchedVariable *)projectNameVar;
    if (watchedVar) {
      watchedVar->RemoveObserver(*this);
      watchedVar->AddObserver(*this);
      // Store the initial project name
      project->GetProjectName(projectName_);
    }
  }

  project->GetInstrumentBank()->Init();

  WatchedVariable::Enable();

  ApplicationCommandDispatcher::GetInstance()->Init(project);

  // Update view data
  viewData_.Load(project);

  if (views_) {
    views_->songView.Reset();
    views_->chainView.Reset();
    views_->phraseView.Reset();
    views_->grooveView.Reset();
    views_->tableView.Reset();
    views_->projectView.Reset();
    views_->instrumentView.Reset();
    views_->mixerView.Reset();
    views_->importView.Reset();
    views_->instrumentImportView.Reset();
    views_->themeView.Reset();
    views_->themeImportView.Reset();
    views_->selectProjectView.Reset();
    views_->sampleEditorView.Reset();
    views_->sampleSlicesView.Reset();
  }

  bool playerOK = true;
  if (!playerInitialized_) {
    playerOK = player->Init(project, &viewData_);
    player->AddObserver(*this);
    playerInitialized_ = true;
  } else {
    player->BindProject(project, &viewData_);
  }

  // Create the controller
  UIController *controller = UIController::GetInstance();
  controller->Init(project, &viewData_);

  _currentView = &views_->songView;
  _currentView->OnFocus();

  if (!playerOK) {
    MessageBox *mb = MessageBox::Create(views_->songView, "Failed to initialize audio", MBBF_OK);
    views_->songView.DoModal(mb);
  }

  if (_currentView) {
    _currentView->SetDirty(true);
    SetDirty();
  }

  if (persist->SaveProjectState(projectName) != PERSIST_SAVED) {
    Trace::Error("Failed to save project state for '%s'", projectName);
  }
  return LoadProjectResult::LOAD_OK;
}

void AppWindow::CloseProject() {

  _closeProject = false;
  Player *player = Player::GetInstance();
  player->Stop();
  player->RemoveObserver(*this);

  SamplePool *pool = SamplePool::GetInstance();
  pool->Reset();

  TableHolder::GetInstance()->Reset();
  TablePlayback::Reset();

  ApplicationCommandDispatcher::GetInstance()->Close();

  UIController *controller = UIController::GetInstance();
  controller->Reset();

  _currentView = &views_->nullView;
  views_->nullView.SetDirty(true);
}

AppWindow *AppWindow::Create(GUICreateWindowParams &params, const char *projectName) {
  I_GUIWindowImp &imp = I_GUIWindowFactory::GetInstance()->CreateWindowImp(params);
  alignas(AppWindow) static char appWindowMemBuf[sizeof(AppWindow)];
  AppWindow *w = new (appWindowMemBuf) AppWindow(imp, projectName);
  return w;
}

void AppWindow::SetDirty() {
  if (_currentView) {
    _currentView->SetDirty(true);
  }
}

void AppWindow::UpdateColorsFromConfig() {
  // now assign custom colors if they have been set device config
  defineColor(FourCC::VarColor_0_Black, colorPalette_[0], 0);
  defineColor(FourCC::VarColor_1_Maroon, colorPalette_[1], 1);
  defineColor(FourCC::VarColor_2_Green, colorPalette_[2], 2);
  defineColor(FourCC::VarColor_3_Olive, colorPalette_[3], 3);
  defineColor(FourCC::VarColor_4_Blue, colorPalette_[4], 4);
  defineColor(FourCC::VarColor_5_Purple, colorPalette_[5], 5);
  defineColor(FourCC::VarColor_6_Turqoise, colorPalette_[6], 6);
  defineColor(FourCC::VarColor_7_LightyGray, colorPalette_[7], 7);
  defineColor(FourCC::VarColor_8_Gray, colorPalette_[8], 8);
  defineColor(FourCC::VarColor_9_Red, colorPalette_[9], 9);
  defineColor(FourCC::VarColor_A_Lime, colorPalette_[10], 10);
  defineColor(FourCC::VarColor_B_Yellow, colorPalette_[11], 11);
  defineColor(FourCC::VarColor_C_LightBlue, colorPalette_[12], 12);
  defineColor(FourCC::VarColor_D_Magenta, colorPalette_[13], 13);
  defineColor(FourCC::VarColor_E_Cyan, colorPalette_[14], 14);
  defineColor(FourCC::VarColor_F_White, colorPalette_[15], 15);
}

bool AppWindow::onEvent(GUIEvent &event) {

  // We need to tell the app to quit once we're out of the
  // mixer lock, otherwise the windows driver will never return

  _shouldQuit = false;

  uint16_t v = 1 << event.GetValue();

  MixerService *sm = MixerService::GetInstance();
  // TODO(democloid): this causes a deadlock, verify original intent
  //  MixerService *ms = MixerService::GetInstance();
  //  ms->Lock();

  switch (event.GetType()) {

    case ET_PADBUTTONDOWN:

      _mask |= v;
      if (_currentView)
        _currentView->ProcessButton(_mask, true);
      break;

    case ET_PADBUTTONUP:

      _mask &= (0xFFFF - v);
      if (_currentView)
        _currentView->ProcessButton(_mask, false);
      break;

    case ET_SYSQUIT:
      _shouldQuit = true;
      break;

      /*		case ET_KEYDOWN:
              if
         (event.GetValue()==EKT_ESCAPE&&!Player::GetInstance()->IsRunning()) {
         if
         (_currentView!=_listView) {
         CloseProject() ;
         _currentView->SetDirty(true) ;
         } else {
                              System::GetInstance()->PostQuitMessage() ;
                      };
              } ;
                  */

    default:
      break;
  }
  //  ms->Unlock();

  if (_shouldQuit) {
    onQuitApp();
  }
  if (_closeProject) {
    CloseProject();
    SetDirty();
  }

  // View dirty flag will be checked in AnimationUpdate to determine if redraw
  // is needed
  return false;
}

void AppWindow::onUpdate(bool redraw) {
  if (redraw) {
    GUIWindow::Clear(colorPalette_[Theme::View::bg]);
    Clear(true);
    // Mark as dirty to trigger redraw in AnimationUpdate
    SetDirty();
  }
  // No Flush here - AnimationUpdate will handle it
}

void AppWindow::AnimationUpdate() {
  // Increment the animation frame counter
  animationFrameCounter_++;
  char failedProjectName_[MAX_PROJECT_NAME_LENGTH + 1] = {0};

  if (awaitingProjectLoadAck_) {
    if (_mask != 0) {
      FileSystem::GetInstance()->DeleteFile("/.current");
      npf_snprintf(projectName_, sizeof(projectName_), "%s", UNNAMED_PROJECT_NAME);
      loadProject_ = true;
      awaitingProjectLoadAck_ = false;
      Trace::Error("Falling back to untitled after failed load of '%s'", failedProjectName_);
    }
    return;
  }

  if (loadProject_) {
    LoadProjectResult loadResult = LoadProject(projectName_);
    loadProject_ = false;
    if (loadResult == LoadProjectResult::LOAD_FAILED) {
      npf_snprintf(failedProjectName_, sizeof(failedProjectName_), "%s", projectName_);
      Status::SetMultiLine("Invalid Project:\n%s\n  \nPress any key\nto continue...", failedProjectName_);
      Trace::Error("Failed to load project '%s'. Waiting for key press to load untitled", failedProjectName_);
      awaitingProjectLoadAck_ = true;
      return;
    }
  }

  if (lowBatteryState_ && !lowBatteryMessageShown_) {
    if (!_currentView->HasModalView()) {
      FullScreenBox *mb = FullScreenBox::Create(*_currentView, "Low battery!", "Connect charger", 0);
      _currentView->DoModal(mb);
      lowBatteryMessageShown_ = true;
      SetDirty();
    }
  } else if (!lowBatteryState_ && lowBatteryMessageShown_) {
    ModalView *modal = _currentView->GetModalView();
    if (modal) {
      modal->EndModal(0);
      _currentView->DismissModal();
      Trace::Debug("CLose Low Batt dialog");
    }
    lowBatteryMessageShown_ = false;
    SetDirty();
  }

  if (sdCardMissing_ && !sdCardMessageShown_) {
    if (_currentView && !_currentView->HasModalView()) {
      FullScreenBox *mb = FullScreenBox::Create(*_currentView, "SD Card Missing", "Insert SD Card", 0);
      _currentView->DoModal(mb);
      sdCardMessageShown_ = true;
      SetDirty();
    }
  } else if (!sdCardMissing_ && sdCardMessageShown_) {
    ModalView *modal = _currentView ? _currentView->GetModalView() : nullptr;
    if (modal) {
      modal->EndModal(0);
      _currentView->DismissModal();
    }
    sdCardMessageShown_ = false;
    // reinserted SD card means we need to either leave the view or reload the current directory in all views
    if (_currentView) {
      // todo: change this to just back out as little as possible, but for now just jump to the song view
      _currentView = &views_->songView;
      _currentView->OnFocus();
    }
    SetDirty();
  }

  // If we need a full redraw due to state changes from key events
  if (_currentView && _currentView->isDirty()) {
    _currentView->Redraw(); // Draw main content
  }

  // Handle view updates
  if (_currentView) {
    // Always update the main view even if modal is active because things like
    // batt gauge still need redrawing and visibility even with a modal onscreen
    _currentView->AnimationUpdate();
    // Now check if there's an active modal view and
    ModalView *modalView = _currentView->GetModalView();
    if (modalView) {
      // Update the modal view
      modalView->AnimationUpdate();
      // Modal can complete from animation updates (e.g. timed hold confirms).
      _currentView->DismissModal();
    }
  }

  // Always flush after AnimationUpdate to ensure consistent state
  Flush();

  // *attempt* to auto save every AUTOSAVE_INTERVAL_IN_SECONDS
  // will return false if auto save was unsuccessful because eg. the sequencer
  // is running
  // we do this here because for sheer convenience because this
  // this callback is called PICO_CLOCK_HZ times a second and we have easy
  // access in this class to the player, projectname and persistence service
  if ((++lastAutoSave / PICO_CLOCK_HZ) > AUTOSAVE_INTERVAL_IN_SECONDS) {
    if (AutoSave()) {
      lastAutoSave = 0;
    }
  }
}

void AppWindow::LayoutChildren() {};

void AppWindow::Update(Observable &o, I_ObservableData *d) {
  if (d && (uintptr_t)d == (uintptr_t)FourCC::VarProjectName) {
    // Update the stored project name from the project
    Project *project = viewData_.project_;
    if (project) {
      project->GetProjectName(projectName_);
      Trace::Log("APPWINDOW", "Project name retrieved: %s", projectName_);
    } else {
      Trace::Error("APPWINDOW: Project name retrieval failed!");
    }
    return;
  }

  ViewEvent *ve = (ViewEvent *)d;

  switch (ve->GetType()) {

    case VET_SWITCH_VIEW:
      {
        ViewType *vt = (ViewType *)ve->GetData();
        if (_currentView) {
          _currentView->LooseFocus();
        }

        switch (*vt) {
          case VT_SONG:
            _currentView = &views_->songView;
            break;
          case VT_CHAIN:
            _currentView = &views_->chainView;
            break;
          case VT_PHRASE:
            _currentView = &views_->phraseView;
            break;
          case VT_DEVICE:
            _currentView = &views_->deviceView;
            break;
          case VT_PROJECT:
            _currentView = &views_->projectView;
            break;
          case VT_INSTRUMENT:
            _currentView = &views_->instrumentView;
            break;
          case VT_TABLE:
            _currentView = &views_->tableView;
            break;
          case VT_TABLE2:
            _currentView = &views_->tableView;
            break;
          case VT_GROOVE:
            _currentView = &views_->grooveView;
            break;
          case VT_IMPORT:
            _currentView = &views_->importView;
            break;
          case VT_INSTRUMENT_IMPORT:
            _currentView = &views_->instrumentImportView;
            break;
          case VT_SELECTPROJECT:
            _currentView = &views_->selectProjectView;
            break;
          case VT_MIXER:
            _currentView = &views_->mixerView;
            break;
          case VT_THEME:
            _currentView = &views_->themeView;
            break;
          case VT_THEME_IMPORT:
            _currentView = &views_->themeImportView;
            break;
          case VT_SELECTTHEME:
            _currentView = &views_->themeView;
            break;
          case VT_SAMPLE_EDITOR:
            _currentView = &views_->sampleEditorView;
            break;
          case VT_SAMPLE_SLICES:
            _currentView = &views_->sampleSlicesView;
            break;
          default:
            break;
        }
        _currentView->SetFocus(*vt);
        SetDirty();
        GUIWindow::Clear(colorPalette_[Theme::View::bg]);
        Clear(true);
        break;
      }

    case VET_PLAYER_POSITION_UPDATE:
      {
        PlayerEvent *pt = (PlayerEvent *)ve;
        if (_currentView) {
          // Check if the current view has a modal view
          const bool hasModal = _currentView->HasModalView();
          if (hasModal) {
            _currentView->GetModalView()->OnPlayerUpdate(pt->GetType(), pt->GetTickCount());
          } else {
            _currentView->OnPlayerUpdate(pt->GetType(), pt->GetTickCount());
          }
        }
        break;
      }

    case VET_LOAD_PROJECT:
      {
        const char *name = static_cast<const char *>(ve->GetData());
        if (name && name[0] != '\0') {
          npf_snprintf(projectName_, sizeof(projectName_), "%s", name);
          createProjectOnLoad_ = false;
          loadProject_ = true;
        }
        break;
      }
    case VET_NEW_PROJECT:
      {
        npf_snprintf(projectName_, sizeof(projectName_), "%s", UNNAMED_PROJECT_NAME);
        createProjectOnLoad_ = true;
        loadProject_ = true;
        break;
      }
    case VET_QUIT_PROJECT:
      {
        // defer event to after we got out of the view
        _closeProject = true;
        break;
      }
    case VET_QUIT_APP:
      _shouldQuit = true;
      break;
    default: // VET_LIST_SELECT, VET_UPDATE
      break;
  }
}

void AppWindow::onQuitApp() {
  Player *player = Player::GetInstance();
  player->Stop();
  player->RemoveObserver(*this);

  player->Reset();
  System::GetInstance()->PostQuitMessage();
}

void AppWindow::Print(char *line) {
  PrintMultiLine(line);
}

void AppWindow::PrintMultiLine(char *line) {
  Clear();

  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::fg);

  int current_y = 11; // Start near the middle of the screen

  // Handle single line case for Print function
  bool isSingleLine = (line != nullptr && strchr(line, '\n') == nullptr);

  // Use strtok to split the string by newline characters
  char *token = strtok(line, "\n");
  int lineCount = 0;

  while (token != NULL) {
    // Stop if we are about to overwrite the build string line
    if (current_y >= 22) {
      break;
    }

    // For single line, center it at position 12 instead of starting at 11
    if (isSingleLine && lineCount == 0) {
      current_y = 12;
    }

    // Horizontally center the current line of text
    int position = 32; // Assumes a screen width of 32 characters
    position -= strlen(token);
    position /= 2;

    GUIPoint pos(position, current_y);
    DrawString(token, pos);

    // Get the next line
    token = strtok(NULL, "\n");
    current_y++;
    lineCount++;
  }

  // Preserve the build string at the bottom of the screen
  // todo: update and merge with instance from nullview
  char buildString[SCREEN_WIDTH + 1];
  npf_snprintf(buildString, sizeof(buildString), "copingTracker build %s%s_%s", PROJECT_NUMBER, PROJECT_RELEASE,
               BUILD_COUNT);
  GUIPoint pos(0, 22);
  pos.x_ = (32 - strlen(buildString)) / 2;
  DrawString(buildString, pos);
  Flush();
}

void AppWindow::SwapColors() {
  Color temp = color_.fg;
  color_.fg = color_.bg;
  color_.bg = temp;
}

void AppWindow::SetColor(Color color) {
  color_.fg = color;
}

void AppWindow::SetBackgroundColor(Color color) {
  color_.bg = color;
}

bool AppWindow::AutoSave() {
  Player *player = Player::GetInstance();
  if (views_ == nullptr || _currentView == nullptr) {
    return false;
  }
  // only auto save when sequencer is not running and the user is in an autosave-safe view.
  bool autosaveSafeView = _currentView == &views_->songView || _currentView == &views_->chainView ||
                          _currentView == &views_->phraseView || _currentView == &views_->tableView ||
                          _currentView == &views_->grooveView || _currentView == &views_->instrumentView ||
                          _currentView == &views_->deviceView || _currentView == &views_->themeView ||
                          _currentView == &views_->mixerView;

  if (!player->IsRunning() && autosaveSafeView) {
    Trace::Log("APPWINDOW", "AutoSaving Project Data");
    // get persistence service and call autosave
    PersistencyService *ps = PersistencyService::GetInstance();
    auto result = ps->AutoSaveProjectData(projectName_);
    if (result != PERSIST_SAVED) {
      Trace::Error("APPWINDOW", "Failed to auto-save project data");
      // we dont return false here as we dont want to go into a bombardment of
      // auto save attempts and instead just attempt to auto save again after
      // the next interval
    }
    return true;
  }
  return false;
}

// Maps the ColorDefinition used in View classes to a GUIColor that is needed by
// DrawRect which unlike the text grid based drawing code works in terms of
// GUIColor and not definitions
GUIColor AppWindow::GetGUIColor(Color cd) {
  return colorPalette_[cd];
}