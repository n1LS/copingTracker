/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef HOST_FILESYSTEM_H_
#define HOST_FILESYSTEM_H_

#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/I_File.h"
#include <cstdio>
#include <filesystem>
#include <vector>

class HostFile : public I_File {
public:
  HostFile(FILE *file);
  virtual ~HostFile();

  virtual int Read(void *ptr, int size) override;
  virtual int GetC() override;
  virtual int Write(const void *ptr, int size, int nmemb) override;
  virtual void Seek(long offset, int whence) override;
  virtual long Tell() override;
  virtual int Error() override;
  virtual bool Sync() override;
  virtual void Dispose() override;

protected:
  virtual bool Close() override;

private:
  FILE *file_;
  int error_;
};

class HostFileSystem : public FileSystem {
public:
  HostFileSystem();
  virtual ~HostFileSystem();

  virtual FileHandle Open(const char *name, const char *mode) override;
  virtual bool chdir(const char *path) override;
  virtual void list(etl::ivector<int> *fileIndexes, const char *filter, uint8_t options = loDefault) override;
  virtual void getFileName(int index, char *name, int length) override;
  virtual PicoFileType getFileType(int index) override;
  virtual bool isParentRoot() override;
  virtual bool isCurrentRoot() override;
  virtual bool DeleteFile(const char *name) override;
  virtual bool DeleteDir(const char *name) override;
  virtual bool exists(const char *path) override;
  virtual bool makeDir(const char *path, bool pFlag = false) override;
  virtual uint64_t getFileSize(int index) override;
  virtual bool CopyFile(const char *srcFilename, const char *destFilename) override;
  virtual bool MoveFile(const char *srcFilename, const char *destFilename) override;
  virtual bool isExFat() override;

private:
  std::filesystem::path baseDir_;
  std::filesystem::path currentDir_;
  std::vector<std::filesystem::directory_entry> entries_;

  std::filesystem::path MakeAbsPath(const char *path) const;
};

#endif
