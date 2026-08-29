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

#include "Adapters/copingTracker/system/picoTrackerProjectLoader.h"

#include "Application/Commands/ApplicationCommandDispatcher.h"
#include "Application/Commands/EventDispatcher.h"
#include "Application/Instruments/InstrumentBank.h"
#include "Application/Instruments/SamplePool.h"
#include "Application/Mixer/MixerService.h"
#include "Application/Model/Mixer.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Application/Persistency/PersistencyService.h"
#include "Application/Player/TablePlayback.h"
#include "Application/Utility/ProjectLoader.h"
#include "Application/Utils/char.h"
#include "Application/Views/Views.h"
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

unsigned char AppWindow::_screenChar[SCREEN_CHARS];
color_t AppWindow::_screenColor[SCREEN_CHARS];

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
  HelpView helpView;
  ThemeView themeView;
  ThemeImportView themeImportView;
  ProjectView projectView;
  SampleImportView importView;
  InstrumentImportView instrumentImportView;
  InstrumentView instrumentView;
  TableView tableView;
  GrooveView grooveView;
  SelectProjectView selectProjectView;
  MixerView mixerView;
  SampleEditorView sampleEditorView;
  SampleSlicesView sampleSlicesView;
  BootView bootView;

  void Reset() {
    bootView.Reset();
    songView.Reset();
    chainView.Reset();
    phraseView.Reset();
    grooveView.Reset();
    tableView.Reset();
    projectView.Reset();
    instrumentView.Reset();
    mixerView.Reset();
    importView.Reset();
    instrumentImportView.Reset();
    themeView.Reset();
    themeImportView.Reset();
    selectProjectView.Reset();
    sampleEditorView.Reset();
    sampleSlicesView.Reset();
  };

  void AddObservers(I_Observer &window) {
    songView.AddObserver(window);
    chainView.AddObserver(window);
    phraseView.AddObserver(window);
    deviceView.AddObserver(window);
    helpView.AddObserver(window);
    themeView.AddObserver(window);
    themeImportView.AddObserver(window);
    projectView.AddObserver(window);
    importView.AddObserver(window);
    instrumentImportView.AddObserver(window);
    instrumentView.AddObserver(window);
    tableView.AddObserver(window);
    grooveView.AddObserver(window);
    selectProjectView.AddObserver(window);
    mixerView.AddObserver(window);
    sampleEditorView.AddObserver(window);
    sampleSlicesView.AddObserver(window);
    bootView.AddObserver(window);
  };

  AppWindowViews(GUIWindow &w, ViewData &viewData)
      : songView(w, &viewData), chainView(w, &viewData), phraseView(w, &viewData), deviceView(w, &viewData),
        helpView(w, &viewData), themeView(w, &viewData), themeImportView(w, &viewData), projectView(w, &viewData),
        importView(w, &viewData), instrumentImportView(w, &viewData), instrumentView(w, &viewData),
        tableView(w, &viewData), grooveView(w, &viewData), selectProjectView(w, &viewData), mixerView(w, &viewData),
        sampleEditorView(w, &viewData), sampleSlicesView(w, &viewData), bootView(w, &viewData) {
  }
};

AppWindow *AppWindow::GetInstance() {
  return instance;
}

void AppWindow::defineColor(Token colorCode, GUIColor &color, int paletteIndex) {
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
    : GUIWindow(imp), project_(projectName), viewData_(&project_), views_(nullptr), currentView_(nullptr),
      projectLoader_(*this, project_) {

  instance = this;

  // Init all members

  currentView_ = nullptr;
  mask_ = 0;
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

  GUIWindow::Clear();

  static AppWindowViews views(*this, viewData_);
  views_ = &views;

  currentView_ = &views_->bootView;
  views_->bootView.SetDirty(true);

  views_->AddObservers(*this);

  memset(_screenChar, ' ', SCREEN_CHARS);
  memset(_screenColor, 0, SCREEN_CHARS);

  ToastView::Init(*this, &viewData_);

  Redraw();

  // there is some sort of race that if we call LoadProject() from here directly
  // causes audio init to fail, so instead set this flag which will then cause
  // LoadProject() to be called from within the next time that AnimationUpdate()
  // is called
  // loadProject_ = true;
}

// Static callback wrapper for SamplePool to notify InstrumentBank
static void OnSampleRemovedFromPool(int removedIndex) {
  AppWindow *window = instance;
  if (window) {
    InstrumentBank *bank = window->GetProject().GetInstrumentBank();
    if (bank) {
      bank->OnSampleRemoved(removedIndex);
    }
  }
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

void AppWindow::DrawString(int x, int y, const char *string) {
  if (!string) {
    return;
  }

  for (const char *current = string; *current; ++current, ++x) {
    DrawChar(x, y, *current);
  }
}

void AppWindow::DrawChar(int x, int y, const char c, bool transparent) {
  if (y < 0 || y >= SCREEN_HEIGHT || x < 0 || x >= SCREEN_WIDTH) {
    return;
  }

  int index = x + SCREEN_WIDTH * y;
  _screenChar[index] = c;

  if (transparent) {
    _screenColor[index].fg = color_.fg;
  } else {
    _screenColor[index] = color_;
  }
}

void AppWindow::Clear() {
  color_t base = (color_t){.fg = Theme::View::fg, .bg = Theme::View::bg};

  memset(_screenChar, ' ', SCREEN_CHARS);
  memset(_screenColor, base.byte, SCREEN_CHARS);
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

  unsigned char *st = _screenChar + x + (SCREEN_WIDTH * y);
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

#define GUI(f, c, p) { GUIWindow::SetColor(f->fg); GUIWindow::SetBackgroundColor(f->bg); GUIWindow::DrawChar(p.x_, p.y_, *c); }

void AppWindow::FlushTransition() {
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      unsigned char *current = _screenChar + y * SCREEN_WIDTH + x;
      color_t *currentColor = _screenColor + y * SCREEN_WIDTH + x;
      GUIPoint pos = {x, y};
      bool draw = ((x & 1) == 0 && (y & 1) == 0) || (transitionFrame_ >= 1 && (x & 1) == 1 && (y & 1) == 1);
      if (draw) {
        GUI(currentColor, current, pos);
      }
    }
  }
}

//
// Flush current screen to display
//
void AppWindow::Flush() {
  // draw the ToastView, it handles its own visibility
  ToastView::GetInstance()->Draw(*this);

  Lock();

  // Start with an invalid color to force color setting on first character
  Color currentFG = (Color)-1;
  Color currentBG = (Color)-1;
  GUIPoint pos(0, 0);

  int count = 0;

  unsigned char *current = _screenChar;
  unsigned char *previous = _preScreen;
  color_t *currentColor = _screenColor;

  if (transitionType_ != vtNone) {
    FlushTransition();
  } else {
    // regular drawing
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      pos = {0, y};
      for (int x = 0; x < SCREEN_WIDTH; x++, current++, currentColor++, pos.x_++) {
        // Extract invert flag from properties
        Color fg = (Color)currentColor->fg;
        Color bg = (Color)currentColor->bg;

        // Extract color index from properties and check if it's different from
        // current color
        if (fg != currentFG) {
          currentFG = fg;
          GUIWindow::SetColor(fg);
        }

        if (bg != currentBG) {
          currentBG = bg;
          GUIWindow::SetBackgroundColor(bg);
        }

        GUIWindow::DrawChar(pos.x_, pos.y_, *current);
        count++;
      }
    }
  }

  GUIWindow::SetFocusRect(currentView_->GetFocusRect());

  GUIWindow::Flush();

  Unlock();

  // skip flipping during transitions
  if (transitionType_ != vtNone) {
    if (transitionFrame_ < 1) {
      transitionFrame_++;
    } else {
      transitionType_ = vtNone;
    }
    return;
  }
}

// ============================================================================
// ProjectLoaderProtocol implementation
// ============================================================================

void AppWindow::onLoadPhaseAComplete() {
  // Phase A completed — the project is loaded into memory and samples are
  // being loaded asynchronously. BootView is already showing.
}

void AppWindow::onLoadProgress(uint32_t index, uint32_t total, const char *message) {
  // Progress is polled from BootView via GetProjectLoadProgress()
  (void)index;
  (void)total;
  (void)message;
}

void AppWindow::onLoadPhaseCComplete(bool success, const char *projectName) {
  // Phase C: view-level completion work after sample loading
  if (!success) {
    Trace::Error("Failed to load project '%s'. Waiting for key press to load untitled", projectName);
    npf_snprintf(projectName_, sizeof(projectName_), "%s", projectName);
    awaitingProjectLoadAck_ = true;
    View &errorView = views_->songView;

    char buffer[32];
    npf_snprintf(buffer, sizeof(buffer), "\"%28s\"", projectName);
    MessageBox *mb = MessageBox::Create(errorView, "Project load failed", "Invalid Project:", buffer, MBBF_OK);
    errorView.DoModal(mb);
    return;
  }

  PersistencyService *persist = PersistencyService::GetInstance();
  Player *player = Player::GetInstance();
  Project *project = &project_;

  // Register callback to update instruments when samples are removed
  SamplePool::SetSampleRemovedCallback(&OnSampleRemovedFromPool);

  WatchedVariable::Disable();

  // Register as an observer of the project name variable
  Variable *projectNameVar = project->FindVariable(Token::VarProjectName);
  if (projectNameVar) {
    WatchedVariable *watchedVar = (WatchedVariable *)projectNameVar;
    if (watchedVar) {
      watchedVar->RemoveObserver(*this);
      watchedVar->AddObserver(*this);
      project->GetProjectName(projectName_);
    }
  }

  WatchedVariable::Enable();

  // Update view data
  viewData_.Load(project);

  if (views_) {
    views_->Reset();
  }

  bool playerOK = true;
  if (!playerInitialized_) {
    playerOK = player->Init(project, &viewData_);
    player->AddObserver(*this);
    playerInitialized_ = true;
  } else {
    player->BindProject(project, &viewData_);
  }

  UIController *controller = UIController::GetInstance();
  controller->Init(project, &viewData_);

  currentView_ = &views_->songView;
  currentView_->OnFocus();

  if (!playerOK) {
    MessageBox *mb = MessageBox::Create(views_->songView, "Audio", "Failed to initialize audio", MBBF_OK);
    views_->songView.DoModal(mb);
  }

  if (currentView_) {
    currentView_->SetDirty(true);
    SetDirty();
  }
}

LoadProjectResult AppWindow::LoadProject(const char *projectName) {
  if (projectLoader_.LoadProject(projectName, createProjectOnLoad_)) {
    createProjectOnLoad_ = false;
    return LoadProjectResult::LOAD_OK;
  }
  return LoadProjectResult::LOAD_FAILED;
}

AppWindow *AppWindow::Create(GUICreateWindowParams &params, const char *projectName) {
  I_GUIWindowImp &imp = I_GUIWindowFactory::GetInstance()->CreateWindowImp(params);
  alignas(AppWindow) static char appWindowMemBuf[sizeof(AppWindow)];
  AppWindow *w = new (appWindowMemBuf) AppWindow(imp, projectName);
  return w;
}

void AppWindow::SetDirty() {
  if (currentView_) {
    currentView_->SetDirty(true);
  }
}

bool AppWindow::IsProjectLoadInProgress() const {
  return projectLoader_.IsLoadInProgress();
}

void AppWindow::GetProjectLoadProgress(uint32_t *index, uint32_t *total, char *msgBuf, size_t bufSize) const {
  picoTrackerProjectLoader::GetProgress(index, total, msgBuf, bufSize);
}

void AppWindow::UpdateColorsFromConfig() {
  // now assign custom colors if they have been set device config
  defineColor(Token::VarColor_0, colorPalette_[0], 0);
  defineColor(Token::VarColor_1, colorPalette_[1], 1);
  defineColor(Token::VarColor_2, colorPalette_[2], 2);
  defineColor(Token::VarColor_3, colorPalette_[3], 3);
  defineColor(Token::VarColor_4, colorPalette_[4], 4);
  defineColor(Token::VarColor_5, colorPalette_[5], 5);
  defineColor(Token::VarColor_6, colorPalette_[6], 6);
  defineColor(Token::VarColor_7, colorPalette_[7], 7);
  defineColor(Token::VarColor_8, colorPalette_[8], 8);
  defineColor(Token::VarColor_9, colorPalette_[9], 9);
  defineColor(Token::VarColor_A, colorPalette_[10], 10);
  defineColor(Token::VarColor_B, colorPalette_[11], 11);
  defineColor(Token::VarColor_C, colorPalette_[12], 12);
  defineColor(Token::VarColor_D, colorPalette_[13], 13);
  defineColor(Token::VarColor_E, colorPalette_[14], 14);
  defineColor(Token::VarColor_F, colorPalette_[15], 15);

  GetImpWindow()->SetPalette(colorPalette_, NUM_COLORS);
}

bool AppWindow::onEvent(GUIEvent &event) {

  // We need to tell the app to quit once we're out of the
  // mixer lock, otherwise the windows driver will never return

  shouldQuit_ = false;

  uint16_t v = 1 << event.GetValue();

  MixerService *sm = MixerService::GetInstance();

  switch (event.GetType()) {
    case ET_PADBUTTONDOWN:
      mask_ |= v;
      if (currentView_)
        currentView_->ProcessButton(mask_, true);
      break;

    case ET_PADBUTTONUP:
      mask_ &= ~v;
      if (currentView_)
        currentView_->ProcessButton(mask_, false);
      break;

    default:
      break;
  }

  // View dirty flag will be checked in AnimationUpdate to determine if redraw
  // is needed
  return false;
}

void AppWindow::onUpdate(bool redraw) {
  if (redraw) {
    Clear();
    // Mark as dirty to trigger redraw in AnimationUpdate
    SetDirty();
  }
  // No Flush here - AnimationUpdate will handle it
}

void AppWindow::DelayedProjectLoad() {
  // This method is re-entered on every AnimationUpdate once a load's samples
  // have finished loading, solely so we can call FinalizeLoad() (Phase C) at
  // that point. It is important therefore that we only *start* a load when
  // none is in flight AND no completed load is already awaiting finalization;
  // otherwise we would (re)start a fresh load and orphan the pending one.
  if (!projectLoader_.IsLoadInProgress() && !projectLoader_.IsSampleLoadDone()) {
    if (projectLoader_.LoadProject(projectName_, createProjectOnLoad_)) {
      createProjectOnLoad_ = false;
      bootLoadTriggered_ = true;
      Trace::Log("APPWINDOW", "Auto-triggering boot load of project: %s", projectName_);
    }
  }

  // When sample loading is done, finalize the load (Phase C) and transition
  // to the main view. If we are still on BootView, wait for its animation to
  // complete first so the user sees the full boot animation.
  if (projectLoader_.IsSampleLoadDone()) {
    bool canFinalize = true;
    if (currentView_ == &views_->bootView) {
      canFinalize = views_->bootView.IsAnimationDone();
    }
    if (canFinalize) {
      projectLoader_.FinalizeLoad();
    }
  }
}

void AppWindow::AnimationUpdate() {
  // Increment the animation frame counter
  animationFrameCounter_++;
  char failedProjectName_[MAX_PROJECT_NAME_LENGTH + 1] = {0};

  if (awaitingProjectLoadAck_) {
    if (mask_ != 0) {
      FileSystem::GetInstance()->DeleteFile("/.current");
      createProjectOnLoad_ = false;
      projectLoader_.SetProjectName(UNNAMED_PROJECT_NAME);
      npf_snprintf(projectName_, sizeof(projectName_), "%s", UNNAMED_PROJECT_NAME);
      projectLoader_.LoadProject(UNNAMED_PROJECT_NAME, true);
      awaitingProjectLoadAck_ = false;
      Trace::Error("Falling back to untitled after failed load of '%s'", failedProjectName_);
    }
    return;
  }

  // Auto-trigger load on boot if BootView is showing and load hasn't started
  // yet, or re-enter to finalize a load once its samples have finished loading
  // (Phase C runs only after this, so the project data actually gets loaded).
  if ((!bootLoadTriggered_ && currentView_ == &views_->bootView) || projectLoader_.IsSampleLoadDone()) {
    DelayedProjectLoad();
  }

  // Drive the async load progress
  projectLoader_.Update();

  // Check for ToastView animation updates (needs to run frequently for smooth
  // animation)
  ToastView::GetInstance()->UpdateTimer();

  if (lowBatteryState_ && !lowBatteryMessageShown_) {
    if (!currentView_->HasModalView()) {
      FullScreenBox *mb = FullScreenBox::Create(*currentView_, "Battery", "Low battery!", "Connect charger", 0);
      currentView_->DoModal(mb);
      lowBatteryMessageShown_ = true;
      SetDirty();
    }
  } else if (!lowBatteryState_ && lowBatteryMessageShown_) {
    ModalView *modal = currentView_->GetModalView();
    if (modal) {
      modal->EndModal(0);
      currentView_->DismissModal();
      Trace::Debug("Close Low Batt dialog");
    }
    lowBatteryMessageShown_ = false;
    SetDirty();
  }

  if (sdCardMissing_ && !sdCardMessageShown_) {
    if (currentView_ && !currentView_->HasModalView()) {
      FullScreenBox *mb = FullScreenBox::Create(*currentView_, "SD Card", "SD Card Missing", "Insert SD Card", 0);
      currentView_->DoModal(mb);
      sdCardMessageShown_ = true;
      SetDirty();
    }
  } else if (!sdCardMissing_ && sdCardMessageShown_) {
    ModalView *modal = currentView_ ? currentView_->GetModalView() : nullptr;
    if (modal) {
      modal->EndModal(0);
      currentView_->DismissModal();
    }
    sdCardMessageShown_ = false;
    // reinserted SD card means we need to either leave the view or reload the current directory in all views
    if (currentView_) {
      // todo: change this to just back out as little as possible, but for now just jump to the song view
      currentView_ = &views_->songView;
      currentView_->OnFocus();
    }
    SetDirty();
  }

  // If we need a full redraw due to state changes from key events
  if (currentView_ && currentView_->isDirty()) {
    currentView_->Redraw(); // Draw main content
  }

  // Handle view updates
  if (currentView_) {
    // Always update the main view even if modal is active because things like
    // batt gauge still need redrawing and visibility even with a modal onscreen
    currentView_->AnimationUpdate();
    // Now check if there's an active modal view and
    ModalView *modalView = currentView_->GetModalView();
    if (modalView) {
      // Update the modal view
      modalView->AnimationUpdate();
      // Modal can complete from animation updates (e.g. timed hold confirms).
      currentView_->DismissModal();
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

void AppWindow::SetTransition(ViewTransition type) {
  transitionType_ = type;
  transitionFrame_ = 0;
}

void AppWindow::Update(Observable &o, I_ObservableData *d) {
  if (d && (uintptr_t)d == (uintptr_t)Token::VarProjectName) {
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

    case vetSwitchView:
      {
        ViewEventData *ved = (ViewEventData *)ve->GetData();

        if (_currentView) {
          _currentView->LoseFocus();
        }

        switch (ved->type) {
          case VT_SONG:
            currentView_ = &views_->songView;
            break;
          case VT_CHAIN:
            currentView_ = &views_->chainView;
            break;
          case VT_PHRASE:
            currentView_ = &views_->phraseView;
            break;
          case VT_DEVICE:
            currentView_ = &views_->deviceView;
            break;
          case VT_HELP:
            currentView_ = &views_->helpView;
            break;
          case VT_PROJECT:
            currentView_ = &views_->projectView;
            break;
          case VT_INSTRUMENT:
            currentView_ = &views_->instrumentView;
            break;
          case VT_TABLE:
            currentView_ = &views_->tableView;
            break;
          case VT_TABLE2:
            currentView_ = &views_->tableView;
            break;
          case VT_GROOVE:
            currentView_ = &views_->grooveView;
            break;
          case VT_IMPORT:
            currentView_ = &views_->importView;
            break;
          case VT_INSTRUMENT_IMPORT:
            currentView_ = &views_->instrumentImportView;
            break;
          case VT_SELECTPROJECT:
            currentView_ = &views_->selectProjectView;
            break;
          case VT_MIXER:
            currentView_ = &views_->mixerView;
            break;
          case VT_THEME:
            currentView_ = &views_->themeView;
            break;
          case VT_THEME_IMPORT:
            currentView_ = &views_->themeImportView;
            break;
          case VT_SELECTTHEME:
            currentView_ = &views_->themeView;
            break;
          case VT_SAMPLE_EDITOR:
            currentView_ = &views_->sampleEditorView;
            break;
          case VT_SAMPLE_SLICES:
            currentView_ = &views_->sampleSlicesView;
            break;
          default:
            break;
        }
        _currentView->SetFocus(ved->type);
        SetDirty();
        Clear();
        SetTransition(ved->transition);
        break;
      }

    case vetPlayerPositionUpdate:
      {
        PlayerEvent *pt = (PlayerEvent *)ve;
        if (currentView_) {
          // Check if the current view has a modal view
          const bool hasModal = currentView_->HasModalView();
          if (hasModal) {
            currentView_->GetModalView()->OnPlayerUpdate(pt->GetType(), pt->GetTickCount());
          } else {
            currentView_->OnPlayerUpdate(pt->GetType(), pt->GetTickCount());
          }
        }
        break;
      }

    case vetLoadProject:
      {
        if (!projectLoader_.IsLoadInProgress()) {
          const char *name = static_cast<const char *>(ve->GetData());
          npf_snprintf(projectName_, sizeof(projectName_), "%s", name);
          if (currentView_ != &views_->bootView) {
            currentView_ = &views_->bootView;
            currentView_->OnFocus();
            bootLoadTriggered_ = false;
            SetDirty();
            Clear();
          } else {
            bootLoadTriggered_ = false;
          }
        }
        break;
      }
    case vetNewProject:
      {
        if (!projectLoader_.IsLoadInProgress()) {
          npf_snprintf(projectName_, sizeof(projectName_), "%s", UNNAMED_PROJECT_NAME);
          createProjectOnLoad_ = true;
          projectLoader_.LoadProject(UNNAMED_PROJECT_NAME, true);
        }
        break;
      }
    default: // vetListSelect, vetUpdate
      break;
  }
}

void AppWindow::Print(char *line) {
  SetBackgroundColor(Theme::View::bg);
  SetColor(Theme::View::fg);

  int lineCount = 1;

  char *s = line;
  while (*s) {
    lineCount += (*s == '\n');
    s++;
  }

  // Start near the bottom of the screen
  int current_y = 21 - lineCount;
  if (current_y < 14) {
    current_y = 14;
  }

  // Use strtok to split the string by newline characters
  char *token = strtok(line, "\n");

  char emptyLine[SCREEN_WIDTH + 1];
  memset(emptyLine, ' ', sizeof(emptyLine) - 1);
  emptyLine[SCREEN_WIDTH] = 0;

  while (token != NULL) {
    // Stop if we are about to overwrite the build string line
    if (current_y > 22) {
      break;
    }

    // Horizontally center the current line of text
    int position = 32; // Assumes a screen width of 32 characters
    position -= strlen(token);
    position /= 2;

    DrawString(0, current_y, emptyLine);
    DrawString(position, current_y, token);

    // Get the next line
    token = strtok(NULL, "\n");
    current_y++;
  }
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
  if (!views_ || !currentView_) {
    return false;
  }

  // only auto save when sequencer is not running and the user is in an autosave-safe view.
  bool dontAutoSave = currentView_ == &views_->bootView;

  if (!player->IsRunning() && !dontAutoSave) {
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
