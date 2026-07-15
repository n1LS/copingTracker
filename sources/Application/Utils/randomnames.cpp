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

#include "randomnames.h"
#include "Adapters/copingTracker/filesystem/picoTrackerFileSystem.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Externals/SdFat/src/SdCard/SdCard.h"
#include <stdlib.h>

static bool getRandomWordFromFile(const FileHandle &file, uint32_t randomBits, int maxLen, char *buffer) {
  file->Seek(0, SEEK_END);
  long fileSize = file->Tell();
  file->Seek(0, SEEK_SET);

  if (fileSize <= 0)
    return false;

  int pos = randomBits % (fileSize - 16);
  file->Seek(pos, SEEK_SET);

  uint8_t byte;
  // scan for the next line break
  while (file->Read(&byte, 1) == 1) {
    if (byte == '\n' || byte == '\r')
      break;
  }

  int i = 0;
  // read the word up to \n or if maxlength is reached
  while (i < maxLen) {
    if (file->Read(&byte, 1) != 1)
      return i > 0;
    if (byte == '\n' || byte == '\r')
      break;
    buffer[i++] = byte;
  }

  // 0-terminate
  buffer[i] = '\0';
  return i > 0;
}

// Generate a random name by reading from adjectives.txt and nouns.txt files
// on the SD card. Picks a random location in each file and reads until newline.
// Max string length: 7 for adjectives, 8 for nouns.
// Returns formatted string "%s-%s" or NULL on error.
static bool getRandomNameFromFile(char *buffer, size_t bufferSize) {
  if (bufferSize < (7 + 1 + 8 + 1))
    return false;

  FileSystem *fs = FileSystem::GetInstance();

  auto adjFile = fs->Open(SD_BASE_DIR "/adjectives.txt", "r");
  auto nounFile = fs->Open(SD_BASE_DIR "/nouns.txt", "r");
  if (!nounFile || !adjFile)
    return false;

  uint32_t randNum = System::GetInstance()->GetRandomNumber();

  char adjective[8];
  char noun[9];
  if (!getRandomWordFromFile(adjFile, randNum & 0xFFFF, 7, adjective) ||
      !getRandomWordFromFile(nounFile, randNum >> 16, 8, noun))
    return false;

  snprintf(buffer, bufferSize, "%s-%s", adjective, noun);
  return true;
}

void getRandomName(char *name, size_t nameSize) {
  // try getting a random name from the SD card first, if available
  if (getRandomNameFromFile(name, nameSize))
    return;

  uint32_t randNum = System::GetInstance()->GetRandomNumber();

  // check buffer is big enough for 4chars per word, with "-" and terminal null
  if (nameSize < (4 + 1 + 4 + 1)) {
    return;
  }
  int adjectivesCount = sizeof(adjectives) / sizeof(adjectives[0]);
  int verbsCount = sizeof(verbs) / sizeof(verbs[0]);
  int rndIndex = randNum % adjectivesCount;
  name[0] = 0; // first make sure buffer is null termin
  strcat(name, adjectives[rndIndex]);
  strcat(name, "-");
  rndIndex = randNum % verbsCount;
  strcat(name, verbs[rndIndex]);
}
