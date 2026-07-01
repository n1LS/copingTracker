/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 nILS Podewski
 *
 * This file was part of the picoTracker firmware
 * This file is part of the copingTracker firmware
 */

#include "picoTrackerEventManager.h"
#include "Adapters/picoTracker/filesystem/picoTrackerFileSystem.h"
#include "Adapters/picoTracker/midi/picoTrackerMidiService.h"
#include "Adapters/picoTracker/system/input.h"
#include "Adapters/picoTracker/utils/utils.h"
#include "Application/AppWindow.h"
#include "Application/Application.h"
#include "Application/Model/Config.h"
#include "Services/Midi/MidiService.h"
#include "System/FileSystem/FileSystem.h"
#include "mirrorUI.h"
#include "picoTrackerGUIWindowImp.h"
#include "usb_utils.h"

// Key debounce time in milliseconds. No state changes for this amount of time
// means we accept the new key state.
#define BM_DEBOUNCE_TIME 10

bool picoTrackerEventManager::finished_ = false;
bool picoTrackerEventManager::redrawing_ = false;
uint16_t picoTrackerEventManager::buttonMask_ = 0;

bool picoTrackerEventManager::isRepeating_ = false;
unsigned long picoTrackerEventManager::time_ = 0;
unsigned int picoTrackerEventManager::keyRepeat_ = 25;
unsigned int picoTrackerEventManager::keyDelay_ = 500;
unsigned int picoTrackerEventManager::keyKill_ = 5;

unsigned int picoTrackerEventManager::lastDebounceTime_ = 0;
uint16_t picoTrackerEventManager::debounceMask_ = 0;
uint16_t picoTrackerEventManager::virtualButtonMask_ = 0;

repeating_timer_t picoTrackerEventManager::timer_ = repeating_timer_t();
SerialDebugUI picoTrackerEventManager::serialDebugUI_ = SerialDebugUI();

uint16_t gTime_ = 0;

picoTrackerEventQueue *queue;

#ifdef SERIAL_REPL
#define INPUT_BUFFER_SIZE 80
char inBuffer[INPUT_BUFFER_SIZE];
#endif

// timer callback at a rate of 1kHz (from a 1ms hardware interrupt timer)
bool timerHandler(repeating_timer_t *rt) {
  queue = picoTrackerEventQueue::GetInstance();
  gTime_++;

  // send a clock (PICO_CLOCK) with the current tick value
  if (gTime_ % PICO_CLOCK_INTERVAL == 0) {
    queue->push(picoTrackerEvent(PICO_CLOCK));
  }
  return true;
}

int readFromUSBCDC(char *buf, int len) {
  int rc = PICO_ERROR_NO_DATA;
  if (tud_cdc_available()) {
    int count = (int)tud_cdc_read(buf, (uint32_t)len);
    rc = count ? count : PICO_ERROR_NO_DATA;
  }
  return rc;
}

picoTrackerEventManager::picoTrackerEventManager() {
}

picoTrackerEventManager::~picoTrackerEventManager() {
}

bool picoTrackerEventManager::Init() {
  EventManager::Init();

  // setup a repeating timer for 1ms ticks
  add_repeating_timer_ms(1, timerHandler, NULL, &timer_);
  return true;
}

int picoTrackerEventManager::MainLoop() {
  queue = picoTrackerEventQueue::GetInstance();
  int loops = 0;
  int events = 0;

  MidiService *midiService = MidiService::GetInstance();
  while (!finished_) {
    loops++;

    // process usb interrupts, should this be done somewhere else??
    handleUSBInterrupts();

    // Poll mirrorUI for incoming input commands from USB CDC
    mirrorUI_processCDCInput();

    // Poll MIDI service to process any pending MIDI messages
    if (midiService) {
      picoTrackerMidiService *ptMidiService = (picoTrackerMidiService *)midiService;
      if (ptMidiService) {
        ptMidiService->poll();
      }
    }

    // Poll SD card presence once per second (1.024 seconds...)
    if ((gTime_ & 0x3ff) == 0) {
      picoTrackerFileSystem *fs = static_cast<picoTrackerFileSystem *>(FileSystem::GetInstance());
      bool present = fs && fs->isCardPresent();
      appwindow_set_sdcard_present(present);
    }

    ProcessInputEvent();
    if (!queue->empty()) {
      picoTrackerEvent event(picoTrackerEventType::LAST);
      queue->pop_into(event);
      events++;
      redrawing_ = true;
      picoTrackerGUIWindowImp::ProcessEvent(event);
      redrawing_ = false;
    }
  }
  // TODO: HW Shutdown
  return 0;
}

void picoTrackerEventManager::SetVirtualButtonMask(uint16_t buttonMask, bool pressed) {
  if (pressed) {
    virtualButtonMask_ |= buttonMask;
  } else {
    virtualButtonMask_ &= ~buttonMask;
  }
}

void picoTrackerEventManager::ProcessInputEvent() {
  uint16_t newMask, sendMask;

  if (redrawing_)
    return;
  bool gotEvent = false;

  // Get current mask (physical buttons + virtual buttons for instance via mirrorUI)
  newMask = scanKeys() | virtualButtonMask_;

  unsigned long now = gTime_;

  if (newMask != debounceMask_) {
    // Key state changed. We begin or continue debouncing.
    debounceMask_ = newMask;
    lastDebounceTime_ = now;
    return;
  } else {
    // Keys have not changed since the last scan. But we cannot
    // continue unless they have not changed for at least BM_DEBOUNCE_TIME ms
    unsigned long settleTime = now - lastDebounceTime_;
    if (settleTime < BM_DEBOUNCE_TIME) {
      return;
    }
  }

  // compute mask to send
  sendMask = (newMask ^ buttonMask_) | (newMask & (BM_LEFT | BM_RIGHT | BM_UP | BM_DOWN));

  // see if we're repeating
  if (newMask == buttonMask_) {
    if ((isRepeating_) && ((now - time_) > keyRepeat_)) {
      gotEvent = (sendMask != 0);
    }
    if ((!isRepeating_) && ((now - time_) > keyDelay_)) {
      gotEvent = (sendMask != 0);
      if (gotEvent)
        isRepeating_ = true;
    }
  } else {
    if ((now - time_) > keyKill_) {
      gotEvent = (sendMask != 0);
      if (gotEvent)
        isRepeating_ = false;
    }
  }
  if (gotEvent) {
    time_ = gTime_; // Get time here so delay is independant of processing speed
    picoTrackerGUIWindowImp::ProcessButtonChange(sendMask, newMask);
    buttonMask_ = newMask;
  }

}
