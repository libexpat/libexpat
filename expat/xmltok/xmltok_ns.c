const ENCODING *NS(XmlGetUtf8InternalEncoding)()
{
  return &ns(internal_utf8_encoding).enc;
}

const ENCODING *NS(XmlGetUtf16InternalEncoding)()
{
#if BYTE_ORDER == 12
  return &ns(internal_little2_encoding).enc;
#elif BYTE_ORDER == 21
  return &ns(internal_big2_encoding).enc;
#else
  const short n = 1;
  return *(const char *)&n ? &ns(internal_little2_encoding).enc : &ns(internal_big2_encoding).enc;
#endif
}

static
int NS(initScan)(const ENCODING *enc, int state, const char *ptr, const char *end,
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
      *encPtr = &ns(big2_encoding).enc;
      return XmlTok(*encPtr, state, ptr, end, nextTokPtr);
    case 0xFEFF:
      *nextTokPtr = ptr + 2;
      *encPtr = &ns(big2_encoding).enc;
      return XML_TOK_BOM;
    case 0x3C00:
      *encPtr = &ns(little2_encoding).enc;
      return XmlTok(*encPtr, state, ptr, end, nextTokPtr);
    case 0xFFFE:
      *nextTokPtr = ptr + 2;
      *encPtr = &ns(little2_encoding).enc;
      return XML_TOK_BOM;
    }
  }
  *encPtr = (enc->minBytesPerChar == 2
	     ? &ns(big2_encoding).enc
	     : &ns(utf8_encoding).enc); 
  return XmlTok(*encPtr, state, ptr, end, nextTokPtr);
}


static
int NS(initScanProlog)(const ENCODING *enc, const char *ptr, const char *end,
		       const char **nextTokPtr)
{
  return NS(initScan)(enc, XML_PROLOG_STATE, ptr, end, nextTokPtr);
}

static
int NS(initScanContent)(const ENCODING *enc, const char *ptr, const char *end,
		      const char **nextTokPtr)
{
  return NS(initScan)(enc, XML_CONTENT_STATE, ptr, end, nextTokPtr);
}

int NS(XmlInitEncoding)(INIT_ENCODING *p, const ENCODING **encPtr, const char *name)
{
  if (name) {
    if (streqci(name, "ISO-8859-1")) {
      *encPtr = &ns(latin1_encoding).enc;
      return 1;
    }
    if (streqci(name, "UTF-8")) {
      *encPtr = &ns(utf8_encoding).enc;
      return 1;
    }
    if (streqci(name, "US-ASCII")) {
      *encPtr = &ns(ascii_encoding).enc;
      return 1;
    }
    if (streqci(name, "UTF-16BE")) {
      *encPtr = &ns(big2_encoding).enc;
      return 1;
    }
    if (streqci(name, "UTF-16LE")) {
      *encPtr = &ns(little2_encoding).enc;
      return 1;
    }
    if (!streqci(name, "UTF-16"))
      return 0;
    p->initEnc.minBytesPerChar = 2;
  }
  else
    p->initEnc.minBytesPerChar = 1;
  p->initEnc.scanners[XML_PROLOG_STATE] = NS(initScanProlog);
  p->initEnc.scanners[XML_CONTENT_STATE] = NS(initScanContent);
  p->initEnc.updatePosition = initUpdatePosition;
  p->encPtr = encPtr;
  *encPtr = &(p->initEnc);
  return 1;
}


static
const ENCODING *NS(findEncoding)(const ENCODING *enc, const char *ptr, const char *end)
{
#define ENCODING_MAX 128
  char buf[ENCODING_MAX];
  char *p = buf;
  int i;
  XmlUtf8Convert(enc, &ptr, end, &p, p + ENCODING_MAX - 1);
  if (ptr != end)
    return 0;
  *p = 0;
  for (i = 0; buf[i]; i++) {
    if ('a' <= buf[i] && buf[i] <= 'z')
      buf[i] +=  'A' - 'a';
  }
  if (streqci(buf, "UTF-8"))
    return &ns(utf8_encoding).enc;
  if (streqci(buf, "ISO-8859-1"))
    return &ns(latin1_encoding).enc;
  if (streqci(buf, "US-ASCII"))
    return &ns(ascii_encoding).enc;
  if (streqci(buf, "UTF-16")) {
    static const unsigned short n = 1;
    if (enc->minBytesPerChar == 2)
      return enc;
    return &ns(big2_encoding).enc;
  }
  return 0;  
}

int NS(XmlParseXmlDecl)(int isGeneralTextEntity,
			const ENCODING *enc,
			const char *ptr,
			const char *end,
			const char **badPtr,
			const char **versionPtr,
			const char **encodingName,
			const ENCODING **encoding,
			int *standalone)
{
  return doParseXmlDecl(NS(findEncoding),
			isGeneralTextEntity,
			enc,
			ptr,
			end,
			badPtr,
			versionPtr,
			encodingName,
			encoding,
			standalone);
}
