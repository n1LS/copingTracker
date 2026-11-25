/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#ifndef _JSON_READER_H_
#define _JSON_READER_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

class I_File;

// Pull parser for JSON files: reads the file in small chunks, nothing but a read buffer, the current key and the
// nesting state is held in memory. Restoring looks like
//
//   JsonReader r(file);
//   if (r.BeginObject()) {
//     while (r.NextKey()) {
//       if (r.KeyIs("version")) {
//         version = r.ReadInt();
//       } else if (r.KeyIs("song")) {
//         r.BeginObject();
//         while (r.NextKey()) { ... }
//       } else {
//         r.Skip(); // unknown, from a newer version
//       }
//     }
//   }
//   if (r.HadError()) { ... }
//
// After NextKey() / NextElement() returned true the value MUST be consumed (Read*, BeginObject / BeginArray with
// their loops, or Skip). The first problem (bad syntax, wrong value type, end of file) sets the error flag, all
// further calls fail and return 0 / false.
class JsonReader {
public:
  explicit JsonReader(I_File &file);

  // '{' and '[' of the next value
  bool BeginObject();
  bool BeginArray();

  // next member of the current object: its key is available through Key(), false at the closing '}'
  bool NextKey();
  // next element of the current array, false at the closing ']'
  bool NextElement();

  const char *Key() const {
    return key_;
  }
  bool KeyIs(const char *name) const {
    return strcmp(key_, name) == 0;
  }

  // numbers outside of the range are clamped, fractions are cut off, exponents are rejected
  int32_t ReadInt();
  uint32_t ReadUInt();
  bool ReadBool();
  // copies up to capacity - 1 characters (UTF-8, escapes resolved), always terminates, the rest is dropped,
  // returns the number of characters stored
  size_t ReadString(char *dst, size_t capacity);
  // skips the next value, whatever it is, with everything in it
  bool Skip();

  // an array of numbers: stores up to capacity of them, returns how many the array had (0 and error when it is no
  // array)
  template <typename T> size_t ReadIntArray(T *dst, size_t capacity) {
    size_t count = 0;
    if (!BeginArray()) {
      return 0;
    }
    while (NextElement()) {
      const int64_t value = ReadInt64();
      if (count < capacity) {
        dst[count] = static_cast<T>(value);
      }
      count++;
    }
    return count;
  }

  bool HadError() const {
    return error_;
  }

  static constexpr size_t kMaxDepth = 32;
  static constexpr size_t kMaxKeyLength = 47;
  // the size of the read buffer, the tests use a tiny one to exercise the refill
#ifdef JSON_READER_BUFFER_SIZE
  static constexpr size_t kBufferSize = JSON_READER_BUFFER_SIZE;
#else
  static constexpr size_t kBufferSize = 128;
#endif

  // the full range, ReadInt / ReadUInt clamp it
  int64_t ReadInt64();

private:
  int peek();
  int get();
  void skipWhitespace();
  bool fail();
  bool expect(char c);
  bool begin(char open, bool isArray);
  bool advance(bool isArray);
  bool readStringBody(char *dst, size_t capacity, size_t &stored);
  bool skipLiteral();
  int hexDigit();

  I_File &file_;
  uint8_t buffer_[kBufferSize];
  size_t length_ = 0;
  size_t position_ = 0;
  bool atEnd_ = false;
  bool error_ = false;

  // one bit per nesting level: array / object and "has had a member"
  uint32_t arrayBits_ = 0;
  uint32_t commaBits_ = 0;
  size_t depth_ = 0;

  char key_[kMaxKeyLength + 1] = {0};
};

#endif
