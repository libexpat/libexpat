#ifndef XmlTok_INCLUDED
#define XmlTok_INCLUDED 1

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XMLTOKAPI
#define XMLTOKAPI /* as nothing */
#endif

/* The following tokens may be returned by both XmlPrologTok and XmlContentTok */
#define XML_TOK_NONE -3    /* The string to be scanned is empty */
#define XML_TOK_PARTIAL_CHAR -2 /* only part of a multibyte sequence */
#define XML_TOK_PARTIAL -1 /* only part of a token */
#define XML_TOK_INVALID 0

/* The following token is returned by XmlPrologTok when it detects the end
of the prolog and is also returned by XmlContentTok */

#define XML_TOK_START_TAG_WITH_ATTS 1
#define XML_TOK_START_TAG_NO_ATTS 2
#define XML_TOK_EMPTY_ELEMENT_WITH_ATTS 3 /* empty element tag <e/> */
#define XML_TOK_EMPTY_ELEMENT_NO_ATTS 4

/* The following tokens are returned only by XmlContentTok */

#define XML_TOK_END_TAG 5
#define XML_TOK_DATA_CHARS 6
#define XML_TOK_CDATA_SECTION 7
#define XML_TOK_ENTITY_REF 8
#define XML_TOK_CHAR_REF 9     /* numeric character reference */

/* The following tokens may be returned by both XmlPrologTok and XmlContentTok */
#define XML_TOK_PI 10      /* processing instruction */
#define XML_TOK_COMMENT 11
#define XML_TOK_BOM 12     /* Byte order mark */

/* The following tokens are returned only by XmlPrologTok */
#define XML_TOK_LITERAL 13
#define XML_TOK_PROLOG_CHARS 14
#define XML_TOK_PROLOG_S 15

#define XML_NSTATES 2
#define XML_PROLOG_STATE 0
#define XML_CONTENT_STATE 1

typedef struct position {
  /* first line and first column are 0 not 1 */
  unsigned long lineNumber;
  unsigned long columnNumber;
  /* if the last character counted was CR, then an immediately
     following LF should be ignored */
  int ignoreInitialLF;
} POSITION;

typedef struct encoding {
  int (*scanners[XML_NSTATES])(const struct encoding *,
			       const char *,
			       const char *,
			       const char **);
  int (*sameName)(const struct encoding *,
	          const char *, const char *);
  int (*getAtts)(const struct encoding *enc, const char *ptr,
	         int attsMax, const char **atts);
  void (*updatePosition)(const struct encoding *,
			 const char *ptr,
			 const char *end,
			 POSITION *);
  int minBytesPerChar;
} ENCODING;

/*
Scan the string starting at ptr until the end of the next complete token,
but do not scan past eptr.  Return an integer giving the type of token.

Return XML_TOK_NONE when ptr == eptr; nextTokPtr will not be set.

Return XML_TOK_PARTIAL when the string does not contain a complete token;
nextTokPtr will not be set.

Return XML_TOK_INVALID when the string does not start a valid token; nextTokPtr
will be set to point to the character which made the token invalid.

Otherwise the string starts with a valid token; nextTokPtr will be set to point
to the character following the end of that token.

Each data character counts as a single token, but adjacent data characters
may be returned together.  Similarly for characters in the prolog outside
literals, comments and processing instructions.
*/


#define XmlTok(enc, state, ptr, end, nextTokPtr) \
  (((enc)->scanners[state])(enc, ptr, end, nextTokPtr))

#define XmlPrologTok(enc, ptr, end, nextTokPtr) \
   XmlTok(enc, XML_PROLOG_STATE, ptr, end, nextTokPtr)

#define XmlContentTok(enc, ptr, end, nextTokPtr) \
   XmlTok(enc, XML_CONTENT_STATE, ptr, end, nextTokPtr)

#define XmlSameName(enc, ptr1, ptr2) (((enc)->sameName)(enc, ptr1, ptr2))

#define XmlGetAttributes(enc, ptr, attsMax, atts) \
  (((enc)->getAtts)(enc, ptr, attsMax, atts))

#define XmlUpdatePosition(enc, ptr, end, pos) \
  (((enc)->updatePosition)(enc, ptr, end, pos))

typedef struct {
  ENCODING initEnc;
  const ENCODING **encPtr;
} INIT_ENCODING;

void XMLTOKAPI XmlInitEncoding(INIT_ENCODING *, const ENCODING **);

#ifdef __cplusplus
}
#endif

#endif /* not XmlTok_INCLUDED */
