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
#define XML_TOK_BOM 1     /* Byte order mark */
#define XML_TOK_COMMENT 2
#define XML_TOK_PI 3      /* processing instruction */

/* The following tokens are returned only by XmlPrologTok */
#define XML_TOK_LITERAL 4
#define XML_TOK_PROLOG_CHARS 5
#define XML_TOK_PROLOG_S 6

/* The following token is returned by XmlPrologTok when it detects the end
of the prolog and is also returned by XmlContentTok */

#define XML_TOK_START_TAG 7

/* The following tokens are returned only by XmlContentTok */

#define XML_TOK_END_TAG 8
#define XML_TOK_EMPTY_ELEMENT 9 /* empty element tag <e/> */
#define XML_TOK_DATA_CHARS 10
#define XML_TOK_CDATA_SECTION 11
#define XML_TOK_ENTITY_REF 12
#define XML_TOK_CHAR_REF 13     /* numeric character reference */

#define XML_NSTATES 2
#define XML_PROLOG_STATE 0
#define XML_CONTENT_STATE 1

typedef struct encoding {
  int (*scanners[XML_NSTATES])(const struct encoding *,
			       const char *,
			       const char *,
			       const char **);
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

typedef struct {
  ENCODING initEnc;
  const ENCODING **encPtr;
} INIT_ENCODING;

void XMLTOKAPI XmlInitEncoding(INIT_ENCODING *, const ENCODING **);

#ifdef __cplusplus
}
#endif

#endif /* not XmlTok_INCLUDED */
