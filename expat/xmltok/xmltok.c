/* TODO

Provide methods to convert to any of UTF-8, UTF-18, UCS-4.

Better prolog tokenization

<!NAME
NMTOKEN
NAME
PEREF

*/

#ifdef _MSC_VER
#define XMLTOKAPI __declspec(dllexport)
#endif

#include "xmltok.h"
#include "nametab.h"

#define UCS2_GET_NAMING(pages, hi, lo) \
   (namingBitmap[(pages[hi] << 3) + ((lo) >> 5)] & (1 << ((lo) & 0x1F)))

/* A 2 byte UTF-8 representation splits the characters 11 bits
between the bottom 5 and 6 bits of the bytes.
We need 8 bits to index into pages, 3 bits to add to that index and
5 bits to generate the mask. */
#define UTF8_GET_NAMING2(pages, byte) \
    (namingBitmap[((pages)[(((byte)[0]) >> 2) & 7] << 3) \
                      + ((((byte)[0]) & 3) << 1) \
                      + ((((byte)[1]) >> 5) & 1)] \
         & (1 << (((byte)[1]) & 0x1F)))

/* A 3 byte UTF-8 representation splits the characters 16 bits
between the bottom 4, 6 and 6 bits of the bytes.
We need 8 bits to index into pages, 3 bits to add to that index and
5 bits to generate the mask. */
#define UTF8_GET_NAMING3(pages, byte) \
  (namingBitmap[((pages)[((((byte)[0]) & 0xF) << 4) \
                             + ((((byte)[1]) >> 2) & 0xF)] \
		       << 3) \
                      + ((((byte)[1]) & 3) << 1) \
                      + ((((byte)[2]) >> 5) & 1)] \
         & (1 << (((byte)[2]) & 0x1F)))

#define UTF8_GET_NAMING(pages, p, n) \
  ((n) == 2 \
  ? UTF8_GET_NAMING2(pages, (const unsigned char *)(p)) \
  : ((n) == 3 \
     ? UTF8_GET_NAMING3(pages, (const unsigned char *)(p)) \
     : 0))


#include "xmltok_impl.h"

struct normal_encoding {
  ENCODING enc;
  unsigned char type[256];
};

/* minimum bytes per character */
#define MINBPC 1
#define BYTE_TYPE(enc, p) \
  (((struct normal_encoding *)(enc))->type[(unsigned char)*(p)])
#define IS_NAME_CHAR(enc, p, n) UTF8_GET_NAMING(namePages, p, n)
#define IS_NMSTRT_CHAR(enc, p, n) UTF8_GET_NAMING(nmstrtPages, p, n)

/* c is an ASCII character */
#define CHAR_MATCHES(enc, p, c) (*(p) == c)

#define PREFIX(ident) normal_ ## ident
#include "xmltok_impl.c"

#undef MINBPC
#undef BYTE_TYPE
#undef CHAR_MATCHES
#undef IS_NAME_CHAR
#undef IS_NMSTRT_CHAR

const struct normal_encoding utf8_encoding = {
  { { PREFIX(prologTok), PREFIX(contentTok) }, 1 },
#include "asciitab.h"
#include "utf8tab.h"
};

#undef PREFIX

static unsigned char latin1tab[256] = {
#include "asciitab.h"
#include "latin1tab.h"
};

static int unicode_byte_type(char hi, char lo)
{
  switch ((unsigned char)hi) {
  case 0xD8: case 0xD9: case 0xDA: case 0xDB:
    return BT_LEAD4;
  case 0xDC: case 0xDD: case 0xDE: case 0xDF:
    return BT_TRAIL;
  case 0xFF:
    switch ((unsigned char)lo) {
    case 0xFF:
    case 0xFE:
      return BT_NONXML;
    }
    break;
  }
  return BT_NONASCII;
}

#define PREFIX(ident) little2_ ## ident
#define MINBPC 2
#define BYTE_TYPE(enc, p) \
 ((p)[1] == 0 ? latin1tab[(unsigned char)*(p)] : unicode_byte_type((p)[1], (p)[0]))
#define CHAR_MATCHES(enc, p, c) ((p)[1] == 0 && (p)[0] == c)
#define IS_NAME_CHAR(enc, p, n) \
  UCS2_GET_NAMING(namePages, (unsigned char)p[1], (unsigned char)p[0])
#define IS_NMSTRT_CHAR(enc, p, n) \
  UCS2_GET_NAMING(nmstrtPages, (unsigned char)p[1], (unsigned char)p[0])

#include "xmltok_impl.c"

#undef MINBPC
#undef BYTE_TYPE
#undef CHAR_MATCHES
#undef IS_NAME_CHAR
#undef IS_NMSTRT_CHAR

const struct encoding little2_encoding = {
 { PREFIX(prologTok), PREFIX(contentTok) }, 2
};

#undef PREFIX

#define PREFIX(ident) big2_ ## ident
#define MINBPC 2
/* CHAR_MATCHES is guaranteed to have MINBPC bytes available. */
#define BYTE_TYPE(enc, p) \
 ((p)[0] == 0 ? latin1tab[(unsigned char)(p)[1]] : unicode_byte_type((p)[0], (p)[1]))
#define CHAR_MATCHES(enc, p, c) ((p)[0] == 0 && (p)[1] == c)
#define IS_NAME_CHAR(enc, p, n) \
  UCS2_GET_NAMING(namePages, (unsigned char)p[0], (unsigned char)p[1])
#define IS_NMSTRT_CHAR(enc, p, n) \
  UCS2_GET_NAMING(nmstrtPages, (unsigned char)p[0], (unsigned char)p[1])

#include "xmltok_impl.c"

#undef MINBPC
#undef BYTE_TYPE
#undef CHAR_MATCHES
#undef IS_NAME_CHAR
#undef IS_NMSTRT_CHAR

const struct encoding big2_encoding = {
 { PREFIX(prologTok), PREFIX(contentTok) }, 2
};

#undef PREFIX

static
int initScan(const ENCODING *enc, int state, const char *ptr, const char *end,
	     const char **nextTokPtr)
{
  const ENCODING **encPtr;

  if (ptr == end)
    return XML_TOK_NONE;
  encPtr = ((const INIT_ENCODING *)enc)->encPtr;
  if (ptr + 1 == end) {
    switch ((unsigned char)*ptr) {
    case 0xFE:
    case 0xFF:
    case 0x00:
    case 0x3C:
      return XML_TOK_PARTIAL;
    }
  }
  else {
    switch (((unsigned char)ptr[0] << 8) | (unsigned char)ptr[1]) {
    case 0x003C:
      *encPtr = &big2_encoding;
      return XmlTok(*encPtr, state, ptr, end, nextTokPtr);
    case 0xFEFF:
      *nextTokPtr = ptr + 2;
      *encPtr = &big2_encoding;
      return XML_TOK_BOM;
    case 0x3C00:
      *encPtr = &little2_encoding;
      return XmlTok(*encPtr, state, ptr, end, nextTokPtr);
    case 0xFFFE:
      *nextTokPtr = ptr + 2;
      *encPtr = &little2_encoding;
      return XML_TOK_BOM;
    }
  }
  *encPtr = &utf8_encoding.enc;
  return XmlTok(*encPtr, state, ptr, end, nextTokPtr);
}

static
int initScanProlog(const ENCODING *enc, const char *ptr, const char *end,
		   const char **nextTokPtr)
{
  return initScan(enc, XML_PROLOG_STATE, ptr, end, nextTokPtr);
}

static
int initScanContent(const ENCODING *enc, const char *ptr, const char *end,
		    const char **nextTokPtr)
{
  return initScan(enc, XML_CONTENT_STATE, ptr, end, nextTokPtr);
}

void XmlInitEncoding(INIT_ENCODING *p, const ENCODING **encPtr)
{
  p->initEnc.scanners[XML_PROLOG_STATE] = initScanProlog;
  p->initEnc.scanners[XML_CONTENT_STATE] = initScanContent;
  p->initEnc.minBytesPerChar = 1;
  p->encPtr = encPtr;
  *encPtr = &(p->initEnc);
}
