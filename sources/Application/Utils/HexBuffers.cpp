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

#include "HexBuffers.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "Application/Utils/char.h"
#include "Externals/etl/include/etl/string.h"

#define XML_CUT_LENGTH 64

void prepareHexChunk(tinyxml2::XMLPrinter *printer, unsigned char *datasrc, int len) {

  bool singleValue = true;
  int singleValueData = -1;
  unsigned char hexBuffer[XML_CUT_LENGTH * 2 + 1] = "";

  char *hex = (char *)hexBuffer;
  for (int i = 0; i < len; i++) {
    byteToHexString(*datasrc, hex);
    if (singleValueData == -1) {
      singleValueData = *datasrc;
    } else {
      if (singleValueData != *datasrc) {
        singleValue = false;
      }
    };
    datasrc++;
    hex += 2;
  };
  if (singleValue) {
    printer->PushAttribute(XML_ATTR_VALUE, singleValueData);
    printer->PushAttribute(XML_ATTR_LENGTH, len);
  } else {
    printer->PushText(reinterpret_cast<const char *>(hexBuffer));
  }
}

void saveHexBuffer(tinyxml2::XMLPrinter *printer, const char *nodeName, unsigned char *src, unsigned len) {

  printer->OpenElement(nodeName);

  unsigned int count = len / XML_CUT_LENGTH;
  unsigned char *datasrc = (unsigned char *)src;

  for (unsigned i = 0; i < count; i++) {
    printer->OpenElement(XML_ELEM_DATA);
    prepareHexChunk(printer, datasrc, XML_CUT_LENGTH);
    datasrc += XML_CUT_LENGTH;
    printer->CloseElement();
  };

  len -= count * XML_CUT_LENGTH;
  if (len > 0) {
    printer->OpenElement(XML_ELEM_DATA);
    prepareHexChunk(printer, datasrc, len);
    printer->CloseElement();
  }
  printer->CloseElement();
}

void saveHexBuffer(tinyxml2::XMLPrinter *printer, const char *nodeName, unsigned int *src, unsigned len) {
  saveHexBuffer(printer, nodeName, (unsigned char *)src, len * sizeof(int));
}

void saveHexBuffer(tinyxml2::XMLPrinter *printer, const char *nodeName, uint16_t *src, unsigned len) {
  saveHexBuffer(printer, nodeName, (unsigned char *)src, len * sizeof(uint16_t));
}

void saveHexBuffer(tinyxml2::XMLPrinter *printer, const char *nodeName, FourCC *src, unsigned len) {
  saveHexBuffer(printer, nodeName, (unsigned char *)src, len * sizeof(FourCC::enum_type));
}

void restoreHexBuffer(PersistencyDocument *doc, unsigned char *destination) {
  unsigned char *dst = destination;

  bool child = doc->FirstChild();
  while (child) {
    bool hasAttr = doc->NextAttribute();
    if (hasAttr) {
      int data = 0;
      int length = 0;
      bool gotData = false;
      while (hasAttr) {
        if (!strcmp(doc->attrname_, XML_ATTR_VALUE)) {
          data = atoi(doc->attrval_);
          gotData = true;
        }
        if (!strcmp(doc->attrname_, XML_ATTR_LENGTH)) {
          length = atoi(doc->attrval_);
        }
        hasAttr = doc->NextAttribute();
      }
      if (gotData) {
        memset(dst, data, length);
      }
      dst += length;
    } else {
      if (doc->HasContent()) {
        for (unsigned int i = 0; i < strlen(doc->content_) / 2; i++) {
          *dst++ = hexStringToByte(doc->content_ + i * 2);
        }
      }
    }
    child = doc->NextSibling();
  }
}
