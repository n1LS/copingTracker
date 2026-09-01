/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _UI_TABFIELD_H
#define _UI_TABFIELD_H

#include "UIIntVarField.h"

class UITabField : public UIIntVarField{
public:
  UITabField(const char *label, const GUIPoint &position, Variable &variable, const char *tabs[], int count);
  void Draw(GUIWindow &w, int offset = 0);
  int GetFocusOffset();
private:
  int min_;
  int max_;
  Token action_;
  Variable &src_;

  const char *label_;
  etl::vector<const char *, 10> tabs_;
};


#endif