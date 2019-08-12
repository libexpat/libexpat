#include <stdint.h>
#include <memory.h>
#include <assert.h>
#include <initializer_list>
#include "expat.h"
#include <stdio.h>

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
