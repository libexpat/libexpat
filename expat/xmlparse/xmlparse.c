/* FIXME Normalize tokenized attribute values. */

#include "xmlparse.h"
#include "xmltok.h"
#include "xmlrole.h"
#include "hashtable.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *name;
  const char *textPtr;
  size_t textLen;
  const char *systemId;
  const char *publicId;
  const char *notation;
  char open;
  char magic;
} ENTITY;

#define INIT_BLOCK_SIZE 1024

typedef struct block {
  struct block *next;
  int size;
  char s[1];
} BLOCK;

typedef struct {
  BLOCK *blocks;
  BLOCK *freeBlocks;
  const char *end;
  char *ptr;
  char *start;
} STRING_POOL;

/* The byte before the name is a scratch byte used to determine whether
an attribute has been specified. */
typedef struct {
  char *name;
} ATTRIBUTE_ID;

typedef struct {
  const ATTRIBUTE_ID *id;
  const char *value;
} DEFAULT_ATTRIBUTE;

typedef struct {
  const char *name;
  int nDefaultAtts;
  int allocDefaultAtts;
  DEFAULT_ATTRIBUTE *defaultAtts;
} ELEMENT_TYPE;

typedef struct {
  HASH_TABLE generalEntities;
  HASH_TABLE paramEntities;
  HASH_TABLE elementTypes;
  HASH_TABLE attributeIds;
  STRING_POOL pool;
  int containsRef;
  int standalone;
} DTD;

typedef enum XML_Error Processor(XML_Parser parser,
				 const char *start,
				 const char *end,
				 const char **endPtr);

static Processor prologProcessor;
static Processor contentProcessor;
static Processor epilogProcessor;

static
int doContent(XML_Parser parser,
	      int startTagLevel,
	      const ENCODING *enc,
	      const char *start,
	      const char *end,
	      const char **endPtr);

static enum XML_Error
checkGeneralTextEntity(XML_Parser parser,
		       const char *s, const char *end,
		       const char **nextPtr,
		       const ENCODING **enc);
static enum XML_Error storeAtts(XML_Parser parser, const ENCODING *, const char *tagName, const char *s);
static int
addDefaultAttribute(ELEMENT_TYPE *type, ATTRIBUTE_ID *, const char *value);
static enum XML_Error
storeAttributeValue(XML_Parser parser, const ENCODING *, const char *, const char *,
		    STRING_POOL *);
static ATTRIBUTE_ID *
getAttributeId(XML_Parser parser, const ENCODING *enc, const char *start, const char *end);
static enum XML_Error
storeEntityValue(XML_Parser parser, const char *start, const char *end);
static int
reportProcessingInstruction(XML_Parser parser, const ENCODING *enc, const char *start, const char *end);

static
const char *pushTag(XML_Parser parser, const ENCODING *enc, const char *rawName);

static int dtdInit(DTD *);
static void dtdDestroy(DTD *);
static void poolInit(STRING_POOL *);
static void poolClear(STRING_POOL *);
static void poolDestroy(STRING_POOL *);
static const char *poolAppend(STRING_POOL *pool, const ENCODING *enc,
			      const char *ptr, const char *end);
static const char *poolStoreString(STRING_POOL *pool, const ENCODING *enc,
			    const char *ptr, const char *end);
static int poolGrow(STRING_POOL *pool);

#define poolStart(pool) ((pool)->start)
#define poolEnd(pool) ((pool)->ptr)
#define poolLength(pool) ((pool)->ptr - (pool)->start)
#define poolDiscard(pool) ((pool)->ptr = (pool)->start)
#define poolFinish(pool) ((pool)->start = (pool)->ptr)
#define poolAppendByte(pool, c) \
  (((pool)->ptr == (pool)->end && !poolGrow(pool)) \
   ? 0 \
   : ((*((pool)->ptr)++ = c), 1))

typedef struct {
  char *buffer;
  /* first character to be parsed */
  const char *bufferPtr;
  /* past last character to be parsed */
  char *bufferEnd;
  /* allocated end of buffer */
  const char *bufferLim;
  long bufferEndByteIndex;
  void *userData;
  XML_StartElementHandler startElementHandler;
  XML_EndElementHandler endElementHandler;
  XML_CharacterDataHandler characterDataHandler;
  XML_ProcessingInstructionHandler processingInstructionHandler;
  const ENCODING *encoding;
  INIT_ENCODING initEncoding;
  PROLOG_STATE prologState;
  Processor *processor;
  enum XML_Error errorCode;
  const char *errorPtr;
  int tagLevel;
  ENTITY *declEntity;
  ELEMENT_TYPE *declElementType;
  ATTRIBUTE_ID *declAttributeId;
  DTD dtd;
  char *tagStack;
  char *tagStackPtr;
  const char *tagStackEnd;
  int attsSize;
  ATTRIBUTE *atts;
  POSITION position;
  long errorByteIndex;
  STRING_POOL tempPool;
  char *groupConnector;
  size_t groupSize;
} Parser;

#define userData (((Parser *)parser)->userData)
#define startElementHandler (((Parser *)parser)->startElementHandler)
#define endElementHandler (((Parser *)parser)->endElementHandler)
#define characterDataHandler (((Parser *)parser)->characterDataHandler)
#define processingInstructionHandler (((Parser *)parser)->processingInstructionHandler)
#define encoding (((Parser *)parser)->encoding)
#define initEncoding (((Parser *)parser)->initEncoding)
#define prologState (((Parser *)parser)->prologState)
#define processor (((Parser *)parser)->processor)
#define errorCode (((Parser *)parser)->errorCode)
#define errorPtr (((Parser *)parser)->errorPtr)
#define errorByteIndex (((Parser *)parser)->errorByteIndex)
#define position (((Parser *)parser)->position)
#define tagLevel (((Parser *)parser)->tagLevel)
#define buffer (((Parser *)parser)->buffer)
#define bufferPtr (((Parser *)parser)->bufferPtr)
#define bufferEnd (((Parser *)parser)->bufferEnd)
#define bufferEndByteIndex (((Parser *)parser)->bufferEndByteIndex)
#define bufferLim (((Parser *)parser)->bufferLim)
#define dtd (((Parser *)parser)->dtd)
#define declEntity (((Parser *)parser)->declEntity)
#define declElementType (((Parser *)parser)->declElementType)
#define declAttributeId (((Parser *)parser)->declAttributeId)
#define tagStackEnd (((Parser *)parser)->tagStackEnd)
#define tagStackPtr (((Parser *)parser)->tagStackPtr)
#define tagStack (((Parser *)parser)->tagStack)
#define atts (((Parser *)parser)->atts)
#define attsSize (((Parser *)parser)->attsSize)
#define tempPool (((Parser *)parser)->tempPool)
#define groupConnector (((Parser *)parser)->groupConnector)
#define groupSize (((Parser *)parser)->groupSize)

XML_Parser XML_ParserCreate(const char *encodingName)
{
  XML_Parser parser = malloc(sizeof(Parser));
  if (!parser)
    return parser;
  processor = prologProcessor;
  XmlInitEncoding(&initEncoding, &encoding);
  XmlPrologStateInit(&prologState);
  userData = 0;
  startElementHandler = 0;
  endElementHandler = 0;
  characterDataHandler = 0;
  processingInstructionHandler = 0;
  buffer = 0;
  bufferPtr = 0;
  bufferEnd = 0;
  bufferEndByteIndex = 0;
  bufferLim = 0;
  declElementType = 0;
  declAttributeId = 0;
  declEntity = 0;
  memset(&position, 0, sizeof(POSITION));
  errorCode = XML_ERROR_NONE;
  errorByteIndex = 0;
  errorPtr = 0;
  tagLevel = 0;
  tagStack = malloc(1024);
  tagStackPtr = tagStack;
  attsSize = 1024;
  atts = malloc(attsSize * sizeof(ATTRIBUTE));
  groupSize = 0;
  groupConnector = 0;
  poolInit(&tempPool);
  if (!dtdInit(&dtd) || !atts || !tagStack) {
    XML_ParserFree(parser);
    return 0;
  }
  tagStackEnd = tagStack + 1024;
  *tagStackPtr++ = '\0';
  return parser;
}

void XML_ParserFree(XML_Parser parser)
{
  poolDestroy(&tempPool);
  dtdDestroy(&dtd);
  free((void *)tagStack);
  free((void *)atts);
  free(groupConnector);
  free(buffer);
  free(parser);
}

void XML_SetUserData(XML_Parser parser, void *p)
{
  userData = p;
}

void XML_SetElementHandler(XML_Parser parser,
			   XML_StartElementHandler start,
			   XML_EndElementHandler end)
{
  startElementHandler = start;
  endElementHandler = end;
}

void XML_SetCharacterDataHandler(XML_Parser parser,
				 XML_CharacterDataHandler handler)
{
  characterDataHandler = handler;
}

void XML_SetProcessingInstructionHandler(XML_Parser parser,
					 XML_ProcessingInstructionHandler handler)
{
  processingInstructionHandler = handler;
}

int XML_Parse(XML_Parser parser, const char *s, size_t len, int isFinal)
{
  bufferEndByteIndex += len;
  if (len == 0) {
    if (!isFinal)
      return 1;
    errorCode = processor(parser, bufferPtr, bufferEnd, 0);
    return errorCode == XML_ERROR_NONE;
  }
  else if (bufferPtr == bufferEnd) {
    const char *end;
    int nLeftOver;
    if (isFinal) {
      errorCode = processor(parser, s, s + len, 0);
      if (errorCode == XML_ERROR_NONE)
	return 1;
      if (errorPtr) {
	errorByteIndex = bufferEndByteIndex - (s + len - errorPtr);
	XmlUpdatePosition(encoding, s, errorPtr, &position);
      }
      return 0;
    }
    errorCode = processor(parser, s, s + len, &end);
    if (errorCode != XML_ERROR_NONE) {
      if (errorPtr) {
	errorByteIndex = bufferEndByteIndex - (s + len - errorPtr);
	XmlUpdatePosition(encoding, s, errorPtr, &position);
      }
      return 0;
    }
    XmlUpdatePosition(encoding, s, end, &position);
    nLeftOver = s + len - end;
    if (nLeftOver) {
      if (buffer == 0 || nLeftOver > bufferLim - buffer) {
	/* FIXME avoid integer overflow */
	buffer = realloc(buffer, len * 2);
	if (!buffer) {
	  errorCode = XML_ERROR_NO_MEMORY;
	  return 0;
	}
	bufferLim = buffer + len * 2;
      }
      memcpy(buffer, end, nLeftOver);
      bufferPtr = buffer;
      bufferEnd = buffer + nLeftOver;
    }
    return 1;
  }
  else {
    memcpy(XML_GetBuffer(parser, len), s, len);
    return XML_ParseBuffer(parser, len, isFinal);
  }
}

int XML_ParseBuffer(XML_Parser parser, size_t len, int isFinal)
{
  const char *start = bufferPtr;
  bufferEnd += len;
  errorCode = processor(parser, bufferPtr, bufferEnd,
			isFinal ? (const char **)0 : &bufferPtr);
  if (errorCode == XML_ERROR_NONE) {
    if (!isFinal)
      XmlUpdatePosition(encoding, start, bufferPtr, &position);
    return 1;
  }
  else {
    if (errorPtr) {
      errorByteIndex = bufferEndByteIndex - (bufferEnd - errorPtr);
      XmlUpdatePosition(encoding, start, errorPtr, &position);
    }
    return 0;
  }
}

void *XML_GetBuffer(XML_Parser parser, size_t len)
{
  if (len > bufferLim - bufferEnd) {
    /* FIXME avoid integer overflow */
    int neededSize = len + (bufferEnd - bufferPtr);
    if (neededSize  <= bufferLim - buffer) {
      memmove(buffer, bufferPtr, bufferEnd - bufferPtr);
      bufferEnd = buffer + (bufferEnd - bufferPtr);
      bufferPtr = buffer;
    }
    else {
      char *newBuf;
      size_t bufferSize = bufferLim - bufferPtr;
      if (bufferSize == 0)
	bufferSize = 1024;
      do {
	bufferSize *= 2;
      } while (bufferSize < neededSize);
      newBuf = malloc(bufferSize);
      if (newBuf == 0) {
	errorCode = XML_ERROR_NO_MEMORY;
	return 0;
      }
      bufferLim = newBuf + bufferSize;
      memcpy(newBuf, bufferPtr, bufferEnd - bufferPtr);
      bufferEnd = newBuf + (bufferEnd - bufferPtr);
      bufferPtr = buffer = newBuf;
    }
  }
  return bufferEnd;
}

int XML_GetErrorCode(XML_Parser parser)
{
  return errorCode;
}

int XML_GetErrorLineNumber(XML_Parser parser)
{
  return position.lineNumber;
}

int XML_GetErrorColumnNumber(XML_Parser parser)
{
  return position.columnNumber;
}

long XML_GetErrorByteIndex(XML_Parser parser)
{
  return errorByteIndex;
}

const char *XML_ErrorString(int code)
{
  static const char *message[] = {
    0,
    "out of memory",
    "syntax error",
    "no element found",
    "not well-formed",
    "unclosed token",
    "unclosed token",
    "mismatched tag",
    "duplicate attribute",
    "junk after document element",
    "parameter entity reference not allowed within declaration in internal subset",
    "undefined entity",
    "recursive entity reference",
    "asynchronous entity",
    "reference to invalid character number",
    "reference to binary entity",
    "reference to external entity in attribute",
    "xml processing instruction not at start of external entity",
    "unknown encoding",
    "encoding specified in XML declaration is incorrect"
  };
  if (code > 0 && code < sizeof(message)/sizeof(message[0]))
    return message[code];
  return 0;
}

static
enum XML_ERROR contentProcessor(XML_Parser parser,
				const char *start,
				const char *end,
				const char **endPtr)
{
  return doContent(parser, 0, encoding, start, end, endPtr);
}

static enum XML_Error
doContent(XML_Parser parser,
	  int startTagLevel,
	  const ENCODING *enc,
	  const char *s,
	  const char *end,
	  const char **nextPtr)
{
  static const char *nullPtr = 0;
  const ENCODING *utf8 = XmlGetInternalEncoding(XML_UTF8_ENCODING);
  for (;;) {
    const char *next;
    int tok = XmlContentTok(enc, s, end, &next);
    switch (tok) {
    case XML_TOK_TRAILING_CR:
    case XML_TOK_NONE:
      if (nextPtr) {
	*nextPtr = s;
	return XML_ERROR_NONE;
      }
      if (startTagLevel > 0) {
	if (tagLevel != startTagLevel) {
	  errorPtr = s;
	  return XML_ERROR_ASYNC_ENTITY;
        }
	return XML_ERROR_NONE;
      }
      errorPtr = s;
      return XML_ERROR_NO_ELEMENTS;
    case XML_TOK_INVALID:
      errorPtr = next;
      return XML_ERROR_INVALID_TOKEN;
    case XML_TOK_PARTIAL:
      if (nextPtr) {
	*nextPtr = s;
	return XML_ERROR_NONE;
      }
      errorPtr = s;
      return XML_ERROR_UNCLOSED_TOKEN;
    case XML_TOK_PARTIAL_CHAR:
      if (nextPtr) {
	*nextPtr = s;
	return XML_ERROR_NONE;
      }
      errorPtr = s;
      return XML_ERROR_PARTIAL_CHAR;
    case XML_TOK_ENTITY_REF:
      {
	const char *name = poolStoreString(&dtd.pool, enc,
					   s + enc->minBytesPerChar,
					   next - enc->minBytesPerChar);
	ENTITY *entity;
	if (!name)
	  return XML_ERROR_NO_MEMORY;
	entity = (ENTITY *)lookup(&dtd.generalEntities, name, 0);
	poolDiscard(&dtd.pool);
	if (!entity) {
	  if (!dtd.containsRef || dtd.standalone) {
	    errorPtr = s;
	    return XML_ERROR_UNDEFINED_ENTITY;
	  }
	  break;
	}
	if (entity->magic) {
	  if (characterDataHandler)
	    characterDataHandler(userData, entity->textPtr, entity->textLen);
	  break;
	}
	if (entity->open) {
	  errorPtr = s;
	  return XML_ERROR_RECURSIVE_ENTITY_REF;
	}
	if (entity->notation) {
	  errorPtr = s;
	  return XML_ERROR_BINARY_ENTITY_REF;
	}
	if (entity) {
	  if (entity->textPtr) {
	    enum XML_Error result;
	    entity->open = 1;
	    result = doContent(parser,
			       tagLevel,
			       utf8,
			       entity->textPtr,
			       entity->textPtr + entity->textLen,
			       0);
	    entity->open = 0;
	    if (result) {
	      errorPtr = s;
	      return result;
	    }
	  }
	}
	break;
      }
    case XML_TOK_START_TAG_WITH_ATTS:
      {
	const char *name;
	name = pushTag(parser, enc, s + enc->minBytesPerChar);
	if (!name)
	  return XML_ERROR_NO_MEMORY;
	++tagLevel;
	if (startElementHandler) {
	  enum XML_Error result = storeAtts(parser, enc, name, s);
	  if (result)
	    return result;
	  startElementHandler(userData, name, (const char **)atts);
	  poolClear(&tempPool);
	}
	else {
	  enum XML_Error result = storeAtts(parser, enc, 0, s);
	  if (result)
	    return result;
	}
	break;
      }
    case XML_TOK_START_TAG_NO_ATTS:
      {
	const char *name = pushTag(parser, enc, s + enc->minBytesPerChar);
	++tagLevel;
	if (!name)
	  return XML_ERROR_NO_MEMORY;
	if (startElementHandler) {
	  enum XML_Error result = storeAtts(parser, enc, name, s);
	  if (result)
	    return result;
	  startElementHandler(userData, name, (const char **)atts);
	}
	break;
      }
    case XML_TOK_EMPTY_ELEMENT_WITH_ATTS:
      if (!startElementHandler) {
	enum XML_Error result = storeAtts(parser, enc, 0, s);
	if (result)
	  return result;
      }
      /* fall through */
    case XML_TOK_EMPTY_ELEMENT_NO_ATTS:
      if (startElementHandler || endElementHandler) {
	const char *rawName = s + enc->minBytesPerChar;
	const char *name = poolStoreString(&tempPool, enc, rawName,
					   rawName
					   + XmlNameLength(enc, rawName));
	if (!name)
	  return XML_ERROR_NO_MEMORY;
	poolFinish(&tempPool);
	if (startElementHandler) {
	  enum XML_Error result = storeAtts(parser, enc, name, s);
	  if (result)
	    return result;
	  startElementHandler(userData, name, (const char **)atts);
	}
	if (endElementHandler)
	  endElementHandler(userData, name);
	poolClear(&tempPool);
      }
      if (tagLevel == 0)
	return epilogProcessor(parser, next, end, nextPtr);
      break;
    case XML_TOK_END_TAG:
      if (tagLevel == startTagLevel) {
        errorPtr = s;
        return XML_ERROR_ASYNC_ENTITY;
      }
      else {
	const char *rawNameStart = s + enc->minBytesPerChar * 2;
	const char *nameEnd;
	const char *name = poolStoreString(&tempPool, enc, rawNameStart,
					   rawNameStart
					   + XmlNameLength(enc, rawNameStart));
	if (!name)
	  return XML_ERROR_NO_MEMORY;
	nameEnd = poolEnd(&tempPool);
	for (;;) {
	  --nameEnd;
	  --tagStackPtr;
	  if (nameEnd == name) {
	    if (tagStackPtr[-1] == '\0')
	      break;
	    return XML_ERROR_TAG_MISMATCH;
	  }
	  if (*nameEnd != *tagStackPtr)
	    return XML_ERROR_TAG_MISMATCH;
	}
	--tagLevel;
	if (endElementHandler)
	  endElementHandler(userData, name);
	poolDiscard(&tempPool);
	if (tagLevel == 0)
	  return epilogProcessor(parser, next, end, nextPtr);
      }
      break;
    case XML_TOK_CHAR_REF:
      {
	int n = XmlCharRefNumber(enc, s);
	if (n < 0) {
	  errorPtr = s;
	  return XML_ERROR_BAD_CHAR_REF;
	}
	if (characterDataHandler) {
	  char buf[XML_MAX_BYTES_PER_CHAR];
	  characterDataHandler(userData, buf, XmlEncode(utf8, n, buf));
	}
      }
      break;
    case XML_TOK_XML_DECL:
      errorPtr = s;
      return XML_ERROR_MISPLACED_XML_PI;
    case XML_TOK_DATA_NEWLINE:
      if (characterDataHandler) {
	char c = '\n';
	characterDataHandler(userData, &c, 1);
      }
      break;
    case XML_TOK_CDATA_SECTION:
      if (characterDataHandler) {
	if (!poolAppend(&tempPool,
			enc,
			s + enc->minBytesPerChar * 9,
			next - enc->minBytesPerChar * 3))
	  return XML_ERROR_NO_MEMORY;
	characterDataHandler(userData, poolStart(&tempPool), poolLength(&tempPool));
	poolDiscard(&tempPool);
      }
      break;
    case XML_TOK_DATA_CHARS:
      if (characterDataHandler) {
	/* FIXME Do this efficiently */
	if (!poolAppend(&tempPool, enc, s, next))
	  return XML_ERROR_NO_MEMORY;
	characterDataHandler(userData, poolStart(&tempPool), poolLength(&tempPool));
	poolDiscard(&tempPool);
      }
      break;
    case XML_TOK_PI:
      if (!reportProcessingInstruction(parser, enc, s, next))
	return XML_ERROR_NO_MEMORY;
      break;
    }
    s = next;
  }
  /* not reached */
}

static
const char *pushTag(XML_Parser parser, const ENCODING *enc, const char *rawName)
{
  const char *name = tagStackPtr;
  const char *rawNameEnd = rawName + XmlNameLength(enc, rawName);
  for (;;) {
    char *newStack;
    size_t newSize;
    XmlConvert(enc, XML_UTF8_ENCODING,
	       &rawName, rawNameEnd,
	       &tagStackPtr, tagStackEnd - 1);
    if (rawName == rawNameEnd)
      break;
    newSize = (tagStackEnd - tagStack) >> 1;
    newStack = realloc(tagStack, newSize);
    if (!newStack)
      return 0;
    tagStackEnd = newStack + newSize;
    tagStackPtr = newStack + (tagStackPtr - tagStack);
    name = newStack + (name - tagStack);
    tagStack = newStack;
  }
  *tagStackPtr++ = '\0';
  return name;
}

/* If tagName is non-null, build a real list of attributes,
otherwise just check the attributes for well-formedness. */

static enum XML_Error storeAtts(XML_Parser parser, const ENCODING *enc,
				const char *tagName, const char *s)
{
  ELEMENT_TYPE *elementType = 0;
  int nDefaultAtts = 0;
  const char **appAtts = (const char **)atts;
  int i;
  int n;

  if (tagName) {
    elementType = (ELEMENT_TYPE *)lookup(&dtd.elementTypes, tagName, 0);
    if (elementType)
      nDefaultAtts = elementType->nDefaultAtts;
  }
  
  n = XmlGetAttributes(enc, s, attsSize, atts);
  if (n + nDefaultAtts > attsSize) {
    attsSize = 2*n;
    atts = realloc((void *)atts, attsSize * sizeof(ATTRIBUTE));
    if (!atts)
      return XML_ERROR_NO_MEMORY;
    if (n > attsSize)
      XmlGetAttributes(enc, s, n, atts);
  }
  for (i = 0; i < n; i++) {
    ATTRIBUTE_ID *attId = getAttributeId(parser, enc, atts[i].name,
					  atts[i].name
					  + XmlNameLength(enc, atts[i].name));
    if (!attId)
      return XML_ERROR_NO_MEMORY;
    if (attId->name[-1]) {
      errorPtr = atts[i].name;
      return XML_ERROR_DUPLICATE_ATTRIBUTE;
    }
    attId->name[-1] = 1;
    appAtts[i << 1] = attId->name;
    if (!atts[i].normalized) {
      enum XML_Error result
	= storeAttributeValue(parser, enc,
			      atts[i].valuePtr,
			      atts[i].valueEnd,
			      &tempPool);
      if (result)
	return result;
      if (!poolAppendByte(&tempPool, '\0'))
	return XML_ERROR_NO_MEMORY;
      if (tagName) {
	appAtts[(i << 1) + 1] = poolStart(&tempPool);
	poolFinish(&tempPool);
      }
      else
	poolDiscard(&tempPool);
    }
    else if (tagName) {
      appAtts[(i << 1) + 1] = poolStoreString(&tempPool, enc, atts[i].valuePtr, atts[i].valueEnd);
      if (appAtts[(i << 1) + 1] == 0)
	return XML_ERROR_NO_MEMORY;
      poolFinish(&tempPool);
    }
  }
  if (tagName) {
    int j;
    for (j = 0; j < nDefaultAtts; j++) {
      const DEFAULT_ATTRIBUTE *da = elementType->defaultAtts + j;
      if (!da->id->name[-1]) {
	da->id->name[-1] = 1;
	appAtts[i << 1] = da->id->name;
	appAtts[(i << 1) + 1] = da->value;
	i++;
      }
    }
    appAtts[i << 1] = 0;
  }
  for (i = 0; i < n; i++)
    ((char *)appAtts[i << 1])[-1] = 0;
  return XML_ERROR_NONE;
}

static enum XML_Error
prologProcessor(XML_Parser parser,
		const char *s,
		const char *end,
		const char **nextPtr)
{
  for (;;) {
    const char *next;
    int tok = XmlPrologTok(encoding, s, end, &next);
    if (tok <= 0) {
      if (nextPtr != 0 && tok != XML_TOK_INVALID) {
	*nextPtr = s;
	return XML_ERROR_NONE;
      }
      switch (tok) {
      case XML_TOK_INVALID:
	errorPtr = next;
	return XML_ERROR_INVALID_TOKEN;
      case XML_TOK_NONE:
	return XML_ERROR_NO_ELEMENTS;
      case XML_TOK_PARTIAL:
	return XML_ERROR_UNCLOSED_TOKEN;
      case XML_TOK_PARTIAL_CHAR:
	return XML_ERROR_PARTIAL_CHAR;
      case XML_TOK_TRAILING_CR:
	errorPtr = s + encoding->minBytesPerChar;
	return XML_ERROR_NO_ELEMENTS;
      default:
	abort();
      }
    }
    switch (XmlTokenRole(&prologState, tok, s, next, encoding)) {
    case XML_ROLE_XML_DECL:
      {
	const char *encodingName = 0;
	const ENCODING *newEncoding = 0;
	const char *version;
	int standalone = -1;
	if (!XmlParseXmlDecl(0,
			     encoding,
			     s,
			     next,
			     &errorPtr,
			     &version,
			     &encodingName,
			     &newEncoding,
			     &standalone))
	  return XML_ERROR_SYNTAX;
	if (newEncoding) {
	  if (newEncoding->minBytesPerChar != encoding->minBytesPerChar) {
	    errorPtr = encodingName;
	    return XML_ERROR_INCORRECT_ENCODING;
	  }
	  encoding = newEncoding;
	}
	else if (encodingName) {
	  errorPtr = encodingName;
	  return XML_ERROR_UNKNOWN_ENCODING;
	}
	if (standalone == 1)
	  dtd.standalone = 1;
	break;
      }
    case XML_ROLE_DOCTYPE_SYSTEM_ID:
      dtd.containsRef = 1;
      break;
    case XML_ROLE_DOCTYPE_PUBLIC_ID:
    case XML_ROLE_ENTITY_PUBLIC_ID:
    case XML_ROLE_NOTATION_PUBLIC_ID:
      if (!XmlIsPublicId(encoding, s, next, &errorPtr))
	return XML_ERROR_SYNTAX;
      break;
    case XML_ROLE_INSTANCE_START:
      processor = contentProcessor;
      return contentProcessor(parser, s, end, nextPtr);
    case XML_ROLE_ATTLIST_ELEMENT_NAME:
      {
	const char *name = poolStoreString(&dtd.pool, encoding, s, next);
	if (!name)
	  return XML_ERROR_NO_MEMORY;
	declElementType = (ELEMENT_TYPE *)lookup(&dtd.elementTypes, name, sizeof(ELEMENT_TYPE));
	if (!declElementType)
	  return XML_ERROR_NO_MEMORY;
	if (declElementType->name != name)
	  poolDiscard(&dtd.pool);
	else
	  poolFinish(&dtd.pool);
	break;
      }
    case XML_ROLE_ATTRIBUTE_NAME:
      declAttributeId = getAttributeId(parser, encoding, s, next);
      if (!declAttributeId)
	return XML_ERROR_NO_MEMORY;
      break;
    case XML_ROLE_DEFAULT_ATTRIBUTE_VALUE:
    case XML_ROLE_FIXED_ATTRIBUTE_VALUE:
      {
	enum XML_Error result
	  = storeAttributeValue(parser, encoding, s + encoding->minBytesPerChar,
			        next - encoding->minBytesPerChar,
			        &dtd.pool);
	if (result)
	  return result;
	if (!poolAppendByte(&dtd.pool, 0))
	  return XML_ERROR_NO_MEMORY;
	if (!addDefaultAttribute(declElementType, declAttributeId, poolStart(&dtd.pool)))
	  return XML_ERROR_NO_MEMORY;
	poolFinish(&dtd.pool);
	break;
      }
    case XML_ROLE_ENTITY_VALUE:
      {
	enum XML_Error result = storeEntityValue(parser, s, next);
	if (result != XML_ERROR_NONE)
	  return result;
      }
      break;
    case XML_ROLE_ENTITY_SYSTEM_ID:
      if (declEntity) {
	declEntity->systemId = poolStoreString(&dtd.pool, encoding,
	                                       s + encoding->minBytesPerChar,
	  				       next - encoding->minBytesPerChar);
	if (!declEntity->systemId)
	  return XML_ERROR_NO_MEMORY;
	poolFinish(&dtd.pool);
      }
      break;
    case XML_ROLE_PARAM_ENTITY_REF:
      {
	const char *name = poolStoreString(&dtd.pool, encoding,
					   s + encoding->minBytesPerChar,
					   next - encoding->minBytesPerChar);
	ENTITY *entity;
	if (!name)
	  return XML_ERROR_NO_MEMORY;
	entity = (ENTITY *)lookup(&dtd.paramEntities, name, 0);
	poolDiscard(&dtd.pool);
	if (!entity) {
	  if (!dtd.containsRef || dtd.standalone) {
	    errorPtr = s;
	    return XML_ERROR_UNDEFINED_ENTITY;
	  }
	}
      }
      break;
    case XML_ROLE_ENTITY_NOTATION_NAME:
      if (declEntity) {
	declEntity->notation = poolStoreString(&dtd.pool, encoding, s, next);
	if (!declEntity->notation)
	  return XML_ERROR_NO_MEMORY;
	poolFinish(&dtd.pool);
      }
      break;
    case XML_ROLE_GENERAL_ENTITY_NAME:
      {
	const char *name = poolStoreString(&dtd.pool, encoding, s, next);
	if (!name)
	  return XML_ERROR_NO_MEMORY;
	declEntity = (ENTITY *)lookup(&dtd.generalEntities, name, sizeof(ENTITY));
	if (!declEntity)
	  return XML_ERROR_NO_MEMORY;
	if (declEntity->name != name) {
	  poolDiscard(&dtd.pool);
	  declEntity = 0;
	}
	else
	  poolFinish(&dtd.pool);
      }
      break;
    case XML_ROLE_PARAM_ENTITY_NAME:
      {
	const char *name = poolStoreString(&dtd.pool, encoding, s, next);
	if (!name)
	  return XML_ERROR_NO_MEMORY;
	declEntity = (ENTITY *)lookup(&dtd.paramEntities, name, sizeof(ENTITY));
	if (!declEntity)
	  return XML_ERROR_NO_MEMORY;
	if (declEntity->name != name) {
	  poolDiscard(&dtd.pool);
	  declEntity = 0;
	}
	else
	  poolFinish(&dtd.pool);
      }
      break;
    case XML_ROLE_ERROR:
      errorPtr = s;
      switch (tok) {
      case XML_TOK_PARAM_ENTITY_REF:
	return XML_ERROR_PARAM_ENTITY_REF;
      case XML_TOK_XML_DECL:
	return XML_ERROR_MISPLACED_XML_PI;
      default:
	return XML_ERROR_SYNTAX;
      }
    case XML_ROLE_GROUP_OPEN:
      if (prologState.level >= groupSize) {
	if (groupSize)
	  groupConnector = realloc(groupConnector, groupSize *= 2);
	else
	  groupConnector = malloc(groupSize = 32);
	if (!groupConnector)
	  return XML_ERROR_NO_MEMORY;
      }
      groupConnector[prologState.level] = 0;
      break;
    case XML_ROLE_GROUP_SEQUENCE:
      if (groupConnector[prologState.level] == '|') {
	*nextPtr = s;
	return XML_ERROR_SYNTAX;
      }
      groupConnector[prologState.level] = ',';
      break;
    case XML_ROLE_GROUP_CHOICE:
      if (groupConnector[prologState.level] == ',') {
	*nextPtr = s;
	return XML_ERROR_SYNTAX;
      }
      groupConnector[prologState.level] = '|';
      break;
    case XML_ROLE_NONE:
      switch (tok) {
      case XML_TOK_PARAM_ENTITY_REF:
	dtd.containsRef = 1;
	break;
      case XML_TOK_PI:
	if (!reportProcessingInstruction(parser, encoding, s, next))
	  return XML_ERROR_NO_MEMORY;
	break;
      }
      break;
    }
    s = next;
  }
  /* not reached */
}

static
enum XML_Error epilogProcessor(XML_Parser parser,
			       const char *s,
			       const char *end,
			       const char **nextPtr)
{
  processor = epilogProcessor;
  for (;;) {
    const char *next;
    int tok = XmlPrologTok(encoding, s, end, &next);
    switch (tok) {
    case XML_TOK_TRAILING_CR:
    case XML_TOK_NONE:
      if (nextPtr)
	*nextPtr = end;
      return XML_ERROR_NONE;
    case XML_TOK_PROLOG_S:
    case XML_TOK_COMMENT:
      break;
    case XML_TOK_PI:
      if (!reportProcessingInstruction(parser, encoding, s, next))
	return XML_ERROR_NO_MEMORY;
      break;
    case XML_TOK_INVALID:
      errorPtr = next;
      return XML_ERROR_INVALID_TOKEN;
    case XML_TOK_PARTIAL:
      if (nextPtr) {
	*nextPtr = s;
	return XML_ERROR_NONE;
      }
      errorPtr = s;
      return XML_ERROR_UNCLOSED_TOKEN;
    case XML_TOK_PARTIAL_CHAR:
      if (nextPtr) {
	*nextPtr = s;
	return XML_ERROR_NONE;
      }
      errorPtr = s;
      return XML_ERROR_PARTIAL_CHAR;
    default:
      errorPtr = s;
      return XML_ERROR_JUNK_AFTER_DOC_ELEMENT;
    }
    s = next;
  }
}

static enum XML_Error
storeAttributeValue(XML_Parser parser, const ENCODING *enc,
		    const char *ptr, const char *end,
		    STRING_POOL *pool)
{
  const ENCODING *utf8 = XmlGetInternalEncoding(XML_UTF8_ENCODING);
  for (;;) {
    const char *next;
    int tok = XmlAttributeValueTok(enc, ptr, end, &next);
    switch (tok) {
    case XML_TOK_NONE:
      return XML_ERROR_NONE;
    case XML_TOK_INVALID:
      errorPtr = next;
      return XML_ERROR_INVALID_TOKEN;
    case XML_TOK_PARTIAL:
      errorPtr = ptr;
      return XML_ERROR_INVALID_TOKEN;
    case XML_TOK_CHAR_REF:
      if (XmlCharRefNumber(enc, ptr) < 0) {
	errorPtr = ptr;
	return XML_ERROR_BAD_CHAR_REF;
      }
      else {
	char buf[XML_MAX_BYTES_PER_CHAR];
	int i;
	int n = XmlCharRefNumber(enc, ptr);
	if (n < 0) {
	  errorPtr = ptr;
	  return XML_ERROR_BAD_CHAR_REF;
	}
	n = XmlEncode(utf8, n, buf);
	if (!n) {
	  errorPtr = ptr;
	  return XML_ERROR_BAD_CHAR_REF;
	}
	for (i = 0; i < n; i++) {
	  if (!poolAppendByte(pool, buf[i]))
	    return XML_ERROR_NO_MEMORY;
	}
      }
      break;
    case XML_TOK_DATA_CHARS:
      if (!poolAppend(pool, enc, ptr, next))
	return XML_ERROR_NO_MEMORY;
      break;
      break;
    case XML_TOK_TRAILING_CR:
      next = ptr + enc->minBytesPerChar;
      /* fall through */
    case XML_TOK_DATA_NEWLINE:
      if (!poolAppendByte(pool, ' '))
	return XML_ERROR_NO_MEMORY;
      break;
    case XML_TOK_ENTITY_REF:
      {
	const char *name = poolStoreString(&dtd.pool, enc,
					   ptr + enc->minBytesPerChar,
					   next - enc->minBytesPerChar);
	ENTITY *entity;
	if (!name)
	  return XML_ERROR_NO_MEMORY;
	entity = (ENTITY *)lookup(&dtd.generalEntities, name, 0);
	poolDiscard(&dtd.pool);
	if (!entity) {
	  if (!dtd.containsRef) {
	    errorPtr = ptr;
	    return XML_ERROR_UNDEFINED_ENTITY;
	  }
	}
	else if (entity->open) {
	  errorPtr = ptr;
	  return XML_ERROR_RECURSIVE_ENTITY_REF;
	}
	else if (entity->notation) {
	  errorPtr = ptr;
	  return XML_ERROR_BINARY_ENTITY_REF;
	}
	else if (entity->magic) {
	  int i;
	  for (i = 0; i < entity->textLen; i++)
	    if (!poolAppendByte(pool, entity->textPtr[i]))
	      return XML_ERROR_NO_MEMORY;
	}
	else if (!entity->textPtr) {
	  errorPtr = ptr;
  	  return XML_ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF;
	}
	else {
	  enum XML_Error result;
	  const char *textEnd = entity->textPtr + entity->textLen;
	  entity->open = 1;
	  result = storeAttributeValue(parser, utf8, entity->textPtr, textEnd, pool);
	  entity->open = 0;
	  if (result) {
	    errorPtr = ptr;
	    return result;
	  }
	}
      }
      break;
    default:
      abort();
    }
    ptr = next;
  }
  /* not reached */
}

static
enum XML_Error storeEntityValue(XML_Parser parser,
				const char *entityTextPtr,
				const char *entityTextEnd)
{
  const ENCODING *utf8 = XmlGetInternalEncoding(XML_UTF8_ENCODING);
  STRING_POOL *pool = &(dtd.pool);
  entityTextPtr += encoding->minBytesPerChar;
  entityTextEnd -= encoding->minBytesPerChar;
  for (;;) {
    const char *next;
    int tok = XmlEntityValueTok(encoding, entityTextPtr, entityTextEnd, &next);
    switch (tok) {
    case XML_TOK_PARAM_ENTITY_REF:
      errorPtr = entityTextPtr;
      return XML_ERROR_SYNTAX;
    case XML_TOK_NONE:
      if (declEntity) {
	declEntity->textPtr = pool->start;
	declEntity->textLen = pool->ptr - pool->start;
	poolFinish(pool);
      }
      else
	poolDiscard(pool);
      return XML_ERROR_NONE;
    case XML_TOK_ENTITY_REF:
    case XML_TOK_DATA_CHARS:
      if (!poolAppend(pool, encoding, entityTextPtr, next))
	return XML_ERROR_NO_MEMORY;
      break;
    case XML_TOK_TRAILING_CR:
      next = entityTextPtr + encoding->minBytesPerChar;
      /* fall through */
    case XML_TOK_DATA_NEWLINE:
      if (pool->end == pool->ptr && !poolGrow(pool))
	return XML_ERROR_NO_MEMORY;
      *(pool->ptr)++ = '\n';
      break;
    case XML_TOK_CHAR_REF:
      {
	char buf[XML_MAX_BYTES_PER_CHAR];
	int i;
	int n = XmlCharRefNumber(encoding, entityTextPtr);
	if (n < 0) {
	  errorPtr = entityTextPtr;
	  return XML_ERROR_BAD_CHAR_REF;
	}
	n = XmlEncode(utf8, n, buf);
	if (!n) {
	  errorPtr = entityTextPtr;
	  return XML_ERROR_BAD_CHAR_REF;
	}
	for (i = 0; i < n; i++) {
	  if (pool->end == pool->ptr && !poolGrow(pool))
	    return XML_ERROR_NO_MEMORY;
	  *(pool->ptr)++ = buf[i];
	}
      }
      break;
    case XML_TOK_PARTIAL:
      errorPtr = entityTextPtr;
      return XML_ERROR_INVALID_TOKEN;
    case XML_TOK_INVALID:
      errorPtr = next;
      return XML_ERROR_INVALID_TOKEN;
    default:
      abort();
    }
    entityTextPtr = next;
  }
  /* not reached */
}

static int
reportProcessingInstruction(XML_Parser parser, const ENCODING *enc, const char *start, const char *end)
{
  const char *target;
  const char *data;
  if (!processingInstructionHandler)
    return 1;
  target = start + enc->minBytesPerChar * 2;
  data = target + XmlNameLength(enc, target);
  target = poolStoreString(&tempPool, enc, target, data);
  if (!target)
    return 0;
  poolFinish(&tempPool);
  data = poolStoreString(&tempPool, enc,
			 XmlSkipS(enc, data),
			 end - enc->minBytesPerChar*2);
  if (!data)
    return 0;
  processingInstructionHandler(userData, target, data);
  poolClear(&tempPool);
  return 1;
}

static int
addDefaultAttribute(ELEMENT_TYPE *type, ATTRIBUTE_ID *attId, const char *value)
{
  DEFAULT_ATTRIBUTE *att;
  if (type->nDefaultAtts == type->allocDefaultAtts) {
    if (type->allocDefaultAtts == 0)
      type->allocDefaultAtts = 8;
    else
      type->allocDefaultAtts *= 2;
    type->defaultAtts = realloc(type->defaultAtts, type->allocDefaultAtts);
    if (!type->defaultAtts)
      return 0;
  }
  att = type->defaultAtts + type->nDefaultAtts;
  att->id = attId;
  att->value = value;
  type->nDefaultAtts += 1;
  return 1;
}

static ATTRIBUTE_ID *
getAttributeId(XML_Parser parser, const ENCODING *enc, const char *start, const char *end)
{
  ATTRIBUTE_ID *id;
  const char *name;
  if (!poolAppendByte(&dtd.pool, 0))
    return 0;
  name = poolStoreString(&dtd.pool, enc, start, end);
  if (!name)
    return 0;
  ++name;
  id = (ATTRIBUTE_ID *)lookup(&dtd.attributeIds, name, sizeof(ATTRIBUTE_ID));
  if (!id)
    return 0;
  if (id->name != name)
    poolDiscard(&dtd.pool);
  else
    poolFinish(&dtd.pool);
  return id;
}

static int dtdInit(DTD *p)
{
  static const char *names[] = { "lt", "amp", "gt", "quot", "apos" };
  static const char chars[] = { '<', '&', '>', '"', '\'' };
  int i;

  poolInit(&(p->pool));
  hashTableInit(&(p->generalEntities));
  for (i = 0; i < 5; i++) {
    ENTITY *entity = (ENTITY *)lookup(&(p->generalEntities), names[i], sizeof(ENTITY));
    if (!entity)
      return 0;
    entity->textPtr = chars + i;
    entity->textLen = 1;
    entity->magic = 1;
  }
  hashTableInit(&(p->paramEntities));
  hashTableInit(&(p->elementTypes));
  hashTableInit(&(p->attributeIds));
  p->containsRef = 0;
  return 1;
}

static void dtdDestroy(DTD *p)
{
  poolDestroy(&(p->pool));
  hashTableDestroy(&(p->generalEntities));
  hashTableDestroy(&(p->paramEntities));
  hashTableDestroy(&(p->elementTypes));
  hashTableDestroy(&(p->attributeIds));
}

static
void poolInit(STRING_POOL *pool)
{
  pool->blocks = 0;
  pool->freeBlocks = 0;
  pool->start = 0;
  pool->ptr = 0;
  pool->end = 0;
}

static
void poolClear(STRING_POOL *pool)
{
  if (!pool->freeBlocks)
    pool->freeBlocks = pool->blocks;
  else {
    BLOCK *p = pool->blocks;
    while (p) {
      BLOCK *tem = p->next;
      p->next = pool->freeBlocks;
      pool->freeBlocks = p;
      p = tem;
    }
  }
  pool->blocks = 0;
  pool->start = 0;
  pool->ptr = 0;
  pool->end = 0;
}

static
void poolDestroy(STRING_POOL *pool)
{
  BLOCK *p = pool->blocks;
  while (p) {
    BLOCK *tem = p->next;
    free(p);
    p = tem;
  }
  pool->blocks = 0;
  p = pool->freeBlocks;
  while (p) {
    BLOCK *tem = p->next;
    free(p);
    p = tem;
  }
  pool->freeBlocks = 0;
  pool->ptr = 0;
  pool->start = 0;
  pool->end = 0;
}

static
const char *poolAppend(STRING_POOL *pool, const ENCODING *enc,
		       const char *ptr, const char *end)
{
  for (;;) {
    XmlConvert(enc, XML_UTF8_ENCODING, &ptr, end, &(pool->ptr), pool->end);
    if (ptr == end)
      break;
    if (!poolGrow(pool))
      return 0;
  }
  return pool->start;
}


static
const char *poolStoreString(STRING_POOL *pool, const ENCODING *enc,
			    const char *ptr, const char *end)
{
  if (!poolAppend(pool, enc, ptr, end))
    return 0;
  if (pool->ptr == pool->end && !poolGrow(pool))
    return 0;
  *(pool->ptr)++ = 0;
  return pool->start;
}

static
int poolGrow(STRING_POOL *pool)
{
  if (pool->freeBlocks) {
    if (pool->start == 0) {
      pool->blocks = pool->freeBlocks;
      pool->freeBlocks = pool->freeBlocks->next;
      pool->blocks->next = 0;
      pool->start = pool->blocks->s;
      pool->end = pool->start + pool->blocks->size;
      pool->ptr = pool->start;
      return 1;
    }
    if (pool->end - pool->start < pool->freeBlocks->size) {
      BLOCK *tem = pool->freeBlocks->next;
      pool->freeBlocks->next = pool->blocks;
      pool->blocks = pool->freeBlocks;
      pool->freeBlocks = tem;
      memcpy(pool->blocks->s, pool->start, pool->end - pool->start);
      pool->ptr = pool->blocks->s + (pool->ptr - pool->start);
      pool->start = pool->blocks->s;
      pool->end = pool->start + pool->blocks->size;
      return 1;
    }
  }
  if (pool->blocks && pool->start == pool->blocks->s) {
    size_t blockSize = (pool->end - pool->start)*2;
    pool->blocks = realloc(pool->blocks, offsetof(BLOCK, s) + blockSize);
    if (!pool->blocks)
      return 0;
    pool->blocks->size = blockSize;
    pool->ptr = pool->blocks->s + (pool->ptr - pool->start);
    pool->start = pool->blocks->s;
    pool->end = pool->start + blockSize;
  }
  else {
    BLOCK *tem;
    size_t blockSize = pool->end - pool->start;
    if (blockSize < INIT_BLOCK_SIZE)
      blockSize = INIT_BLOCK_SIZE;
    else
      blockSize *= 2;
    tem = malloc(offsetof(BLOCK, s) + blockSize);
    if (!tem)
      return 0;
    tem->size = blockSize;
    tem->next = pool->blocks;
    pool->blocks = tem;
    memcpy(tem->s, pool->start, pool->ptr - pool->start);
    pool->ptr = tem->s + (pool->ptr - pool->start);
    pool->start = tem->s;
    pool->end = tem->s + blockSize;
  }
  return 1;
}
