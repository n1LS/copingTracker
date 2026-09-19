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

#ifndef _PERSISTENCY_DOCUMENT_H_
#define _PERSISTENCY_DOCUMENT_H_

#include "Externals/yxml/yxml.h"
#include "System/FileSystem/FileHandle.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Memory/MemoryPool.h"

class PersistencyDocument {
public:
  PersistencyDocument();
  ~PersistencyDocument(); // Add destructor
  bool Load(const char *filename);
  void Close(); // Add method to explicitly close the file

  // r_ < YXML_OK to signal that the xml parsing had a fatal error
  bool HadError() const {
    return r_ < YXML_OK;
  }

  char *attrname() {
    return MemoryPool::persistencyAttrName();
  }
  char *attrval() {
    return MemoryPool::persistencyAttrVal();
  }
  char *content() {
    return MemoryPool::persistencyAttrContent();
  }
  int contentsize() {
    return MemoryPool::persistencyAttrContentSize;
  }
  int attrsize() {
    return MemoryPool::persistencyAttrSize;
  }
  int attrnamesize() {
    return MemoryPool::persistencyAttrNameSize;
  }

  bool FirstChild();
  bool NextSibling();
  bool NextAttribute();
  bool HasContent();
  char *ElemName();

  yxml_ret_t r_;

  int version_;

private:
  inline static yxml_t state_[1];
  FileHandle fp_;
};
#endif
