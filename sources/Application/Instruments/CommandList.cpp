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

#include "CommandList.h"

// Keep command entries grouped by displayed mnemonic first letter;
// GetNextAlpha/GetPrevAlpha depend on this ordering.
const int CommandList::CommandCount = 29;

const Token CommandList::AllCommands[CommandCount] = {
    Token::InstrumentCommandNone,
    Token::InstrumentCommandArpeggiator,
    Token::InstrumentCommandCrush,
    Token::InstrumentCommandDelay,
    Token::InstrumentCommandFilterCut,
    Token::InstrumentCommandLowPassFilter,
    Token::InstrumentCommandFilterResonance,
    Token::InstrumentCommandGateOff,
    Token::InstrumentCommandGroove,
    Token::InstrumentCommandHop,
    Token::InstrumentCommandInstrumentRetrigger,
    Token::InstrumentCommandKill,
    Token::InstrumentCommandLegato,
    Token::InstrumentCommandLoopOffset,
    Token::InstrumentCommandMidiCC,
    Token::InstrumentCommandMidiChord,
    Token::InstrumentCommandMidiPC,
    Token::InstrumentCommandPan,
    Token::InstrumentCommandPitchFineTune,
    Token::InstrumentCommandPlayOfset,
    Token::InstrumentCommandPitchSlide,
    Token::InstrumentCommandRetrigger,
    Token::InstrumentCommandStop,
    Token::InstrumentCommandSetInstrumentParameter,
    Token::InstrumentCommandTable,
    Token::InstrumentCommandTempo,
    Token::InstrumentCommandVelocity,
    Token::InstrumentCommandVibrato,
    Token::InstrumentCommandVolume,
};

Token CommandList::GetFirst() {
  return Token::InstrumentCommandArpeggiator;
}

static char GetCommandGroupLetter(Token command) {
  const char *name = Token(command).c_str();
  return (name && name[0]) ? name[0] : '\0';
}

// Applies command-specific range limits to parameter values
uint16_t CommandList::RangeLimitCommandParam(Token command, uint16_t paramValue) {
  // Each command type can have its own specific range limits
  if (command == Token::InstrumentCommandVelocity) {
    // For VEL command, limit the bb part to 0x7F (127) while preserving the aa
    // part
    return (paramValue & 0xFF00) | (paramValue & 0x7F);
  }
  // Add more command-specific limits here as needed
  // Example:
  // else if (command == Token::InstrumentCommandMidiCC) {
  //   // MIDI CC values should also be limited to 0-127
  //   return (paramValue & 0xFF00) | (paramValue & 0x7F);
  // }

  // If no specific limit applies, return the original value
  return paramValue;
}

Token CommandList::GetNext(Token current) {
  for (uint32_t i = 0; i < sizeof(AllCommands) / sizeof(Token) - 1; i++) {
    if (AllCommands[i] == current) {
      return AllCommands[i + 1];
    };
  };
  return current;
}

Token CommandList::GetPrev(Token current) {
  uint32_t count = sizeof(AllCommands) / sizeof(Token);
  for (uint32_t i = 2; i < count; i++) {
    if (AllCommands[i] == current) {
      return AllCommands[i - 1];
    };
  };
  return current;
}

Token CommandList::GetNextAlpha(Token current) {
  char letter = GetCommandGroupLetter(current);
  bool found = false;
  for (uint32_t i = 0; i < sizeof(AllCommands) / sizeof(Token); i++) {
    char tLetter = GetCommandGroupLetter(AllCommands[i]);
    if (!found) {
      if (tLetter == letter) {
        found = true;
      }
    } else {
      if (tLetter != letter) {
        return AllCommands[i];
      }
    };
  };
  return current;
}

Token CommandList::GetPrevAlpha(Token current) {

  char letter = GetCommandGroupLetter(current);
  bool found = false;
  Token tReturn = Token::Default;
  uint32_t count = sizeof(AllCommands) / sizeof(Token);

  for (uint32_t i = count - 1; i > 0; i--) {
    char tLetter = GetCommandGroupLetter(AllCommands[i]);
    if (!found) {
      if (tLetter == letter) {
        found = true;
      }
    } else {
      if (tLetter != letter) {
        if (tReturn == 0xFF) {
          tReturn = AllCommands[i];
        } else {
          if (tLetter != GetCommandGroupLetter(tReturn)) {
            return tReturn;
          } else {
            tReturn = AllCommands[i];
          }
        }
      }
    };
  };
  if (tReturn != 0xFF) {
    return tReturn;
  }
  return current;
}
