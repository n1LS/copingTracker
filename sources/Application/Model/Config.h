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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "Application/Persistency/Persistent.h"
#include "Foundation/T_Singleton.h"
#include "Foundation/Variables/StringVariable.h"
#include "Foundation/Variables/VariableContainer.h"
#include "Foundation/Variables/WatchedVariable.h"
#include "System/Console/Trace.h"

#define DEFAULT_PREVIEW_VOLUME 60

class Config : public T_Singleton<Config>, public VariableContainer {
public:
  Config();
  ~Config();
  int GetValue(Token token);
  void ProcessArguments(int argc, char **argv);
  bool Save();

  // Methods for handling color variables and themes
  void WriteColorVariables(tinyxml2::XMLPrinter *printer);
  void ReadColorVariable(PersistencyDocument *doc);

  // Theme-related methods (replacing Theme class)
  bool SaveTheme(tinyxml2::XMLPrinter *printer, const char *themeName);
  bool LoadTheme(PersistencyDocument *doc);
  bool ExportTheme(const char *themeName, bool overwrite);
  bool ImportTheme(const char *themeName);

private:
  etl::list<Variable *, 29> variables_;
  // Config variables (kept as members to avoid heap allocation)
  WatchedVariable color0_;
  WatchedVariable color1_;
  WatchedVariable color2_;
  WatchedVariable color3_;
  WatchedVariable color4_;
  WatchedVariable color5_;
  WatchedVariable color6_;
  WatchedVariable color7_;
  WatchedVariable color8_;
  WatchedVariable color9_;
  WatchedVariable color10_;
  WatchedVariable color11_;
  WatchedVariable color12_;
  WatchedVariable color13_;
  WatchedVariable color14_;
  WatchedVariable color15_;

  WatchedVariable lineOut_;
  WatchedVariable midiDevice_;
  WatchedVariable midiSync_;
  WatchedVariable mirrorUI_;
  WatchedVariable importResampler_;
  WatchedVariable commandInputMode_;
  WatchedVariable uiFont_;
  StringVariable<MAX_VARIABLE_STRING_LENGTH> themeName_;
  WatchedVariable backlightLevel_;
  WatchedVariable outputVolume_;
  WatchedVariable keyDelay_;
  WatchedVariable keyRepeat_;
  WatchedVariable previewVolume_;

  void SaveContent(tinyxml2::XMLPrinter *printer);
  void useDefaultConfig();
};

#endif
