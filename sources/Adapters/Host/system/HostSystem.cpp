/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "HostSystem.h"
#include "Adapters/Host/audio/HostAudio.h"
#include "Adapters/Host/filesystem/HostFileSystem.h"
#include "Adapters/Host/gui/HostGUIFactory.h"
#include "Adapters/Host/midi/HostMidiService.h"
#include "Adapters/Host/mutex/HostMutex.h"
#include "Adapters/Host/timer/HostTimer.h"
#include "Application/AppWindow.h"
#include "Application/Instruments/SamplePool.h"
#include "Application/Views/ScreenView.h"
#include "HostSamplePool.h"
#include "Services/Audio/Audio.h"
#include "Services/Midi/MidiService.h"
#include "System/Console/Trace.h"
#include "System/FileSystem/FileSystem.h"
#include "UIFramework/Interfaces/I_GUIWindowFactory.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <cstdio>
#include <random>
#include <thread>

static std::chrono::steady_clock::time_point g_boot_time;
static HostMutex g_mutex;

EventManager *HostSystem::eventManager_ = nullptr;

void HostSystem::Boot(int argc, char **argv) {
  g_boot_time = std::chrono::steady_clock::now();

  alignas(HostGUIFactory) static char gui_mem[sizeof(HostGUIFactory)];
  I_GUIWindowFactory::Install(new (gui_mem) HostGUIFactory());

  alignas(HostTimerService) static char timer_mem[sizeof(HostTimerService)];
  TimerService::Install(new (timer_mem) HostTimerService());

  alignas(HostFileSystem) static char fs_mem[sizeof(HostFileSystem)];
  FileSystem::Install(new (fs_mem) HostFileSystem());

  alignas(HostMidiService) static char midi_mem[sizeof(HostMidiService)];
  MidiService::Install(new (midi_mem) HostMidiService());

  alignas(HostSystem) static char system_mem[sizeof(HostSystem)];
  System::Install(new (system_mem) HostSystem());

  AudioSettings audio_hints;
  audio_hints.audioAPI_ = "SDL2";
  audio_hints.audioDevice_ = "default";
  audio_hints.bufferSize_ = 1024;
  audio_hints.preBufferCount_ = 2;

  alignas(HostAudio) static char audio_mem[sizeof(HostAudio)];
  Audio::Install(new (audio_mem) HostAudio(audio_hints));
  Audio::GetInstance()->Init();

  alignas(HostSamplePool) static char pool_mem[sizeof(HostSamplePool)];
  SamplePool::Install(new (pool_mem) HostSamplePool());

  eventManager_ = I_GUIWindowFactory::GetInstance()->GetEventManager();
  if (eventManager_) {
    eventManager_->Init();
  }
}

void HostSystem::Shutdown() {
  if (Audio::GetInstance()) {
    Audio::GetInstance()->Close();
  }
}

int HostSystem::MainLoop() {
  if (eventManager_) {
    return eventManager_->MainLoop();
  }
  return 0;
}

unsigned long HostSystem::GetClock() {
  return Millis();
}

void HostSystem::GetBatteryState(BatteryState &state) {
  state.percentage = 100;
  state.voltage_mv = 5000;
  state.temperature_c = 25;
  state.charging = false;
  state.error = true;
}

void HostSystem::SetDisplayBrightness(unsigned char value) {
}

unsigned int HostSystem::GetMemoryUsage() {
  return 0;
}

void HostSystem::SystemBootloader() {
}

void HostSystem::SystemReboot() {
  AppWindow *window = AppWindow::GetInstance();
  if (window) {
    window->SetDirty();
    window->Clear();
    ((ScreenView *)window->GetCurrentView())->Navigate(VT_BOOT, vtNone);
    window->SetBootLoadTriggered(false);
  }
}

void HostSystem::SystemMassStorage() {
}

void HostSystem::SystemPutChar(int c) {
  putchar(c);
}

uint32_t HostSystem::GetRandomNumber() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<uint32_t> dis;
  return dis(gen);
}

uint32_t HostSystem::Micros() {
  auto now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - g_boot_time);
  return (uint32_t)(duration.count() & 0xFFFFFFFF);
}

uint32_t HostSystem::Millis() {
  auto now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_boot_time);
  return (uint32_t)(duration.count() & 0xFFFFFFFF);
}

void HostSystem::Sleep(uint32_t millis) {
  std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

SysMutex *HostSystem::GetMutex() {
  return &g_mutex;
}
