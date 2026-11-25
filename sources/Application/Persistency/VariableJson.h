/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _VARIABLE_JSON_H_
#define _VARIABLE_JSON_H_

#include "Application/Persistency/JsonReader.h"
#include "Application/Persistency/JsonWriter.h"
#include "Foundation/Variables/Variable.h"

// A variable as a member of the open object, by its name: numbers, switches and the like as numbers / booleans,
// the lists as the text of the chosen option (so they survive a change of the order of the options), strings as
// strings.
inline void WriteVariableJson(JsonWriter &writer, Variable &variable) {
  switch (variable.GetType()) {
    case Variable::BOOL:
      writer.Bool(variable.GetName(), variable.GetBool());
      break;
    case Variable::CHAR_LIST:
    case Variable::STRING:
      writer.String(variable.GetName(), variable.GetPersistenceString().c_str());
      break;
    default:
      writer.Int(variable.GetName(), variable.GetInt());
      break;
  }
}

// reads the value that WriteVariableJson wrote, a value of the wrong kind is an error of the reader
inline void ReadVariableJson(JsonReader &reader, Variable &variable) {
  switch (variable.GetType()) {
    case Variable::BOOL:
      {
        const bool value = reader.ReadBool();
        if (!reader.HadError()) {
          variable.SetBool(value);
        }
        break;
      }
    case Variable::CHAR_LIST:
    case Variable::STRING:
      {
        char text[MAX_VARIABLE_STRING_LENGTH + 1];
        reader.ReadString(text, sizeof(text));
        // an empty option text is a list without a valid selection
        if (!reader.HadError() && (text[0] != '\0' || variable.GetType() == Variable::STRING)) {
          variable.SetString(text);
        }
        break;
      }
    default:
      {
        const int value = reader.ReadInt();
        if (!reader.HadError()) {
          variable.SetInt(value);
        }
        break;
      }
  }
}

#endif
