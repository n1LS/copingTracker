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

#ifndef _CHAR_H_
#define _CHAR_H_

#include "Foundation/Types/Types.h"
#include "Application/Instruments/InstrumentBank.h"
#include "System/Console/nanoprintf.h"
#include <string.h>

extern const char hexChars[16];
extern const char *noteNames[12];

static inline unsigned char hexNibble(char c) {
    unsigned char v = c - '0';
    if (v < 10) return v;

    v = (c | 0x20) - 'a'; // lowercase normalize trick
    return (v < 6) ? (v + 10) : 0;
}

static inline char hexChar(unsigned char v) {
    return hexChars[v & 0x0F];
}

inline void noteToString(unsigned char d, char *s) {
  int oct = d / 12;
  int note = d % 12;
  strcpy(s, noteNames[note]);
  s[2] = hexChars[oct];
  s[3] = 0;
}

inline uint8_t hexStringToByte(char *s) {
  return (hexNibble(s[0]) << 4) + hexNibble(s[1]);
}

static inline void byteToHexString(unsigned char c, char *s) {
    s[0] = hexChar(c >> 4);
    s[1] = hexChar(c);
    s[2] = 0;
}

inline void formatNote(uint8_t note, uint8_t instrument, InstrumentBank *bank, char *buffer) {
  if (note == NO_NOTE) {
    npf_snprintf(buffer, sizeof(buffer), "---");
  } else if (note == NOTE_OFF) {
    npf_snprintf(buffer, sizeof(buffer), "off");
  } else {
    bool showSlice = false;
    bool invalidSlice = false;
    uint8_t sliceIndex = 0;
    if (instrument != 0xFF && bank) {
      I_Instrument *instrObj = bank->GetInstrument(instrument);
      if (instrObj && instrObj->GetType() == IT_SAMPLE) {
        SampleInstrument *sampleInstr = static_cast<SampleInstrument *>(instrObj);
        if (sampleInstr->HasSlicesForPlayback()) {
          if (sampleInstr->ShouldDisplaySliceForNote(note)) {
            showSlice = true;
            sliceIndex = static_cast<uint8_t>(note - SampleInstrument::SliceNoteBase);
          } else {
            invalidSlice = true;
          }
        }
      }
    }
    
    if (showSlice) {
      npf_snprintf(buffer, sizeof(buffer), "S%02u", static_cast<unsigned>(sliceIndex));
    } else if (invalidSlice) {
      npf_snprintf(buffer, sizeof(buffer), "S**");
    } else {
      noteToString(note, buffer);
    }
  }
}

#endif
