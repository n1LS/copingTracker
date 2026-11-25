/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2026 nILS Podewski
 *
 * This file is part of the copingTracker firmware
 */

#include "JsonWriter.h"
#include "System/FileSystem/I_File.h"
#include <string.h>

JsonWriter::JsonWriter(I_File &file) : file_(file) {
}

JsonWriter::~JsonWriter() {
  flush();
}

void JsonWriter::flush() {
  if (used_ == 0) {
    return;
  }
  if (file_.Write(buffer_, 1, static_cast<int>(used_)) != static_cast<int>(used_)) {
    flushFailed_ = true;
  }
  used_ = 0;
}

void JsonWriter::put(const char *data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    putChar(data[i]);
  }
}

void JsonWriter::putChar(char c) {
  if (used_ == sizeof(buffer_)) {
    flush();
  }
  buffer_[used_++] = c;
}

void JsonWriter::newline(size_t depth) {
  putChar('\n');
  for (size_t i = 0; i < depth; i++) {
    putChar(' ');
    putChar(' ');
  }
}

void JsonWriter::writeString(const char *s) {
  static const char hex[] = "0123456789abcdef";
  putChar('"');
  for (; *s; s++) {
    const unsigned char c = static_cast<unsigned char>(*s);
    switch (c) {
      case '"':
        put("\\\"", 2);
        break;
      case '\\':
        put("\\\\", 2);
        break;
      case '\n':
        put("\\n", 2);
        break;
      case '\r':
        put("\\r", 2);
        break;
      case '\t':
        put("\\t", 2);
        break;
      default:
        if (c < 0x20) {
          put("\\u00", 4);
          putChar(hex[c >> 4]);
          putChar(hex[c & 0xF]);
        } else {
          putChar(static_cast<char>(c));
        }
    }
  }
  putChar('"');
}

void JsonWriter::beginValue(const char *key, bool isContainer) {
  if (depth_ == 0) {
    // the root: a single value
    if (rootWritten_ || key) {
      error_ = true;
      return;
    }
    rootWritten_ = true;
    return;
  }

  Frame &top = stack_[depth_ - 1];
  if (top.isArray) {
    if (key) {
      error_ = true;
      return;
    }
    if (!top.hasItems) {
      // an array of containers is written one element per line, an array of plain values in one line
      top.multiline = isContainer;
    } else {
      putChar(',');
      if (!top.multiline) {
        putChar(' ');
      }
    }
    if (top.multiline) {
      newline(depth_);
    }
  } else {
    if (!key) {
      error_ = true;
      return;
    }
    if (top.hasItems) {
      putChar(',');
    }
    newline(depth_);
    writeString(key);
    put(": ", 2);
  }
  top.hasItems = true;
}

void JsonWriter::begin(const char *key, bool isArray) {
  if (error_) {
    return;
  }
  if (depth_ == kMaxDepth) {
    error_ = true;
    return;
  }
  beginValue(key, true);
  if (error_) {
    return;
  }
  putChar(isArray ? '[' : '{');
  stack_[depth_++] = {isArray, false, false};
}

void JsonWriter::end(bool isArray) {
  if (error_) {
    return;
  }
  if (depth_ == 0 || stack_[depth_ - 1].isArray != isArray) {
    error_ = true;
    return;
  }
  const Frame frame = stack_[--depth_];
  if (frame.hasItems && (!isArray || frame.multiline)) {
    newline(depth_);
  }
  putChar(isArray ? ']' : '}');
}

void JsonWriter::BeginObject(const char *key) {
  begin(key, false);
}

void JsonWriter::EndObject() {
  end(false);
}

void JsonWriter::BeginArray(const char *key) {
  begin(key, true);
}

void JsonWriter::EndArray() {
  end(true);
}

void JsonWriter::Int(const char *key, int64_t value) {
  if (error_) {
    return;
  }
  beginValue(key, false);
  if (error_) {
    return;
  }
  char digits[24];
  size_t n = 0;
  // the magnitude as unsigned: negating INT64_MIN would overflow
  uint64_t magnitude = value < 0 ? 0 - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
  do {
    digits[n++] = static_cast<char>('0' + magnitude % 10);
    magnitude /= 10;
  } while (magnitude != 0);
  if (value < 0) {
    putChar('-');
  }
  while (n > 0) {
    putChar(digits[--n]);
  }
}

void JsonWriter::Int(int64_t value) {
  Int(nullptr, value);
}

void JsonWriter::Bool(const char *key, bool value) {
  if (error_) {
    return;
  }
  beginValue(key, false);
  if (error_) {
    return;
  }
  put(value ? "true" : "false", value ? 4 : 5);
}

void JsonWriter::Bool(bool value) {
  Bool(nullptr, value);
}

void JsonWriter::String(const char *key, const char *value) {
  if (error_) {
    return;
  }
  beginValue(key, false);
  if (error_) {
    return;
  }
  writeString(value ? value : "");
}

void JsonWriter::String(const char *value) {
  String(nullptr, value);
}

bool JsonWriter::Finish() {
  if (depth_ != 0 || !rootWritten_) {
    error_ = true;
  }
  if (!error_) {
    putChar('\n');
  }
  flush();
  return !error_ && !flushFailed_ && file_.Error() == 0;
}
