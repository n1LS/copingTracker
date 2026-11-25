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

#ifndef _PERSISTENT_H_
#define _PERSISTENT_H_

#include "Application/Persistency/JsonReader.h"
#include "Application/Persistency/JsonWriter.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Foundation/Services/SubService.h"
#include "System/Console/n_assert.h"

class Persistent : SubService {
public:
  Persistent(const char *nodeName, bool registerWithService = true);
  ~Persistent();
  // the name of the member the content is saved under in the project file
  const char *NodeName() const {
    return nodeName_;
  }
  // writes "NodeName": { content }, as a member of the object that is open
  void Save(JsonWriter &writer);
  // writes { content } on its own, as the root of a document (instruments files)
  void SaveDocument(JsonWriter &writer);
  // reads the value that follows a key (or the root of a document): an object whose members are restored
  bool Restore(JsonReader &reader);

protected:
  // writes the members of the object that is open
  virtual void SaveContent(JsonWriter &writer) = 0;
  // reads the members of the object that is open: loops NextKey() up to the end of the object, members that are
  // not known are skipped
  virtual void RestoreContent(JsonReader &reader) = 0;

private:
  const char *nodeName_;
  bool registerWithService_;
};

#endif
