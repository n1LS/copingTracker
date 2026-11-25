/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _JSON_WRITER_H_
#define _JSON_WRITER_H_

#include <stddef.h>
#include <stdint.h>

class I_File;

// Streams JSON to a file, nothing is held in memory but a small output buffer and the nesting state.
//
// Objects get one member per line, arrays of numbers / strings stay on one line, arrays of objects get one
// element per line:
//
//   {
//     "version": 1,
//     "steps": [1, 2, 3],
//     "chains": [
//       {
//         "id": 4
//       }
//     ]
//   }
//
// The key arguments are for members of an object, the variants without key for the root and for elements of an
// array. Misuse (key missing in an object, too deep, unbalanced) sets the error flag and writes nothing more.
class JsonWriter {
public:
  explicit JsonWriter(I_File &file);
  ~JsonWriter();

  void BeginObject(const char *key = nullptr);
  void EndObject();
  void BeginArray(const char *key = nullptr);
  void EndArray();

  void Int(const char *key, int64_t value);
  void Int(int64_t value);
  void Bool(const char *key, bool value);
  void Bool(bool value);
  void String(const char *key, const char *value);
  void String(const char *value);

  // a whole array of numbers: "key": [1, 2, 3]
  template <typename T> void IntArray(const char *key, const T *values, size_t count) {
    BeginArray(key);
    for (size_t i = 0; i < count; i++) {
      Int(static_cast<int64_t>(values[i]));
    }
    EndArray();
  }

  // writes the closing newline and everything buffered, true when the document is complete and every write worked
  bool Finish();
  bool HadError() const {
    return error_;
  }

  static constexpr size_t kMaxDepth = 16;

private:
  struct Frame {
    bool isArray;
    bool hasItems;
    bool multiline;
  };

  // everything that comes before a value: separator, line break, indentation, key
  void beginValue(const char *key, bool isContainer);
  void begin(const char *key, bool isArray);
  void end(bool isArray);
  void newline(size_t depth);
  void writeString(const char *s);
  void put(const char *data, size_t length);
  void putChar(char c);
  void flush();

  I_File &file_;
  Frame stack_[kMaxDepth];
  size_t depth_ = 0;
  bool rootWritten_ = false;
  bool error_ = false;
  bool flushFailed_ = false;
  char buffer_[128];
  size_t used_ = 0;
};

#endif
