#include <stdlib.h>
#include <string.h>
#include "wfcheck.h"

#ifdef _MSC_VER
#define XMLTOKAPI __declspec(dllimport)
#endif

#include "xmltok.h"

static
int skipProlog(const char **s, const char *end, const char **nextTokP,
	       const ENCODING **enc, const char **doctypeP);
static
void setPosition(const ENCODING *enc,
		 const char *start, const char *end,
		 const char **badPtr, unsigned long *badLine, unsigned long *badCol);

enum WfCheckResult
wfCheck(const char *s, size_t n,
	const char **badPtr, unsigned long *badLine, unsigned long *badCol)
{
  unsigned nElements = 0;
  unsigned nAtts = 0;
  const char *start = s;
  const char *end = s + n;
  const char *next;
  const ENCODING *enc;
  const char *doctype = 0;
  size_t stackSize = 1024;
  size_t level = 0;
  int tok;
  const char **startName = malloc(stackSize * sizeof(char *));
  int attsSize = 1024;
  const char **atts = malloc(attsSize * sizeof(char *));
#define RETURN_CLEANUP(n) return (free((void *)startName), free((void *)atts), (n))
  if (!startName)
    return noMemory;
  tok = skipProlog(&s, end, &next, &enc, &doctype);
  for (;;) {
    switch (tok) {
    case XML_TOK_NONE:
      setPosition(enc, start, s, badPtr, badLine, badCol);
      RETURN_CLEANUP(noElements);
    case XML_TOK_INVALID:
      setPosition(enc, start, next, badPtr, badLine, badCol);
      RETURN_CLEANUP(invalidToken);
    case XML_TOK_PARTIAL:
      setPosition(enc, start, s, badPtr, badLine, badCol);
      RETURN_CLEANUP(unclosedToken);
    case XML_TOK_PARTIAL_CHAR:
      setPosition(enc, start, s, badPtr, badLine, badCol);
      RETURN_CLEANUP(partialChar);
    case XML_TOK_EMPTY_ELEMENT_NO_ATTS:
      nElements++;
      break;
    case XML_TOK_START_TAG_NO_ATTS:
      nElements++;
      if (level == stackSize) {
	startName = realloc((void *)startName, (stackSize *= 2) * sizeof(char *));
	if (!startName) {
	  free((void *)atts);
	  return noMemory;
	}
      }
      startName[level++] = s + enc->minBytesPerChar;
      break;
    case XML_TOK_START_TAG_WITH_ATTS:
      if (level == stackSize) {
	startName = realloc((void *)startName, (stackSize *= 2) * sizeof(char *));
	if (!startName) {
	  free((void *)atts);
	  return noMemory;
	}
      }
      startName[level++] = s + enc->minBytesPerChar;
      /* fall through */
    case XML_TOK_EMPTY_ELEMENT_WITH_ATTS:
      nElements++;
      {
	int i;
	int n = XmlGetAttributes(enc, s, attsSize, atts);
	nAtts += n;
	if (n > attsSize) {
	  attsSize = 2*n;
	  atts = realloc((void *)atts, attsSize * sizeof(char *));
	  if (!atts) {
	    free((void *)startName);
	    return noMemory;
	  }
	  XmlGetAttributes(enc, s, n, atts);
	}
	for (i = 1; i < n; i++) {
	  int j;
	  for (j = 0; j < i; j++) {
	    if (XmlSameName(enc, atts[i], atts[j])) {
	      setPosition(enc, start, atts[i], badPtr, badLine, badCol);
	      RETURN_CLEANUP(duplicateAttribute);
	    }
	  }
	}
      }
      break;
    case XML_TOK_END_TAG:
      --level;
      if (!XmlSameName(enc, startName[level], s + enc->minBytesPerChar * 2)) {
	setPosition(enc, start, s, badPtr, badLine, badCol);
	RETURN_CLEANUP(tagMismatch);
      }
      break;
    }
    s = next;
    if (level == 0) {
      do {
	tok = XmlPrologTok(enc, s, end, &next);
	switch (tok) {
	case XML_TOK_NONE:
	  if (doctype) {
	    setPosition(enc, start, doctype, badPtr, badLine, badCol);
	    RETURN_CLEANUP(wellFormedOutsideDtd);
	  }
	  RETURN_CLEANUP(wellFormed);
	case XML_TOK_PROLOG_S:
	case XML_TOK_COMMENT:
	case XML_TOK_PI:
	  s = next;
	  break;
	default:
	  if (tok > 0) {
	    setPosition(enc, start, s, badPtr, badLine, badCol);
	    RETURN_CLEANUP(junkAfterDocElement);
	  }
	  break;
	}
      } while (tok > 0);
    }
    else
      tok = XmlContentTok(enc, s, end, &next);
  }
  /* not reached */
  return 0;
}

static
int skipProlog(const char **startp, const char *end,
	       const char **nextTokP, const ENCODING **enc,
	       const char **doctypeP)
{
  const char *s = *startp;
  INIT_ENCODING initEnc;
  XmlInitEncoding(&initEnc, enc);
  *doctypeP = 0;
  for (;;) {
    int tok = XmlPrologTok(*enc, s, end, nextTokP);
    switch (tok) {
    case XML_TOK_START_TAG_WITH_ATTS:
    case XML_TOK_START_TAG_NO_ATTS:
    case XML_TOK_EMPTY_ELEMENT_WITH_ATTS:
    case XML_TOK_EMPTY_ELEMENT_NO_ATTS:
    case XML_TOK_INVALID:
    case XML_TOK_NONE:
    case XML_TOK_PARTIAL:
      *startp = s;
      return tok;
    case XML_TOK_DECL_OPEN:
      if (!*doctypeP) {
	if (XmlNameMatchesAscii(*enc, s + 2 * (*enc)->minBytesPerChar, "DOCTYPE"))
	  *doctypeP = s;
	else {
	  *startp = s;
	  return XML_TOK_INVALID;
	}
      }
      break;
    case XML_TOK_PROLOG_S:
    case XML_TOK_LITERAL:
    case XML_TOK_COMMENT:
    case XML_TOK_PI:
      break;
    default:
      if (!*doctypeP) {
	*startp = s;
	return XML_TOK_INVALID;
      }
      break;
    }
    s = *nextTokP;
  }
  /* not reached */
}

static
void setPosition(const ENCODING *enc,
		 const char *start, const char *end,
		 const char **badPtr, unsigned long *badLine, unsigned long *badCol)
{
  POSITION pos;
  memset(&pos, 0, sizeof(POSITION));
  XmlUpdatePosition(enc, start, end, &pos);
  *badPtr = end;
  *badLine = pos.lineNumber;
  *badCol = pos.columnNumber;
}
