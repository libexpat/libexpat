#ifdef _MSC_VER
#define XMLTOKAPI __declspec(dllexport)
#endif

#include "xmltok.h"

#ifdef UNICODE
typedef wchar_t TCHAR;
#else
typedef char TCHAR;
#endif

#define DIGIT_CASES \
  case '0': case '1': case '2': case '3': case '4': \
  case '5': case '6': case '7': case '8': case '9':

#define HEX_DIGIT_CASES DIGIT_CASES \
  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': \
  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': 

#define S_CASES case ' ': case '\t': case '\r':  case '\n':

/* ptr points to character following "<!-" */

static
int scanComment(const TCHAR *ptr, const TCHAR *end, const TCHAR **nextTokPtr)
{
  if (ptr != end) {
    if (*ptr != '-') {
      *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
    for (++ptr; ptr != end; ptr++) {
      if (*ptr == '-') {
	if (++ptr == end)
	  return XML_TOK_PARTIAL;
	if (*ptr == '-') {
	  if (++ptr == end)
	    return XML_TOK_PARTIAL;
	  if (*ptr != '>') {
	    *nextTokPtr = ptr;
	    return XML_TOK_INVALID;
	  }
	  *nextTokPtr = ptr + 1;
	  return XML_TOK_COMMENT;
	}
      }
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "<!" */

static
int scanDecl(const TCHAR *ptr, const TCHAR *end, const TCHAR **nextTokPtr)
{
  if (ptr != end) {
    if (*ptr == '-')
      return scanComment(ptr + 1, end, nextTokPtr);
    do {
      switch (*ptr) {
      case '\'':
      case '"':
      case '<':
	*nextTokPtr = ptr;
	return XML_TOK_PROLOG_CHARS;
      }
    } while (++ptr != end);
    *nextTokPtr = ptr;
    return XML_TOK_PROLOG_CHARS;
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "<?" */

static
int scanPi(const TCHAR *ptr, const TCHAR *end, const TCHAR **nextTokPtr)
{
  for (; ptr != end; ++ptr) {
    switch (*ptr) {
    case '?':
      if (ptr + 1 == end)
	return XML_TOK_PARTIAL;
      if (ptr[1] == '>') {
	*nextTokPtr = ptr + 2;
	return XML_TOK_PI;
      }
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "<" */

static
int scanStartTag(const TCHAR *ptr, const TCHAR *end, const TCHAR **nextTokPtr)
{
  for (; ptr != end; ++ptr) {
    switch (*ptr) {
    case '>':
      *nextTokPtr = ptr + 1;
      return XML_TOK_START_TAG;
    case '/':
      if (++ptr == end)
	return XML_TOK_PARTIAL;
      if (*ptr != '>') {
	*nextTokPtr = ptr;
	return XML_TOK_INVALID;
      }
      *nextTokPtr = ptr + 1;
      return XML_TOK_EMPTY_ELEMENT;
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "</" */

static
int scanEndTag(const TCHAR *ptr, const TCHAR *end, const TCHAR **nextTokPtr)
{
  for (; ptr != end; ++ptr) {
    switch (*ptr) {
    case '>':
      *nextTokPtr = ptr + 1;
      return XML_TOK_END_TAG;
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "&#X" */

static
int scanHexCharRef(const TCHAR *ptr, const TCHAR *end, const TCHAR **nextTokPtr)
{
  if (ptr != end) {
    switch (*ptr) {
    HEX_DIGIT_CASES
      break;
    default:
      *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
    for (++ptr; ptr != end; ++ptr) {
      switch (*ptr) {
      HEX_DIGIT_CASES
	break;
      case ';':
	*nextTokPtr = ptr + 1;
	return XML_TOK_CHAR_REF;
      default:
	*nextTokPtr = ptr;
	return XML_TOK_INVALID;
      }
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "&#" */

static
int scanCharRef(const TCHAR *ptr, const TCHAR *end, const TCHAR **nextTokPtr)
{
  if (ptr != end) {
    switch (*ptr) {
    case 'x':
    case 'X':
      return scanHexCharRef(ptr + 1, end, nextTokPtr);
    DIGIT_CASES
      break;
    default:
      *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
    for (++ptr; ptr != end; ++ptr) {
      switch (*ptr) {
      DIGIT_CASES
	break;
      case ';':
	*nextTokPtr = ptr + 1;
	return XML_TOK_CHAR_REF;
      default:
	*nextTokPtr = ptr;
	return XML_TOK_INVALID;
      }
    }
  }
  return XML_TOK_PARTIAL;
}

static
int scanEntityRef(const TCHAR *ptr, const TCHAR *end, const TCHAR **nextTokPtr)
{
  for (; ptr != end; ++ptr) {
    switch (*ptr) {
    case ';':
      *nextTokPtr = ptr + 1;
      return XML_TOK_ENTITY_REF;
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "<![" */

static
int scanCdataSection(const TCHAR *ptr, const TCHAR *end, const TCHAR **nextTokPtr)
{
  int i;
  /* CDATA[]]> */
  if (end - ptr < 9)
    return XML_TOK_PARTIAL;
  for (i = 0; i < 6; i++, ptr++) {
    if (*ptr != "CDATA["[i]) {
       *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
  }
  end -= 2;
  for (; ptr != end; ++ptr) {
    if (*ptr == ']') {
      if (ptr[1] == ']' && ptr[2] == '>') {
	*nextTokPtr = ptr + 3;
	return XML_TOK_CDATA_SECTION;
      }
    }
  }
  return XML_TOK_PARTIAL;

}

int XmlContentTok(const TCHAR *ptr, const TCHAR *end, const TCHAR **nextTokPtr)
{
  if (ptr != end) {
    switch (*ptr) {
    case '<':
      {
	++ptr;
	if (ptr == end)
	  return XML_TOK_PARTIAL;
	switch (*ptr) {
	case '!':
	  if (++ptr == end)
	    return XML_TOK_PARTIAL;
	  switch (*ptr) {
	  case '-':
	    return scanComment(ptr + 1, end, nextTokPtr);
	  case '[':
	    return scanCdataSection(ptr + 1, end, nextTokPtr);
	  }
	  *nextTokPtr = ptr;
	  return XML_TOK_INVALID;
	case '?':
	  return scanPi(ptr + 1, end, nextTokPtr);
	case '/':
	  return scanEndTag(ptr + 1, end, nextTokPtr);
	case '>':
	S_CASES
	  *nextTokPtr = ptr;
	  return XML_TOK_INVALID;
	default:
	  return scanStartTag(ptr, end, nextTokPtr);
	}
      }
    case '&':
      {
	++ptr;
	if (ptr == end)
	  return XML_TOK_PARTIAL;
	switch (*ptr) {
	case '#':
	  return scanCharRef(ptr + 1, end, nextTokPtr);
	S_CASES
	case ';':
	  *nextTokPtr = ptr;
	  return XML_TOK_INVALID;
	}
	return scanEntityRef(ptr + 1, end, nextTokPtr);
      }
    default:
      {
	for (++ptr; ptr != end; ++ptr) {
	  switch (*ptr) {
	  case '&':
	  case '<':
	    *nextTokPtr = ptr;
	    return XML_TOK_DATA_CHARS;
	  }
	}
	*nextTokPtr = ptr;
	return XML_TOK_DATA_CHARS;
      }
    }
  }
  return XML_TOK_NONE;
}

int XmlPrologTok(const TCHAR *ptr, const TCHAR *end, const TCHAR **nextTokPtr)
{
  if (ptr != end) {
    switch (*ptr) {
    case '"':
      {
	for (++ptr; ptr != end; ++ptr) {
	  if (*ptr == '"') {
	    *nextTokPtr = ptr + 1;
	    return XML_TOK_LITERAL;
	  }
	}
	return XML_TOK_PARTIAL;
      }
    case '\'':
      {
	for (++ptr; ptr != end; ++ptr) {
	  if (*ptr == '\'') {
	    *nextTokPtr = ptr + 1;
	    return XML_TOK_LITERAL;
	  }
	}
	return XML_TOK_PARTIAL;
      }
    case '<':
      {
	++ptr;
	if (ptr == end)
	  return XML_TOK_PARTIAL;
	switch (*ptr) {
	case '!':
	  return scanDecl(ptr + 1, end, nextTokPtr);
	case '?':
	  return scanPi(ptr + 1, end, nextTokPtr);
	case '/':
	  *nextTokPtr = ptr;
	  return XML_TOK_INVALID;
	default:
	  return XmlContentTok(ptr - 1, end, nextTokPtr);
	}
      }
    default:
      {
	for (++ptr; ptr != end; ++ptr) {
	  switch (*ptr) {
	  case '<':
	  case '"':
	  case '\'':
	    *nextTokPtr = ptr;
	    return XML_TOK_PROLOG_CHARS;
	  }
	}
	*nextTokPtr = ptr;
	return XML_TOK_PROLOG_CHARS;
      }
    }
  }
  return XML_TOK_NONE;
}
