/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <memory.h>
#include <assert.h>
#include <initializer_list>
#include "expat.h"

static void XMLCALL start(void *, const char *, const char **) {}
static void XMLCALL end(void *, const char *) {}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  for (auto enc : {"UTF-16", "UTF-8", "ISO_8859_1", "US_ASCII", "UTF_16BE",
                   "UTF_16LE", (const char *)nullptr}) {
    XML_Parser p = XML_ParserCreate(enc);
    assert(p);
    XML_SetElementHandler(p, start, end);

    void *buf = XML_GetBuffer(p, size);
    memcpy(buf, data, size);
    XML_ParseBuffer(p, size, size == 0);
    XML_ParserFree(p);
  }
  return 0;
}
