/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "HostFileSystem.h"
#include <cstring>

namespace fs = std::filesystem;

HostFile::HostFile(FILE *file) : file_(file), error_(0) {
}

HostFile::~HostFile() {
  if (file_) {
    fclose(file_);
  }
}

int HostFile::Read(void *ptr, int size) {
  if (!file_)
    return 0;
  int nread = (int)fread(ptr, 1, size, file_);
  if (ferror(file_)) {
    error_ = ferror(file_);
  }
  return nread;
}

int HostFile::GetC() {
  if (!file_)
    return EOF;
  int c = fgetc(file_);
  if (ferror(file_)) {
    error_ = ferror(file_);
  }
  return c;
}

int HostFile::Write(const void *ptr, int size, int nmemb) {
  if (!file_)
    return 0;
  int nwritten = (int)fwrite(ptr, size, nmemb, file_);
  if (ferror(file_)) {
    error_ = ferror(file_);
  }
  return nwritten;
}

void HostFile::Seek(long offset, int whence) {
  if (file_) {
    fseek(file_, offset, whence);
  }
}

long HostFile::Tell() {
  if (!file_)
    return -1;
  return ftell(file_);
}

int HostFile::Error() {
  return error_;
}

bool HostFile::Sync() {
  if (!file_)
    return false;
  return fflush(file_) == 0;
}

void HostFile::Dispose() {
  delete this;
}

bool HostFile::Close() {
  if (file_) {
    int result = fclose(file_);
    file_ = nullptr;
    return result == 0;
  }
  return true;
}

HostFileSystem::HostFileSystem() {
  baseDir_ = fs::current_path() / "data";
  currentDir_ = baseDir_;
  if (!fs::exists(baseDir_)) {
    fs::create_directories(baseDir_);
  }
}

HostFileSystem::~HostFileSystem() {
}

fs::path HostFileSystem::MakeAbsPath(const char *path) const {
  if (!path || path[0] == '\0') {
    return currentDir_;
  }
  if (path[0] == '/') {
    return baseDir_ / std::string(path + 1);
  }
  return currentDir_ / path;
}

FileHandle HostFileSystem::Open(const char *name, const char *mode) {
  if (!name || !mode) {
    return FileHandle();
  }
  fs::path absPath = MakeAbsPath(name);
  FILE *f = fopen(absPath.c_str(), mode);
  if (!f) {
    return FileHandle();
  }
  return FileHandle(new HostFile(f));
}

bool HostFileSystem::chdir(const char *path) {
  if (!path) {
    return false;
  }
  fs::path absPath = MakeAbsPath(path);
  if (!fs::is_directory(absPath)) {
    return false;
  }
  currentDir_ = absPath;
  entries_.clear();
  return true;
}

void HostFileSystem::list(etl::ivector<int> *fileIndexes, const char *filter, uint8_t options) {
  entries_.clear();
  if (fileIndexes) {
    fileIndexes->clear();
  }
  if (!fs::is_directory(currentDir_)) {
    return;
  }
  try {
    for (const auto &entry : fs::directory_iterator(currentDir_)) {
      bool isDir = fs::is_directory(entry);
      bool isFile = fs::is_regular_file(entry);
      if (isDir && !(options & loFolders))
        continue;
      if (isFile && !(options & loFiles))
        continue;
      entries_.push_back(entry);
    }
    if (fileIndexes) {
      for (int i = 0; i < (int)entries_.size() && fileIndexes->size() < fileIndexes->max_size(); ++i) {
        fileIndexes->push_back(i);
      }
    }
  } catch (...) {
  }
}

void HostFileSystem::getFileName(int index, char *name, int length) {
  if (!name || length <= 0 || index < 0 || index >= (int)entries_.size()) {
    if (name && length > 0)
      name[0] = '\0';
    return;
  }
  std::string fname = entries_[index].path().filename().string();
  strncpy(name, fname.c_str(), length - 1);
  name[length - 1] = '\0';
}

PicoFileType HostFileSystem::getFileType(int index) {
  if (index < 0 || index >= (int)entries_.size()) {
    return PFT_UNKNOWN;
  }
  return fs::is_directory(entries_[index]) ? PFT_DIR : PFT_FILE;
}

bool HostFileSystem::isParentRoot() {
  return currentDir_ == baseDir_;
}

bool HostFileSystem::isCurrentRoot() {
  return currentDir_ == baseDir_;
}

bool HostFileSystem::DeleteFile(const char *name) {
  if (!name)
    return false;
  fs::path absPath = MakeAbsPath(name);
  try {
    return fs::remove(absPath);
  } catch (...) {
    return false;
  }
}

bool HostFileSystem::DeleteDir(const char *name) {
  if (!name)
    return false;
  fs::path absPath = MakeAbsPath(name);
  try {
    return fs::remove_all(absPath) > 0;
  } catch (...) {
    return false;
  }
}

bool HostFileSystem::exists(const char *path) {
  if (!path)
    return false;
  fs::path absPath = MakeAbsPath(path);
  try {
    return fs::exists(absPath);
  } catch (...) {
    return false;
  }
}

bool HostFileSystem::makeDir(const char *path, bool pFlag) {
  if (!path)
    return false;
  fs::path absPath = MakeAbsPath(path);
  try {
    if (pFlag) {
      return fs::create_directories(absPath);
    } else {
      return fs::create_directory(absPath);
    }
  } catch (...) {
    return false;
  }
}

uint64_t HostFileSystem::getFileSize(int index) {
  if (index < 0 || index >= (int)entries_.size()) {
    return 0;
  }
  try {
    return fs::file_size(entries_[index]);
  } catch (...) {
    return 0;
  }
}

bool HostFileSystem::CopyFile(const char *srcFilename, const char *destFilename) {
  if (!srcFilename || !destFilename)
    return false;
  fs::path srcPath = MakeAbsPath(srcFilename);
  fs::path destPath = MakeAbsPath(destFilename);
  try {
    fs::copy(srcPath, destPath, fs::copy_options::overwrite_existing);
    return true;
  } catch (...) {
    return false;
  }
}

bool HostFileSystem::MoveFile(const char *srcFilename, const char *destFilename) {
  if (!srcFilename || !destFilename)
    return false;
  fs::path srcPath = MakeAbsPath(srcFilename);
  fs::path destPath = MakeAbsPath(destFilename);
  try {
    fs::rename(srcPath, destPath);
    return true;
  } catch (...) {
    return false;
  }
}

bool HostFileSystem::isExFat() {
  return false;
}
