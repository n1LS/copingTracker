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

#ifndef _GROOVE_H_
#define _GROOVE_H_

#include "Application/Model/Song.h"
#include "Application/Model/UndoJournal.h"
#include "Application/Persistency/Persistent.h"
#include "Foundation/T_Singleton.h"

#define MAX_GROOVES 0x20
#define STEPS_PER_GROOVE 16
#define NO_GROOVE_DATA 0xFF

struct ChannelGroove {
  unsigned char groove_;   // selected groove
  unsigned char position_; // step in the groove table
  unsigned char ticks_;    // number of ticks before next step
};

class Groove : public T_Singleton<Groove>, Persistent {
public:
  Groove();
  ~Groove();
  void Reset();
  void Clear();
  void Trigger();
  bool TriggerChannel(int channel);
  void SetGroove(int channel, int groove);
  bool UpdateGroove(ChannelGroove &g, bool reverse);
  void GetChannelData(int channel, int *groove, int *position);
  // The steps of a groove, EditGrooveData records them in the undo history before it hands them out (call it right
  // before the change, not to look).
  const unsigned char *GetGrooveData(int groove) const;
  unsigned char *EditGrooveData(int groove);
  virtual void SaveContent(JsonWriter &writer);
  virtual void RestoreContent(JsonReader &reader);

private:
  ChannelGroove channelGroove_[SONG_CHANNEL_COUNT];
  static unsigned char data_[MAX_GROOVES][STEPS_PER_GROOVE];
};
#endif
