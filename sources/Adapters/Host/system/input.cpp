/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "input.h"
#include <SDL2/SDL.h>

#include "Application/Views/BaseClasses/View.h"

uint16_t scanKeys() {
  const uint8_t *key_state = SDL_GetKeyboardState(nullptr);
  uint16_t mask = 0;

  if (key_state[SDL_SCANCODE_LEFT])
    mask |= BM_LEFT;
  if (key_state[SDL_SCANCODE_DOWN])
    mask |= BM_DOWN;
  if (key_state[SDL_SCANCODE_RIGHT])
    mask |= BM_RIGHT;
  if (key_state[SDL_SCANCODE_UP])
    mask |= BM_UP;
  if (key_state[SDL_SCANCODE_A])
    mask |= BM_ALT;
  if (key_state[SDL_SCANCODE_S])
    mask |= BM_NAV;
  if (key_state[SDL_SCANCODE_E])
    mask |= BM_EDIT;
  if (key_state[SDL_SCANCODE_D])
    mask |= BM_ENTER;
  if (key_state[SDL_SCANCODE_SPACE])
    mask |= BM_PLAY;

  return mask;
}
