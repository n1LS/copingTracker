/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "Adapters/Host/system/HostSystem.h"
#include "Application/Application.h"
#include "System/FileSystem/FileSystem.h"
#include "UIFramework/BasicDatas/GUICreateWindowParams.h"
#include <SDL2/SDL.h>
#include <cstring>
#include <iostream>

int main(int argc, char *argv[]) {
  bool testMode = (argc > 1 && std::string(argv[1]) == "--test-fs");

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  if (testMode) {
    // Filesystem test mode not yet implemented
    // Requires full Application initialization
    SDL_Quit();
    return 0;
  }

  HostSystem::Boot(argc, argv);

  GUICreateWindowParams params;
  params.title = "copingTracker (Host)";

  Application::GetInstance()->Init(params);

  int result = HostSystem::MainLoop();

  HostSystem::Shutdown();

  SDL_Quit();

  return result;
}
