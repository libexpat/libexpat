#ifndef XmlTok_INCLUDED
#define XmlTok_INCLUDED 1

#ifndef XMLTOKAPI
#define XMLTOKAPI /* as nothing */
#endif

#include <stddef.h>

/* The following tokens may be returned by both XmlPrologTok and XmlContentTok */
#define XML_TOK_NONE -2    /* The string to be scanned is empty */
#define XML_TOK_PARTIAL -1
#define XML_TOK_INVALID 0
#define XML_TOK_COMMENT 1
#define XML_TOK_PI 2      /* processing instruction */

/* The following tokens are returned only by XmlPrologTok */
#define XML_TOK_LITERAL 3
#define XML_TOK_PROLOG_CHARS 4

/* The following token is returned by XmlPrologTok when it detects the end
of the prolog and is also returned by XmlContentTok */

#define XML_TOK_START_TAG 5

/* The following tokens are returned only by XmlContentTok */

#define XML_TOK_END_TAG 6
#define XML_TOK_EMPTY_ELEMENT 7 /* empty element tag <e/> */
#define XML_TOK_DATA_CHARS 8
#define XML_TOK_CDATA_SECTION 9
#define XML_TOK_ENTITY_REF 10
#define XML_TOK_CHAR_REF 11     /* numeric character reference */

#ifdef __cplusplus
extern "C" {
#endif

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

int XMLTOKAPI XmlPrologTokA(const char *ptr,
			    const char *eptr,
			    const char **nextTokPtr);
int XMLTOKAPI XmlContentTokA(const char *ptr,
			     const char *eptr,
			     const char **nextTokPtr);

int XMLTOKAPI XmlPrologTokW(const wchar_t *ptr,
			    const wchar_t *eptr,
			    const wchar_t **nextTokPtr);
int XMLTOKAPI XmlContentTokW(const wchar_t *ptr,
			     const wchar_t *eptr,
			     const wchar_t **nextTokPtr);

#ifdef __cplusplus
}
#endif

#ifdef UNICODE
#define XmlPrologTok XmlPrologTokW
#define XmlContentTok XmlContentTokW
#else
#define XmlPrologTok XmlPrologTokA
#define XmlContentTok XmlContentTokA
#endif

#endif /* not XmlTok_INCLUDED */
