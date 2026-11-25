/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "JsonReader.h"
#include "System/FileSystem/I_File.h"

JsonReader::JsonReader(I_File &file) : file_(file) {
}

int JsonReader::peek() {
  if (position_ == length_) {
    if (atEnd_) {
      return -1;
    }
    const int n = file_.Read(buffer_, static_cast<int>(sizeof(buffer_)));
    if (n <= 0) {
      atEnd_ = true;
      length_ = position_ = 0;
      return -1;
    }
    length_ = static_cast<size_t>(n);
    position_ = 0;
  }
  return buffer_[position_];
}

int JsonReader::get() {
  const int c = peek();
  if (c >= 0) {
    position_++;
  }
  return c;
}

void JsonReader::skipWhitespace() {
  for (int c = peek(); c == ' ' || c == '\t' || c == '\n' || c == '\r'; c = peek()) {
    position_++;
  }
}

bool JsonReader::fail() {
  error_ = true;
  return false;
}

bool JsonReader::expect(char c) {
  if (get() != c) {
    return fail();
  }
  return true;
}

bool JsonReader::begin(char open, bool isArray) {
  if (error_) {
    return false;
  }
  skipWhitespace();
  if (peek() != open || depth_ == kMaxDepth) {
    return fail();
  }
  get();
  const uint32_t bit = 1u << depth_;
  arrayBits_ = isArray ? (arrayBits_ | bit) : (arrayBits_ & ~bit);
  commaBits_ &= ~bit;
  depth_++;
  return true;
}

bool JsonReader::BeginObject() {
  return begin('{', false);
}

bool JsonReader::BeginArray() {
  return begin('[', true);
}

// moves to the next member / element of the current container: false at its end (or on an error), true when a
// member / element follows, the separator in front of it is consumed
bool JsonReader::advance(bool isArray) {
  if (error_) {
    return false;
  }
  if (depth_ == 0 || (((arrayBits_ >> (depth_ - 1)) & 1u) != 0) != isArray) {
    return fail();
  }
  skipWhitespace();
  const int c = peek();
  if (c == (isArray ? ']' : '}')) {
    get();
    depth_--;
    return false;
  }
  const uint32_t bit = 1u << (depth_ - 1);
  if ((commaBits_ & bit) != 0) {
    if (c != ',') {
      return fail();
    }
    get();
    skipWhitespace();
  }
  commaBits_ |= bit;
  return true;
}

bool JsonReader::NextKey() {
  if (!advance(false)) {
    return false;
  }
  skipWhitespace();
  size_t stored = 0;
  if (!expect('"') || !readStringBody(key_, sizeof(key_), stored)) {
    key_[0] = '\0';
    return false;
  }
  skipWhitespace();
  if (!expect(':')) {
    key_[0] = '\0';
    return false;
  }
  return true;
}

bool JsonReader::NextElement() {
  return advance(true);
}

int JsonReader::hexDigit() {
  const int c = get();
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

// after the opening quote: reads up to the closing one, stores what fits (capacity includes the terminator)
bool JsonReader::readStringBody(char *dst, size_t capacity, size_t &stored) {
  stored = 0;
  auto store = [&](char c) {
    if (dst && stored + 1 < capacity) {
      dst[stored++] = c;
    }
  };
  auto storeUtf8 = [&](uint32_t cp) {
    char encoded[4];
    size_t n = 0;
    if (cp < 0x80) {
      encoded[n++] = static_cast<char>(cp);
    } else if (cp < 0x800) {
      encoded[n++] = static_cast<char>(0xC0 | (cp >> 6));
      encoded[n++] = static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
      encoded[n++] = static_cast<char>(0xE0 | (cp >> 12));
      encoded[n++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      encoded[n++] = static_cast<char>(0x80 | (cp & 0x3F));
    } else {
      encoded[n++] = static_cast<char>(0xF0 | (cp >> 18));
      encoded[n++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
      encoded[n++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      encoded[n++] = static_cast<char>(0x80 | (cp & 0x3F));
    }
    // a character is stored whole or not at all
    if (dst && stored + n < capacity) {
      for (size_t i = 0; i < n; i++) {
        dst[stored++] = encoded[i];
      }
    }
  };
  auto readHex4 = [&](uint32_t &value) {
    value = 0;
    for (int i = 0; i < 4; i++) {
      const int d = hexDigit();
      if (d < 0) {
        return false;
      }
      value = (value << 4) | static_cast<uint32_t>(d);
    }
    return true;
  };

  for (;;) {
    const int c = get();
    if (c < 0) {
      return fail();
    }
    if (c == '"') {
      break;
    }
    if (c != '\\') {
      store(static_cast<char>(c));
      continue;
    }
    const int e = get();
    switch (e) {
      case '"':
      case '\\':
      case '/':
        store(static_cast<char>(e));
        break;
      case 'b':
        store('\b');
        break;
      case 'f':
        store('\f');
        break;
      case 'n':
        store('\n');
        break;
      case 'r':
        store('\r');
        break;
      case 't':
        store('\t');
        break;
      case 'u':
        {
          uint32_t cp;
          if (!readHex4(cp)) {
            return fail();
          }
          if (cp >= 0xD800 && cp <= 0xDBFF) {
            // a surrogate pair is one character, a lone one becomes '?'
            if (peek() == '\\') {
              get();
              uint32_t low;
              if (get() != 'u' || !readHex4(low)) {
                return fail();
              }
              if (low >= 0xDC00 && low <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
              } else {
                cp = '?';
              }
            } else {
              cp = '?';
            }
          } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
            cp = '?';
          }
          storeUtf8(cp);
          break;
        }
      default:
        return fail();
    }
  }
  if (dst && capacity > 0) {
    dst[stored] = '\0';
  }
  return true;
}

size_t JsonReader::ReadString(char *dst, size_t capacity) {
  if (capacity > 0) {
    dst[0] = '\0';
  }
  if (error_) {
    return 0;
  }
  skipWhitespace();
  size_t stored = 0;
  if (!expect('"') || !readStringBody(dst, capacity, stored)) {
    if (capacity > 0) {
      dst[0] = '\0';
    }
    return 0;
  }
  return stored;
}

int64_t JsonReader::ReadInt64() {
  if (error_) {
    return 0;
  }
  skipWhitespace();
  bool negative = false;
  if (peek() == '-') {
    negative = true;
    get();
  }
  int c = peek();
  if (c < '0' || c > '9') {
    fail();
    return 0;
  }
  // far beyond what the clamps below accept, the further digits are only consumed
  constexpr int64_t limit = 1000000000000000LL;
  int64_t value = 0;
  for (; c >= '0' && c <= '9'; c = peek()) {
    if (value < limit) {
      value = value * 10 + (c - '0');
    }
    get();
  }
  if (c == '.') {
    get();
    for (c = peek(); c >= '0' && c <= '9'; c = peek()) {
      get();
    }
  }
  if (c == 'e' || c == 'E') {
    // never written, not worth supporting
    fail();
    return 0;
  }
  return negative ? -value : value;
}

int32_t JsonReader::ReadInt() {
  const int64_t value = ReadInt64();
  if (value > INT32_MAX) {
    return INT32_MAX;
  }
  if (value < INT32_MIN) {
    return INT32_MIN;
  }
  return static_cast<int32_t>(value);
}

uint32_t JsonReader::ReadUInt() {
  const int64_t value = ReadInt64();
  if (value < 0) {
    return 0;
  }
  if (value > UINT32_MAX) {
    return UINT32_MAX;
  }
  return static_cast<uint32_t>(value);
}

bool JsonReader::ReadBool() {
  if (error_) {
    return false;
  }
  skipWhitespace();
  const char *rest;
  bool value;
  const int c = get();
  if (c == 't') {
    rest = "rue";
    value = true;
  } else if (c == 'f') {
    rest = "alse";
    value = false;
  } else {
    return fail();
  }
  for (; *rest; rest++) {
    if (get() != *rest) {
      return fail();
    }
  }
  return value;
}

// numbers, true, false, null: up to the next delimiter
bool JsonReader::skipLiteral() {
  size_t length = 0;
  for (int c = peek(); c >= 0 && c != ',' && c != '}' && c != ']' && c != ' ' && c != '\t' && c != '\n' && c != '\r' &&
                       c != '"' && c != '{' && c != '[' && c != ':';
       c = peek()) {
    get();
    length++;
  }
  return length > 0 ? true : fail();
}

bool JsonReader::Skip() {
  if (error_) {
    return false;
  }
  skipWhitespace();
  const int c = peek();
  if (c == '"') {
    get();
    size_t stored;
    return readStringBody(nullptr, 0, stored);
  }
  if (c != '{' && c != '[') {
    return skipLiteral();
  }
  // brackets of all kinds are only counted, strings are skipped whole so brackets in them do not count
  size_t nesting = 0;
  for (;;) {
    const int d = get();
    if (d < 0) {
      return fail();
    }
    if (d == '"') {
      size_t stored;
      if (!readStringBody(nullptr, 0, stored)) {
        return false;
      }
    } else if (d == '{' || d == '[') {
      nesting++;
    } else if (d == '}' || d == ']') {
      if (--nesting == 0) {
        return true;
      }
    }
  }
}
