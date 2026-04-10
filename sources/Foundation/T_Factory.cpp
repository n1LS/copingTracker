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

template <class Item> Item *T_Factory<Item>::instance_ = 0;

template <class Item> void T_Factory<Item>::Install(Item *instance) {
  instance_ = instance;
}
template <class Item> Item *T_Factory<Item>::GetInstance() {
  return instance_;
}
