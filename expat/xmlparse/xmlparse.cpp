/*
The contents of this file are subject to the Mozilla Public License
Version 1.1 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations
under the License.

The Original Code is expat.

The Initial Developer of the Original Code is James Clark.
Portions created by James Clark are Copyright (C) 1998, 1999
James Clark. All Rights Reserved.

Contributor(s):

Alternatively, the contents of this file may be used under the terms
of the GNU General Public License (the "GPL"), in which case the
provisions of the GPL are applicable instead of those above.  If you
wish to allow use of your version of this file only under the terms of
the GPL and not to allow others to use your version of this file under
the MPL, indicate your decision by deleting the provisions above and
replace them with the notice and other provisions required by the
GPL. If you do not delete the provisions above, a recipient may use
your version of this file under either the MPL or the GPL.
*/

#include <string.h>
#include <stdlib.h>
#include <new.h>
#include "xmlparse.hpp"

#ifdef XML_UNICODE
#define XML_ENCODE_MAX XML_UTF16_ENCODE_MAX
#define XmlConvert XmlUtf16Convert
#define XmlGetInternalEncoding XmlGetUtf16InternalEncoding
#define XmlGetInternalEncodingNS XmlGetUtf16InternalEncodingNS
#define XmlEncode XmlUtf16Encode
#define MUST_CONVERT(enc, s) (!(enc)->isUtf16 || (((unsigned long)s) & 1))
typedef unsigned short ICHAR;
#else
#define XML_ENCODE_MAX XML_UTF8_ENCODE_MAX
#define XmlConvert XmlUtf8Convert
#define XmlGetInternalEncoding XmlGetUtf8InternalEncoding
#define XmlGetInternalEncodingNS XmlGetUtf8InternalEncodingNS
#define XmlEncode XmlUtf8Encode
#define MUST_CONVERT(enc, s) (!(enc)->isUtf8)
typedef char ICHAR;
#endif


#ifndef XML_NS

#define XmlInitEncodingNS XmlInitEncoding
#define XmlInitUnknownEncodingNS XmlInitUnknownEncoding
#undef XmlGetInternalEncodingNS
#define XmlGetInternalEncodingNS XmlGetInternalEncoding
#define XmlParseXmlDeclNS XmlParseXmlDecl

#endif

#ifdef XML_UNICODE_WCHAR_T
#define XML_T(x) L ## x
#else
#define XML_T(x) x
#endif

/* Round up n to be a multiple of sz, where sz is a power of 2. */
inline int ROUND_UP(int n, int sz) 
{
  return (n + (sz - 1)) & ~(sz - 1);
}

#include "xmltok.h"
#include "xmlrole.h"

const int INIT_BLOCK_SIZE = 1024;
const int INIT_TAG_BUF_SIZE = 32;  /* must be a multiple of sizeof(XML_Char) */
const int INIT_DATA_BUF_SIZE = 1024;
const int INIT_ATTS_SIZE = 16;
const int INIT_BUFFER_SIZE = 1024;

const int EXPAND_SPARE = 24;

typedef const XML_Char *Key;

struct Named {
  Key name;
};

class HashTable {
public:
  HashTable();
  ~HashTable();
  Named *lookup(Key name, size_t createSize);
  friend class HashTableIter;
private:
  static unsigned long hash(Key s);
  static int keyeq(Key, Key);
  Named **v_;
  size_t size_;
  size_t used_;
  size_t usedLim_;
};

class HashTableIter {
public:
  HashTableIter();
  HashTableIter(const HashTable &);
  Named *next();
  void init(const HashTable &);
private:
  Named **p_;
  Named **end_;
};

class StringPool {
public:
  StringPool();
  ~StringPool();
  void clear();
  const XML_Char *start() const {
    return start_;
  }
  const int length() const {
    return ptr_ - start_;
  }
  void chop() {
    --ptr_;
  }
  XML_Char lastChar() const {
    return ptr_[-1];
  }

  void discard() {
    ptr_ = start_;
  }
  void finish() {
    start_ = ptr_;
  }

  int appendChar(XML_Char c) {
    if (ptr_ == end_ && !grow())
      return 0;
    else {
      *ptr_++ = c;
      return 1;
    }
  }
  XML_Char *append(const ENCODING *enc, const char *ptr, const char *end);
  XML_Char *storeString(const ENCODING *enc,
			const char *ptr, const char *end);
  const XML_Char *copyString(const XML_Char *s);
  const XML_Char *copyStringN(const XML_Char *s, int n);
private:
  int grow();
  
  struct Block {
    Block *next;
    int size;
    XML_Char *s() {
      return (XML_Char *)(this + 1);
    }
  };

  Block *blocks_;
  Block *freeBlocks_;
  const XML_Char *end_;
  XML_Char *ptr_;
  XML_Char *start_;
};

struct Prefix;
struct AttributeId;

struct Binding {
  Prefix *prefix;
  Binding *nextTagBinding;
  Binding *prevPrefixBinding;
  const AttributeId *attId;
  XML_Char *uri;
  int uriLen;
  int uriAlloc;
};

struct Prefix {
  const XML_Char *name;
  Binding *binding;
};

struct TagName {
  const XML_Char *str;
  const XML_Char *localPart;
  int uriLen;
};

struct Tag {
  Tag *parent;
  const char *rawName;
  int rawNameLength;
  TagName name;
  char *buf;
  char *bufEnd;
  Binding *bindings;
};

struct Entity {
  const XML_Char *name;
  const XML_Char *textPtr;
  int textLen;
  const XML_Char *systemId;
  const XML_Char *base;
  const XML_Char *publicId;
  const XML_Char *notation;
  char open;
};

/* The XML_Char before the name is used to determine whether
an attribute has been specified. */
struct AttributeId {
  XML_Char *name;
  Prefix *prefix;
  char maybeTokenized;
  char xmlns;
};

struct DefaultAttribute {
  const AttributeId *id;
  char isCdata;
  const XML_Char *value;
};

struct ElementType {
  const XML_Char *name;
  Prefix *prefix;
  int nDefaultAtts;
  int allocDefaultAtts;
  DefaultAttribute *defaultAtts;
};

struct Dtd {
  HashTable generalEntities;
  HashTable elementTypes;
  HashTable attributeIds;
  HashTable prefixes;
  StringPool pool;
  int complete;
  int standalone;
#ifdef XML_DTD
  HashTable paramEntities;
#endif /* XML_DTD */
  Prefix defaultPrefix;
  Dtd();
  ~Dtd();
#ifdef XML_DTD
  static void swap(Dtd &, Dtd &);
#endif /* XML_DTD */
  static int copy(Dtd &newDtd, const Dtd &oldDtd);
  static int copyEntityTable(HashTable &, StringPool &, const HashTable &);
};

struct OpenInternalEntity {
  const char *internalEventPtr;
  const char *internalEventEndPtr;
  OpenInternalEntity *next;
  Entity *entity;
};

class XML_ParserImpl : public XML_Parser {
public:
  char *buffer;
  // first character to be parsed
  const char *bufferPtr;
  // past last character to be parsed
  char *bufferEnd;
  // allocated end of buffer
  const char *bufferLim;
  long parseEndByteIndex;
  const char *parseEndPtr;
  XML_Char *dataBuf;
  XML_Char *dataBufEnd;
  XML_ElementHandler *elementHandler;
  XML_CharacterDataHandler *characterDataHandler;
  XML_ProcessingInstructionHandler *processingInstructionHandler;
  XML_CommentHandler *commentHandler;
  XML_CdataSectionHandler *cdataSectionHandler;
  XML_DefaultHandler *defaultHandler;
  XML_DoctypeDeclHandler *doctypeDeclHandler;
  XML_UnparsedEntityDeclHandler *unparsedEntityDeclHandler;
  XML_NotationDeclHandler *notationDeclHandler;
  XML_NamespaceDeclHandler *namespaceDeclHandler;
  XML_NotStandaloneHandler *notStandaloneHandler;
  XML_ExternalEntityRefHandler *externalEntityRefHandler;
  XML_UnknownEncodingHandler *unknownEncodingHandler;
  const ENCODING *encoding;
  INIT_ENCODING initEncoding;
  const ENCODING *internalEncoding;
  const XML_Char *protocolEncodingName;
  int ns;
  XML_Encoding *unknownEncoding;
  void *unknownEncodingMem;
  PROLOG_STATE prologState;
  Error (XML_ParserImpl::*processor)(const char *start, const char *end, const char **endPtr);
  Error errorCode;
  const char *eventPtr;
  const char *eventEndPtr;
  const char *positionPtr;
  OpenInternalEntity *openInternalEntities;
  int defaultExpandInternalEntities;
  int tagLevel;
  Entity *declEntity;
  const XML_Char *declNotationName;
  const XML_Char *declNotationPublicId;
  ElementType *declElementType;
  AttributeId *declAttributeId;
  char declAttributeIsCdata;
  Dtd dtd;
  const XML_Char *curBase;
  Tag *tagStack;
  Tag *freeTagList;
  Binding *inheritedBindings;
  Binding *freeBindingList;
  int attsSize;
  int nSpecifiedAtts;
  ATTRIBUTE *atts;
  POSITION position;
  StringPool tempPool;
  StringPool temp2Pool;
  char *groupConnector;
  unsigned groupSize;
  int hadExternalDoctype;
  XML_Char namespaceSeparator;
#ifdef XML_DTD
  enum ParamEntityParsing paramEntityParsing;
  XML_ParserImpl *parentParser;
#endif
private:
  ~XML_ParserImpl();
  Error handleUnknownEncoding(const XML_Char *encodingName);
  Error processXmlDecl(int isGeneralTextEntity, const char *, const char *);
  Error initializeEncoding();
  Error doProlog(const ENCODING *enc, const char *s,
		     const char *end, int tok, const char *next, const char **nextPtr);
  Error processInternalParamEntity(Entity *entity);
  Error doContent(int startTagLevel, const ENCODING *enc,
		      const char *start, const char *end, const char **endPtr);
  Error doCdataSection(const ENCODING *, const char **startPtr, const char *end, const char **nextPtr);
#ifdef XML_DTD
  Error doIgnoreSection(const ENCODING *, const char **startPtr, const char *end, const char **nextPtr);
#endif /* XML_DTD */
  Error storeAtts(const ENCODING *, const char *s,
		      TagName *tagNamePtr, Binding **bindingsPtr);

  int addBinding(Prefix *prefix, const AttributeId *attId, const XML_Char *uri, Binding **bindingsPtr);
  static int defineAttribute(ElementType *type, AttributeId *, int isCdata, const XML_Char *dfltValue);
  Error storeAttributeValue(const ENCODING *, int isCdata, const char *, const char *, StringPool &);
  Error appendAttributeValue(const ENCODING *, int isCdata, const char *, const char *, StringPool &);
  AttributeId *getAttributeId(const ENCODING *enc, const char *start, const char *end);
  int setElementTypePrefix(ElementType *);
  Error storeEntityValue(const ENCODING *enc, const char *start, const char *end);
  int reportProcessingInstruction(const ENCODING *enc, const char *start, const char *end);
  int reportComment(const ENCODING *enc, const char *start, const char *end);
  void reportDefault(const ENCODING *enc, const char *start, const char *end);
  const XML_Char *getContext();
  int setContext(const XML_Char *context);
  static void normalizePublicId(XML_Char *s);
  static void normalizeLines(XML_Char *s);
  Error prologProcessor(const char *start, const char *end, const char **endPtr);
  Error prologInitProcessor(const char *start, const char *end, const char **endPtr);
  Error contentProcessor(const char *start, const char *end, const char **endPtr);
  Error cdataSectionProcessor(const char *start, const char *end, const char **endPtr);
#ifdef XML_DTD
  Error ignoreSectionProcessor(const char *start, const char *end, const char **endPtr);
#endif /* XML_DTD */
  Error epilogProcessor(const char *start, const char *end, const char **endPtr);
  Error errorProcessor(const char *start, const char *end, const char **endPtr);
  Error externalEntityInitProcessor(const char *start, const char *end, const char **endPtr);
  Error externalEntityInitProcessor2(const char *start, const char *end, const char **endPtr);
  Error externalEntityInitProcessor3(const char *start, const char *end, const char **endPtr);
  Error externalEntityContentProcessor(const char *start, const char *end, const char **endPtr);
public:
  int init(const XML_Char *encodingName);
  int initNS(const XML_Char *encodingName, XML_Char nsSep);
  void release();
  void setElementHandler(XML_ElementHandler *handler);
  void setCharacterDataHandler(XML_CharacterDataHandler *handler);
  void setProcessingInstructionHandler(XML_ProcessingInstructionHandler *handler);
  void setCommentHandler(XML_CommentHandler *handler);
  void setCdataSectionHandler(XML_CdataSectionHandler *handler);
  void setDefaultHandler(XML_DefaultHandler *handler);
  void setDefaultHandlerExpand(XML_DefaultHandler *handler);
  void setDoctypeDeclHandler(XML_DoctypeDeclHandler *handler);
  void setUnparsedEntityDeclHandler(XML_UnparsedEntityDeclHandler *handler);
  void setNotationDeclHandler(XML_NotationDeclHandler *handler);
  void setNamespaceDeclHandler(XML_NamespaceDeclHandler *handler);
  void setNotStandaloneHandler(XML_NotStandaloneHandler *handler);
  void setExternalEntityRefHandler(XML_ExternalEntityRefHandler *handler);
  void setUnknownEncodingHandler(XML_UnknownEncodingHandler *handler);
  void defaultCurrent();
  int setEncoding(const XML_Char *encoding);
  int setBase(const XML_Char *base);
  const XML_Char *getBase();
  int getSpecifiedAttributeCount();
  int parse(const char *s, int len, int isFinal);
  char *getBuffer(int len);
  int parseBuffer(int len, int isFinal);
  XML_Parser *externalEntityParserCreate(const XML_Char *context, const XML_Char *encoding);
  int setParamEntityParsing(ParamEntityParsing parsing);
  Error getErrorCode();
  int getCurrentLineNumber();
  int getCurrentColumnNumber();
  long getCurrentByteIndex();
  int getCurrentByteCount();
};

void XML_ParserImpl::release()
{
  this->~XML_ParserImpl();
  free(this);
}

XML_Parser *XML_Parser::parserCreate(const XML_Char *encodingName)
{
  void *mem = malloc(sizeof(XML_ParserImpl));
  if (!mem)
    return 0;
  XML_ParserImpl *p = new (mem) XML_ParserImpl;
  if (!p->init(encodingName)) {
    p->release();
    return 0;
  }
  return p;
}

XML_Parser *XML_Parser::parserCreateNS(const XML_Char *encodingName, XML_Char namespaceSeparator)
{
  void *mem = malloc(sizeof(XML_ParserImpl));
  if (!mem)
    return 0;
  XML_ParserImpl *p = new (mem) XML_ParserImpl;
  if (!p->initNS(encodingName, namespaceSeparator)) {
    p->release();
    return 0;
  }
  return p;
}


int XML_ParserImpl::init(const XML_Char *encodingName)
{
  processor = prologInitProcessor;
  XmlPrologStateInit(&prologState);
  elementHandler = 0;
  characterDataHandler = 0;
  processingInstructionHandler = 0;
  commentHandler = 0;
  cdataSectionHandler = 0;
  defaultHandler = 0;
  doctypeDeclHandler = 0;
  unparsedEntityDeclHandler = 0;
  notationDeclHandler = 0;
  namespaceDeclHandler = 0;
  notStandaloneHandler = 0;
  externalEntityRefHandler = 0;
  unknownEncodingHandler = 0;
  buffer = 0;
  bufferPtr = 0;
  bufferEnd = 0;
  parseEndByteIndex = 0;
  parseEndPtr = 0;
  bufferLim = 0;
  declElementType = 0;
  declAttributeId = 0;
  declEntity = 0;
  declNotationName = 0;
  declNotationPublicId = 0;
  memset(&position, 0, sizeof(POSITION));
  errorCode = ERROR_NONE;
  eventPtr = 0;
  eventEndPtr = 0;
  positionPtr = 0;
  openInternalEntities = 0;
  tagLevel = 0;
  tagStack = 0;
  freeTagList = 0;
  freeBindingList = 0;
  inheritedBindings = 0;
  attsSize = INIT_ATTS_SIZE;
  atts = (ATTRIBUTE *)malloc(attsSize * sizeof(ATTRIBUTE));
  nSpecifiedAtts = 0;
  dataBuf = (XML_Char *)malloc(INIT_DATA_BUF_SIZE * sizeof(XML_Char));
  groupSize = 0;
  groupConnector = 0;
  hadExternalDoctype = 0;
  unknownEncoding = 0;
  unknownEncodingMem = 0;
  namespaceSeparator = '!';
#ifdef XML_DTD
  parentParser = 0;
  paramEntityParsing = PARAM_ENTITY_PARSING_NEVER;
#endif
  ns = 0;
  protocolEncodingName = encodingName ? tempPool.copyString(encodingName) : 0;
  curBase = 0;
  if (!atts || !dataBuf
      || (encodingName && !protocolEncodingName))
    return 0;
  dataBufEnd = dataBuf + INIT_DATA_BUF_SIZE;
  XmlInitEncoding(&initEncoding, &encoding, 0);
  internalEncoding = XmlGetInternalEncoding();
  return 1;
}


int XML_ParserImpl::initNS(const XML_Char *encodingName, XML_Char nsSep)
{
  static
  const XML_Char implicitContext[] = {
    XML_T('x'), XML_T('m'), XML_T('l'), XML_T('='),
    XML_T('h'), XML_T('t'), XML_T('t'), XML_T('p'), XML_T(':'),
    XML_T('/'), XML_T('/'), XML_T('w'), XML_T('w'), XML_T('w'),
    XML_T('.'), XML_T('w'), XML_T('3'),
    XML_T('.'), XML_T('o'), XML_T('r'), XML_T('g'),
    XML_T('/'), XML_T('X'), XML_T('M'), XML_T('L'),
    XML_T('/'), XML_T('1'), XML_T('9'), XML_T('9'), XML_T('8'),
    XML_T('/'), XML_T('n'), XML_T('a'), XML_T('m'), XML_T('e'),
    XML_T('s'), XML_T('p'), XML_T('a'), XML_T('c'), XML_T('e'),
    XML_T('\0')
  };

  if (!init(encodingName))
    return 0;
  XmlInitEncodingNS(&initEncoding, &encoding, 0);
  ns = 1;
  internalEncoding = XmlGetInternalEncodingNS();
  namespaceSeparator = nsSep;
  if (!setContext(implicitContext))
    return 0;
  return 1;
}

int XML_ParserImpl::setEncoding(const XML_Char *encodingName)
{
  if (!encodingName)
    protocolEncodingName = 0;
  else {
    protocolEncodingName = tempPool.copyString(encodingName);
    if (!protocolEncodingName)
      return 0;
  }
  return 1;
}

XML_Parser *
XML_ParserImpl::externalEntityParserCreate(const XML_Char *context,
					   const XML_Char *encodingName)
{
  XML_ParserImpl *parser = (XML_ParserImpl *)(ns
    ? XML_Parser::parserCreateNS(encodingName, namespaceSeparator)
    : XML_Parser::parserCreate(encodingName));
  if (!parser)
    return 0;
  parser->elementHandler = elementHandler;
  parser->characterDataHandler = characterDataHandler;
  parser->processingInstructionHandler = processingInstructionHandler;
  parser->commentHandler = commentHandler;
  parser->cdataSectionHandler = cdataSectionHandler;
  parser->defaultHandler = defaultHandler;
  parser->namespaceDeclHandler = namespaceDeclHandler;
  parser->notStandaloneHandler = notStandaloneHandler;
  parser->externalEntityRefHandler = parser->externalEntityRefHandler;
  parser->unknownEncodingHandler = unknownEncodingHandler;
  parser->defaultExpandInternalEntities = defaultExpandInternalEntities;
#ifdef XML_DTD
  parser->paramEntityParsing = paramEntityParsing;
  if (context) {
#endif /* XML_DTD */
    if (!Dtd::copy(parser->dtd, dtd) || !parser->setContext(context)) {
      parser->release();
      return 0;
    }
    parser->processor = externalEntityInitProcessor;
#ifdef XML_DTD
  }
  else {
    Dtd::swap(parser->dtd, dtd);
    parser->parentParser = this;
    XmlPrologStateInitExternalEntity(&(parser->prologState));
    parser->dtd.complete = 1;
    parser->hadExternalDoctype = 1;
  }
#endif /* XML_DTD */
  return parser;
}

static
void destroyBindings(Binding *bindings)
{
  for (;;) {
    Binding *b = bindings;
    if (!b)
      break;
    bindings = b->nextTagBinding;
    free(b->uri);
    free(b);
  }
}

XML_ParserImpl::~XML_ParserImpl()
{
  for (;;) {
    Tag *p;
    if (tagStack == 0) {
      if (freeTagList == 0)
	break;
      tagStack = freeTagList;
      freeTagList = 0;
    }
    p = tagStack;
    tagStack = tagStack->parent;
    free(p->buf);
    destroyBindings(p->bindings);
    free(p);
  }
  destroyBindings(freeBindingList);
  destroyBindings(inheritedBindings);
#ifdef XML_DTD
  if (parentParser) {
    if (hadExternalDoctype)
      dtd.complete = 0;
    Dtd::swap(dtd, parentParser->dtd);
  }
#endif /* XML_DTD */
  free((void *)atts);
  free(groupConnector);
  free(buffer);
  free(dataBuf);
  free(unknownEncodingMem);
  if (unknownEncoding)
    unknownEncoding->release();
}

int XML_ParserImpl::setBase(const XML_Char *p)
{
  if (p) {
    p = dtd.pool.copyString(p);
    if (!p)
      return 0;
    curBase = p;
  }
  else
    curBase = 0;
  return 1;
}

const XML_Char *XML_ParserImpl::getBase()
{
  return curBase;
}

int XML_ParserImpl::getSpecifiedAttributeCount()
{
  return nSpecifiedAtts;
}

void XML_ParserImpl::setElementHandler(XML_ElementHandler *handler)
{
  elementHandler = handler;
}

void XML_ParserImpl::setCharacterDataHandler(XML_CharacterDataHandler *handler)
{
  characterDataHandler = handler;
}

void XML_ParserImpl::setProcessingInstructionHandler(XML_ProcessingInstructionHandler *handler)
{
  processingInstructionHandler = handler;
}

void XML_ParserImpl::setCommentHandler(XML_CommentHandler *handler)
{
  commentHandler = handler;
}

void XML_ParserImpl::setCdataSectionHandler(XML_CdataSectionHandler *handler)
{
  cdataSectionHandler = handler;
}

void XML_ParserImpl::setDefaultHandler(XML_DefaultHandler *handler)
{
  defaultHandler = handler;
  defaultExpandInternalEntities = 0;
}

void XML_ParserImpl::setDefaultHandlerExpand(XML_DefaultHandler *handler)
{
  defaultHandler = handler;
  defaultExpandInternalEntities = 1;
}

void XML_ParserImpl::setDoctypeDeclHandler(XML_DoctypeDeclHandler *handler)
{
  doctypeDeclHandler = handler;
}

void XML_ParserImpl::setUnparsedEntityDeclHandler(XML_UnparsedEntityDeclHandler *handler)
{
  unparsedEntityDeclHandler = handler;
}

void XML_ParserImpl::setNotationDeclHandler(XML_NotationDeclHandler *handler)
{
  notationDeclHandler = handler;
}

void XML_ParserImpl::setNamespaceDeclHandler(XML_NamespaceDeclHandler *handler)
{
  namespaceDeclHandler = handler;
}

void XML_ParserImpl::setNotStandaloneHandler(XML_NotStandaloneHandler *handler)
{
  notStandaloneHandler = handler;
}

void XML_ParserImpl::setExternalEntityRefHandler(XML_ExternalEntityRefHandler *handler)
{
  externalEntityRefHandler = handler;
}

void XML_ParserImpl::setUnknownEncodingHandler(XML_UnknownEncodingHandler *handler)
{
  unknownEncodingHandler = handler;
}

int XML_ParserImpl::setParamEntityParsing(ParamEntityParsing parsing)
{
#ifdef XML_DTD
  paramEntityParsing = parsing;
  return 1;
#else
  return parsing == PARAM_ENTITY_PARSING_NEVER;
#endif
}

int XML_ParserImpl::parse(const char *s, int len, int isFinal)
{
  if (len == 0) {
    if (!isFinal)
      return 1;
    positionPtr = bufferPtr;
    errorCode = (this->*processor)(bufferPtr, parseEndPtr = bufferEnd, 0);
    if (errorCode == ERROR_NONE)
      return 1;
    eventEndPtr = eventPtr;
    processor = errorProcessor;
    return 0;
  }
  else if (bufferPtr == bufferEnd) {
    const char *end;
    int nLeftOver;
    parseEndByteIndex += len;
    positionPtr = s;
    if (isFinal) {
      errorCode = (this->*processor)(s, parseEndPtr = s + len, 0);
      if (errorCode == ERROR_NONE)
	return 1;
      eventEndPtr = eventPtr;
      processor = errorProcessor;
      return 0;
    }
    errorCode = (this->*processor)(s, parseEndPtr = s + len, &end);
    if (errorCode != ERROR_NONE) {
      eventEndPtr = eventPtr;
      processor = errorProcessor;
      return 0;
    }
    XmlUpdatePosition(encoding, positionPtr, end, &position);
    nLeftOver = s + len - end;
    if (nLeftOver) {
      if (buffer == 0 || nLeftOver > bufferLim - buffer) {
	/* FIXME avoid integer overflow */
	buffer = (char *)(buffer == 0 ? malloc(len * 2) : realloc(buffer, len * 2));
	/* FIXME storage leak if realloc fails */
	if (!buffer) {
	  errorCode = ERROR_NO_MEMORY;
	  eventPtr = eventEndPtr = 0;
	  processor = errorProcessor;
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
    memcpy(getBuffer(len), s, len);
    return parseBuffer(len, isFinal);
  }
}

int XML_ParserImpl::parseBuffer(int len, int isFinal)
{
  const char *start = bufferPtr;
  positionPtr = start;
  bufferEnd += len;
  parseEndByteIndex += len;
  errorCode = (this->*processor)(start, parseEndPtr = bufferEnd,
			isFinal ? (const char **)0 : &bufferPtr);
  if (errorCode == ERROR_NONE) {
    if (!isFinal)
      XmlUpdatePosition(encoding, positionPtr, bufferPtr, &position);
    return 1;
  }
  else {
    eventEndPtr = eventPtr;
    processor = errorProcessor;
    return 0;
  }
}

char *XML_ParserImpl::getBuffer(int len)
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
      int bufferSize = bufferLim - bufferPtr;
      if (bufferSize == 0)
	bufferSize = INIT_BUFFER_SIZE;
      do {
	bufferSize *= 2;
      } while (bufferSize < neededSize);
      newBuf = (char *)malloc(bufferSize);
      if (newBuf == 0) {
	errorCode = ERROR_NO_MEMORY;
	return 0;
      }
      bufferLim = newBuf + bufferSize;
      if (bufferPtr) {
	memcpy(newBuf, bufferPtr, bufferEnd - bufferPtr);
	free(buffer);
      }
      bufferEnd = newBuf + (bufferEnd - bufferPtr);
      bufferPtr = buffer = newBuf;
    }
  }
  return bufferEnd;
}

XML_Parser::Error XML_ParserImpl::getErrorCode()
{
  return errorCode;
}

long XML_ParserImpl::getCurrentByteIndex()
{
  if (eventPtr)
    return parseEndByteIndex - (parseEndPtr - eventPtr);
  return -1;
}

int XML_ParserImpl::getCurrentByteCount()
{
  if (eventEndPtr && eventPtr)
    return eventEndPtr - eventPtr;
  return 0;
}

int XML_ParserImpl::getCurrentLineNumber()
{
  if (eventPtr) {
    XmlUpdatePosition(encoding, positionPtr, eventPtr, &position);
    positionPtr = eventPtr;
  }
  return position.lineNumber + 1;
}

int XML_ParserImpl::getCurrentColumnNumber()
{
  if (eventPtr) {
    XmlUpdatePosition(encoding, positionPtr, eventPtr, &position);
    positionPtr = eventPtr;
  }
  return position.columnNumber;
}

void XML_ParserImpl::defaultCurrent()
{
  if (defaultHandler) {
    if (openInternalEntities)
      reportDefault(internalEncoding,
		    openInternalEntities->internalEventPtr,
		    openInternalEntities->internalEventEndPtr);
    else
      reportDefault(encoding, eventPtr, eventEndPtr);
  }
}

const XML_LChar *XML_Parser::errorString(int code)
{
  static const XML_LChar *message[] = {
    0,
    XML_T("out of memory"),
    XML_T("syntax error"),
    XML_T("no element found"),
    XML_T("not well-formed"),
    XML_T("unclosed token"),
    XML_T("unclosed token"),
    XML_T("mismatched tag"),
    XML_T("duplicate attribute"),
    XML_T("junk after document element"),
    XML_T("illegal parameter entity reference"),
    XML_T("undefined entity"),
    XML_T("recursive entity reference"),
    XML_T("asynchronous entity"),
    XML_T("reference to invalid character number"),
    XML_T("reference to binary entity"),
    XML_T("reference to external entity in attribute"),
    XML_T("xml processing instruction not at start of external entity"),
    XML_T("unknown encoding"),
    XML_T("encoding specified in XML declaration is incorrect"),
    XML_T("unclosed CDATA section"),
    XML_T("error in processing external entity reference"),
    XML_T("document is not standalone")
  };
  if (code > 0 && code < sizeof(message)/sizeof(message[0]))
    return message[code];
  return 0;
}

XML_Parser::Error
XML_ParserImpl::contentProcessor(const char *start,
				 const char *end,
				 const char **endPtr)
{
  return doContent(0, encoding, start, end, endPtr);
}

XML_Parser::Error
XML_ParserImpl::externalEntityInitProcessor(const char *start,
					    const char *end,
					    const char **endPtr)
{
  Error result = initializeEncoding();
  if (result != ERROR_NONE)
    return result;
  processor = externalEntityInitProcessor2;
  return externalEntityInitProcessor2(start, end, endPtr);
}

XML_Parser::Error
XML_ParserImpl::externalEntityInitProcessor2(const char *start,
					     const char *end,
					     const char **endPtr)
{
  const char *next;
  int tok = XmlContentTok(encoding, start, end, &next);
  switch (tok) {
  case XML_TOK_BOM:
    start = next;
    break;
  case XML_TOK_PARTIAL:
    if (endPtr) {
      *endPtr = start;
      return ERROR_NONE;
    }
    eventPtr = start;
    return ERROR_UNCLOSED_TOKEN;
  case XML_TOK_PARTIAL_CHAR:
    if (endPtr) {
      *endPtr = start;
      return ERROR_NONE;
    }
    eventPtr = start;
    return ERROR_PARTIAL_CHAR;
  }
  processor = externalEntityInitProcessor3;
  return externalEntityInitProcessor3(start, end, endPtr);
}

XML_Parser::Error
XML_ParserImpl::externalEntityInitProcessor3(const char *start,
					     const char *end,
					     const char **endPtr)
{
  const char *next;
  int tok = XmlContentTok(encoding, start, end, &next);
  switch (tok) {
  case XML_TOK_XML_DECL:
    {
      Error result = processXmlDecl(1, start, next);
      if (result != ERROR_NONE)
	return result;
      start = next;
    }
    break;
  case XML_TOK_PARTIAL:
    if (endPtr) {
      *endPtr = start;
      return ERROR_NONE;
    }
    eventPtr = start;
    return ERROR_UNCLOSED_TOKEN;
  case XML_TOK_PARTIAL_CHAR:
    if (endPtr) {
      *endPtr = start;
      return ERROR_NONE;
    }
    eventPtr = start;
    return ERROR_PARTIAL_CHAR;
  }
  processor = externalEntityContentProcessor;
  tagLevel = 1;
  return doContent(1, encoding, start, end, endPtr);
}

XML_Parser::Error
XML_ParserImpl::externalEntityContentProcessor(const char *start,
					       const char *end,
					       const char **endPtr)
{
  return doContent(1, encoding, start, end, endPtr);
}

XML_Parser::Error
XML_ParserImpl::doContent(int startTagLevel,
			  const ENCODING *enc,
			  const char *s,
			  const char *end,
			  const char **nextPtr)
{
  const char **eventPP;
  const char **eventEndPP;
  if (enc == encoding) {
    eventPP = &eventPtr;
    eventEndPP = &eventEndPtr;
  }
  else {
    eventPP = &(openInternalEntities->internalEventPtr);
    eventEndPP = &(openInternalEntities->internalEventEndPtr);
  }
  *eventPP = s;
  for (;;) {
    const char *next = s; /* XmlContentTok doesn't always set the last arg */
    int tok = XmlContentTok(enc, s, end, &next);
    *eventEndPP = next;
    switch (tok) {
    case XML_TOK_TRAILING_CR:
      if (nextPtr) {
	*nextPtr = s;
	return ERROR_NONE;
      }
      *eventEndPP = end;
      if (characterDataHandler) {
	XML_Char c = 0xA;
	characterDataHandler->characterData(&c, 1);
      }
      else if (defaultHandler)
	reportDefault(enc, s, end);
      if (startTagLevel == 0)
	return ERROR_NO_ELEMENTS;
      if (tagLevel != startTagLevel)
	return ERROR_ASYNC_ENTITY;
      return ERROR_NONE;
    case XML_TOK_NONE:
      if (nextPtr) {
	*nextPtr = s;
	return ERROR_NONE;
      }
      if (startTagLevel > 0) {
	if (tagLevel != startTagLevel)
	  return ERROR_ASYNC_ENTITY;
	return ERROR_NONE;
      }
      return ERROR_NO_ELEMENTS;
    case XML_TOK_INVALID:
      *eventPP = next;
      return ERROR_INVALID_TOKEN;
    case XML_TOK_PARTIAL:
      if (nextPtr) {
	*nextPtr = s;
	return ERROR_NONE;
      }
      return ERROR_UNCLOSED_TOKEN;
    case XML_TOK_PARTIAL_CHAR:
      if (nextPtr) {
	*nextPtr = s;
	return ERROR_NONE;
      }
      return ERROR_PARTIAL_CHAR;
    case XML_TOK_ENTITY_REF:
      {
	const XML_Char *name;
	Entity *entity;
	XML_Char ch = XmlPredefinedEntityName(enc,
					      s + enc->minBytesPerChar,
					      next - enc->minBytesPerChar);
	if (ch) {
	  if (characterDataHandler)
	    characterDataHandler->characterData(&ch, 1);
	  else if (defaultHandler)
	    reportDefault(enc, s, next);
	  break;
	}
	name = dtd.pool.storeString(enc,
				s + enc->minBytesPerChar,
				next - enc->minBytesPerChar);
	if (!name)
	  return ERROR_NO_MEMORY;
	entity = (Entity *)dtd.generalEntities.lookup(name, 0);
	dtd.pool.discard();
	if (!entity) {
	  if (dtd.complete || dtd.standalone)
	    return ERROR_UNDEFINED_ENTITY;
	  if (defaultHandler)
	    reportDefault(enc, s, next);
	  break;
	}
	if (entity->open)
	  return ERROR_RECURSIVE_ENTITY_REF;
	if (entity->notation)
	  return ERROR_BINARY_ENTITY_REF;
	if (entity) {
	  if (entity->textPtr) {
	    Error result;
	    OpenInternalEntity openEntity;
	    if (defaultHandler && !defaultExpandInternalEntities) {
	      reportDefault(enc, s, next);
	      break;
	    }
	    entity->open = 1;
	    openEntity.next = openInternalEntities;
	    openInternalEntities = &openEntity;
	    openEntity.entity = entity;
	    openEntity.internalEventPtr = 0;
	    openEntity.internalEventEndPtr = 0;
	    result = doContent(tagLevel,
			       internalEncoding,
			       (char *)entity->textPtr,
			       (char *)(entity->textPtr + entity->textLen),
			       0);
	    entity->open = 0;
	    openInternalEntities = openEntity.next;
	    if (result)
	      return result;
	  }
	  else if (externalEntityRefHandler) {
	    const XML_Char *context;
	    entity->open = 1;
	    context = getContext();
	    entity->open = 0;
	    if (!context)
	      return ERROR_NO_MEMORY;
	    if (!externalEntityRefHandler->externalEntityRef(this,
				          context,
					  entity->base,
					  entity->systemId,
					  entity->publicId))
	      return ERROR_EXTERNAL_ENTITY_HANDLING;
	    tempPool.discard();
	  }
	  else if (defaultHandler)
	    reportDefault(enc, s, next);
	}
	break;
      }
    case XML_TOK_START_TAG_WITH_ATTS:
      if (!elementHandler) {
	Error result = storeAtts(enc, s, 0, 0);
	if (result)
	  return result;
      }
      /* fall through */
    case XML_TOK_START_TAG_NO_ATTS:
      {
	Tag *tag;
	if (freeTagList) {
	  tag = freeTagList;
	  freeTagList = freeTagList->parent;
	}
	else {
	  tag = (Tag *)malloc(sizeof(Tag));
	  if (!tag)
	    return ERROR_NO_MEMORY;
	  tag->buf = (char *)malloc(INIT_TAG_BUF_SIZE);
	  if (!tag->buf)
	    return ERROR_NO_MEMORY;
	  tag->bufEnd = tag->buf + INIT_TAG_BUF_SIZE;
	}
	tag->bindings = 0;
	tag->parent = tagStack;
	tagStack = tag;
	tag->name.localPart = 0;
	tag->rawName = s + enc->minBytesPerChar;
	tag->rawNameLength = XmlNameLength(enc, tag->rawName);
	if (nextPtr) {
	  /* Need to guarantee that:
	     tag->buf + ROUND_UP(tag->rawNameLength, sizeof(XML_Char)) <= tag->bufEnd - sizeof(XML_Char) */
	  if (tag->rawNameLength + (int)(sizeof(XML_Char) - 1) + (int)sizeof(XML_Char) > tag->bufEnd - tag->buf) {
	    int bufSize = tag->rawNameLength * 4;
	    bufSize = ROUND_UP(bufSize, sizeof(XML_Char));
	    tag->buf = (char *)realloc(tag->buf, bufSize);
	    if (!tag->buf)
	      return ERROR_NO_MEMORY;
	    tag->bufEnd = tag->buf + bufSize;
	  }
	  memcpy(tag->buf, tag->rawName, tag->rawNameLength);
	  tag->rawName = tag->buf;
	}
	++tagLevel;
	if (elementHandler) {
	  Error result;
	  XML_Char *toPtr;
	  for (;;) {
	    const char *rawNameEnd = tag->rawName + tag->rawNameLength;
	    const char *fromPtr = tag->rawName;
	    int bufSize;
	    if (nextPtr)
	      toPtr = (XML_Char *)(tag->buf + ROUND_UP(tag->rawNameLength, sizeof(XML_Char)));
	    else
	      toPtr = (XML_Char *)tag->buf;
	    tag->name.str = toPtr;
	    XmlConvert(enc,
		       &fromPtr, rawNameEnd,
		       (ICHAR **)&toPtr, (ICHAR *)tag->bufEnd - 1);
	    if (fromPtr == rawNameEnd)
	      break;
	    bufSize = (tag->bufEnd - tag->buf) << 1;
	    tag->buf = (char *)realloc(tag->buf, bufSize);
	    if (!tag->buf)
	      return ERROR_NO_MEMORY;
	    tag->bufEnd = tag->buf + bufSize;
	    if (nextPtr)
	      tag->rawName = tag->buf;
	  }
	  *toPtr = XML_T('\0');
	  result = storeAtts(enc, s, &(tag->name), &(tag->bindings));
	  if (result)
	    return result;
	  elementHandler->startElement(tag->name.str, (const XML_Char **)atts);
	  tempPool.clear();
	}
	else {
	  tag->name.str = 0;
	  if (defaultHandler)
	    reportDefault(enc, s, next);
	}
	break;
      }
    case XML_TOK_EMPTY_ELEMENT_WITH_ATTS:
      if (!elementHandler) {
	Error result = storeAtts(enc, s, 0, 0);
	if (result)
	  return result;
      }
      /* fall through */
    case XML_TOK_EMPTY_ELEMENT_NO_ATTS:
      if (elementHandler) {
	const char *rawName = s + enc->minBytesPerChar;
	Error result;
	Binding *bindings = 0;
	TagName name;
	name.str = tempPool.storeString(enc, rawName,
				   rawName + XmlNameLength(enc, rawName));
	if (!name.str)
	  return ERROR_NO_MEMORY;
	tempPool.finish();
	result = storeAtts(enc, s, &name, &bindings);
	if (result)
	  return result;
	tempPool.finish();
	if (elementHandler) {
	  elementHandler->startElement(name.str, (const XML_Char **)atts);
	  *eventPP = *eventEndPP;
	  elementHandler->endElement(name.str);
	}
	tempPool.clear();
	while (bindings) {
	  Binding *b = bindings;
	  if (namespaceDeclHandler)
	    namespaceDeclHandler->endNamespaceDecl(b->prefix->name);
	  bindings = bindings->nextTagBinding;
	  b->nextTagBinding = freeBindingList;
	  freeBindingList = b;
	  b->prefix->binding = b->prevPrefixBinding;
	}
      }
      else if (defaultHandler)
	reportDefault(enc, s, next);
      if (tagLevel == 0)
	return epilogProcessor(next, end, nextPtr);
      break;
    case XML_TOK_END_TAG:
      if (tagLevel == startTagLevel)
        return ERROR_ASYNC_ENTITY;
      else {
	int len;
	const char *rawName;
	Tag *tag = tagStack;
	tagStack = tag->parent;
	tag->parent = freeTagList;
	freeTagList = tag;
	rawName = s + enc->minBytesPerChar*2;
	len = XmlNameLength(enc, rawName);
	if (len != tag->rawNameLength
	    || memcmp(tag->rawName, rawName, len) != 0) {
	  *eventPP = rawName;
	  return ERROR_TAG_MISMATCH;
	}
	--tagLevel;
	if (elementHandler && tag->name.str) {
	  if (tag->name.localPart) {
	    XML_Char *to = (XML_Char *)tag->name.str + tag->name.uriLen;
	    const XML_Char *from = tag->name.localPart;
	    while ((*to++ = *from++) != 0)
	      ;
	  }
	  elementHandler->endElement(tag->name.str);
	}
	else if (defaultHandler)
	  reportDefault(enc, s, next);
	while (tag->bindings) {
	  Binding *b = tag->bindings;
	  if (namespaceDeclHandler)
	    namespaceDeclHandler->endNamespaceDecl(b->prefix->name);
	  tag->bindings = tag->bindings->nextTagBinding;
	  b->nextTagBinding = freeBindingList;
	  freeBindingList = b;
	  b->prefix->binding = b->prevPrefixBinding;
	}
	if (tagLevel == 0)
	  return epilogProcessor(next, end, nextPtr);
      }
      break;
    case XML_TOK_CHAR_REF:
      {
	int n = XmlCharRefNumber(enc, s);
	if (n < 0)
	  return ERROR_BAD_CHAR_REF;
	if (characterDataHandler) {
	  XML_Char buf[XML_ENCODE_MAX];
	  characterDataHandler->characterData(buf, XmlEncode(n, (ICHAR *)buf));
	}
	else if (defaultHandler)
	  reportDefault(enc, s, next);
      }
      break;
    case XML_TOK_XML_DECL:
      return ERROR_MISPLACED_XML_PI;
    case XML_TOK_DATA_NEWLINE:
      if (characterDataHandler) {
	XML_Char c = 0xA;
	characterDataHandler->characterData(&c, 1);
      }
      else if (defaultHandler)
	reportDefault(enc, s, next);
      break;
    case XML_TOK_CDATA_SECT_OPEN:
      {
	Error result;
	if (cdataSectionHandler)
  	  cdataSectionHandler->startCdataSection();
#if 0
	/* Suppose you doing a transformation on a document that involves
	   changing only the character data.  You set up a defaultHandler
	   and a characterDataHandler.  The defaultHandler simply copies
	   characters through.  The characterDataHandler does the transformation
	   and writes the characters out escaping them as necessary.  This case
	   will fail to work if we leave out the following two lines (because &
	   and < inside CDATA sections will be incorrectly escaped).

	   However, now we have a start/endCdataSectionHandler, so it seems
	   easier to let the user deal with this. */

	else if (characterDataHandler)
  	  characterDataHandler->characterData(dataBuf, 0);
#endif
	else if (defaultHandler)
	  reportDefault(enc, s, next);
	result = doCdataSection(enc, &next, end, nextPtr);
	if (!next) {
	  processor = cdataSectionProcessor;
	  return result;
	}
      }
      break;
    case XML_TOK_TRAILING_RSQB:
      if (nextPtr) {
	*nextPtr = s;
	return ERROR_NONE;
      }
      if (characterDataHandler) {
	if (MUST_CONVERT(enc, s)) {
	  ICHAR *dataPtr = (ICHAR *)dataBuf;
	  XmlConvert(enc, &s, end, &dataPtr, (ICHAR *)dataBufEnd);
	  characterDataHandler->characterData(dataBuf, dataPtr - (ICHAR *)dataBuf);
	}
	else
	  characterDataHandler->characterData((XML_Char *)s,
					      (XML_Char *)end - (XML_Char *)s);
      }
      else if (defaultHandler)
	reportDefault(enc, s, end);
      if (startTagLevel == 0) {
        *eventPP = end;
	return ERROR_NO_ELEMENTS;
      }
      if (tagLevel != startTagLevel) {
	*eventPP = end;
	return ERROR_ASYNC_ENTITY;
      }
      return ERROR_NONE;
    case XML_TOK_DATA_CHARS:
      if (characterDataHandler) {
	if (MUST_CONVERT(enc, s)) {
	  for (;;) {
	    ICHAR *dataPtr = (ICHAR *)dataBuf;
	    XmlConvert(enc, &s, next, &dataPtr, (ICHAR *)dataBufEnd);
	    *eventEndPP = s;
	    characterDataHandler->characterData(dataBuf, dataPtr - (ICHAR *)dataBuf);
	    if (s == next)
	      break;
	    *eventPP = s;
	  }
	}
	else
	  characterDataHandler->characterData((XML_Char *)s,
					      (XML_Char *)next - (XML_Char *)s);
      }
      else if (defaultHandler)
	reportDefault(enc, s, next);
      break;
    case XML_TOK_PI:
      if (!reportProcessingInstruction(enc, s, next))
	return ERROR_NO_MEMORY;
      break;
    case XML_TOK_COMMENT:
      if (!reportComment(enc, s, next))
	return ERROR_NO_MEMORY;
      break;
    default:
      if (defaultHandler)
	reportDefault(enc, s, next);
      break;
    }
    *eventPP = s = next;
  }
  /* not reached */
}

/* If tagNamePtr is non-null, build a real list of attributes,
otherwise just check the attributes for well-formedness. */

XML_Parser::Error
XML_ParserImpl::storeAtts(const ENCODING *enc,
			  const char *attStr, TagName *tagNamePtr,
			  Binding **bindingsPtr)
{
  ElementType *elementType = 0;
  int nDefaultAtts = 0;
  const XML_Char **appAtts;   /* the attribute list to pass to the application */
  int attIndex = 0;
  int i;
  int n;
  int nPrefixes = 0;
  Binding *binding;
  const XML_Char *localPart;

  /* lookup the element type name */
  if (tagNamePtr) {
    elementType = (ElementType *)dtd.elementTypes.lookup(tagNamePtr->str, 0);
    if (!elementType) {
      tagNamePtr->str = dtd.pool.copyString(tagNamePtr->str);
      if (!tagNamePtr->str)
	return ERROR_NO_MEMORY;
      elementType = (ElementType *)dtd.elementTypes.lookup(tagNamePtr->str, sizeof(ElementType));
      if (!elementType)
        return ERROR_NO_MEMORY;
      if (ns && !setElementTypePrefix(elementType))
        return ERROR_NO_MEMORY;
    }
    nDefaultAtts = elementType->nDefaultAtts;
  }
  /* get the attributes from the tokenizer */
  n = XmlGetAttributes(enc, attStr, attsSize, atts);
  if (n + nDefaultAtts > attsSize) {
    int oldAttsSize = attsSize;
    attsSize = n + nDefaultAtts + INIT_ATTS_SIZE;
    atts = (ATTRIBUTE *)realloc((void *)atts, attsSize * sizeof(ATTRIBUTE));
    if (!atts)
      return ERROR_NO_MEMORY;
    if (n > oldAttsSize)
      XmlGetAttributes(enc, attStr, n, atts);
  }
  appAtts = (const XML_Char **)atts;
  for (i = 0; i < n; i++) {
    /* add the name and value to the attribute list */
    AttributeId *attId = getAttributeId(enc, atts[i].name,
					 atts[i].name
					 + XmlNameLength(enc, atts[i].name));
    if (!attId)
      return ERROR_NO_MEMORY;
    /* detect duplicate attributes */
    if ((attId->name)[-1]) {
      if (enc == encoding)
	eventPtr = atts[i].name;
      return ERROR_DUPLICATE_ATTRIBUTE;
    }
    (attId->name)[-1] = 1;
    appAtts[attIndex++] = attId->name;
    if (!atts[i].normalized) {
      Error result;
      int isCdata = 1;

      /* figure out whether declared as other than CDATA */
      if (attId->maybeTokenized) {
	int j;
	for (j = 0; j < nDefaultAtts; j++) {
	  if (attId == elementType->defaultAtts[j].id) {
	    isCdata = elementType->defaultAtts[j].isCdata;
	    break;
	  }
	}
      }
      /* normalize the attribute value */
      result = storeAttributeValue(enc, isCdata,
				   atts[i].valuePtr, atts[i].valueEnd,
			           tempPool);
      if (result)
	return result;
      if (tagNamePtr) {
	appAtts[attIndex] = tempPool.start();
	tempPool.finish();
      }
      else
	tempPool.discard();
    }
    else if (tagNamePtr) {
      appAtts[attIndex] = tempPool.storeString(enc, atts[i].valuePtr, atts[i].valueEnd);
      if (appAtts[attIndex] == 0)
	return ERROR_NO_MEMORY;
      tempPool.finish();
    }
    /* handle prefixed attribute names */
    if (attId->prefix && tagNamePtr) {
      if (attId->xmlns) {
	/* deal with namespace declarations here */
        if (!addBinding(attId->prefix, attId, appAtts[attIndex], bindingsPtr))
          return ERROR_NO_MEMORY;
        --attIndex;
      }
      else {
	/* deal with other prefixed names later */
        attIndex++;
        nPrefixes++;
        (attId->name)[-1] = 2;
      }
    }
    else
      attIndex++;
  }
  nSpecifiedAtts = attIndex;
  /* do attribute defaulting */
  if (tagNamePtr) {
    int j;
    for (j = 0; j < nDefaultAtts; j++) {
      const DefaultAttribute *da = elementType->defaultAtts + j;
      if (!(da->id->name)[-1] && da->value) {
        if (da->id->prefix) {
          if (da->id->xmlns) {
	    if (!addBinding(da->id->prefix, da->id, da->value, bindingsPtr))
	      return ERROR_NO_MEMORY;
	  }
          else {
	    (da->id->name)[-1] = 2;
	    nPrefixes++;
  	    appAtts[attIndex++] = da->id->name;
	    appAtts[attIndex++] = da->value;
	  }
	}
	else {
	  (da->id->name)[-1] = 1;
	  appAtts[attIndex++] = da->id->name;
	  appAtts[attIndex++] = da->value;
	}
      }
    }
    appAtts[attIndex] = 0;
  }
  i = 0;
  if (nPrefixes) {
    /* expand prefixed attribute names */
    for (; i < attIndex; i += 2) {
      if (appAtts[i][-1] == 2) {
        AttributeId *id;
        ((XML_Char *)(appAtts[i]))[-1] = 0;
	id = (AttributeId *)dtd.attributeIds.lookup(appAtts[i], 0);
	if (id->prefix->binding) {
	  int j;
	  const Binding *b = id->prefix->binding;
	  const XML_Char *s = appAtts[i];
	  for (j = 0; j < b->uriLen; j++) {
	    if (!tempPool.appendChar(b->uri[j]))
	      return ERROR_NO_MEMORY;
	  }
	  while (*s++ != ':')
	    ;
	  do {
	    if (!tempPool.appendChar(*s))
	      return ERROR_NO_MEMORY;
	  } while (*s++);
	  appAtts[i] = tempPool.start();
	  tempPool.finish();
	}
	if (!--nPrefixes)
	  break;
      }
      else
	((XML_Char *)(appAtts[i]))[-1] = 0;
    }
  }
  /* clear the flags that say whether attributes were specified */
  for (; i < attIndex; i += 2)
    ((XML_Char *)(appAtts[i]))[-1] = 0;
  if (!tagNamePtr)
    return ERROR_NONE;
  for (binding = *bindingsPtr; binding; binding = binding->nextTagBinding)
    binding->attId->name[-1] = 0;
  /* expand the element type name */
  if (elementType->prefix) {
    binding = elementType->prefix->binding;
    if (!binding)
      return ERROR_NONE;
    localPart = tagNamePtr->str;
    while (*localPart++ != XML_T(':'))
      ;
  }
  else if (dtd.defaultPrefix.binding) {
    binding = dtd.defaultPrefix.binding;
    localPart = tagNamePtr->str;
  }
  else
    return ERROR_NONE;
  tagNamePtr->localPart = localPart;
  tagNamePtr->uriLen = binding->uriLen;
  i = binding->uriLen;
  do {
    if (i == binding->uriAlloc) {
      binding->uri = (XML_Char *)realloc(binding->uri, (binding->uriAlloc *= 2) * sizeof(XML_Char));
      if (!binding->uri)
	return ERROR_NO_MEMORY;
    }
    binding->uri[i++] = *localPart;
  } while (*localPart++);
  tagNamePtr->str = binding->uri;
  return ERROR_NONE;
}

int XML_ParserImpl::addBinding(Prefix *prefix, const AttributeId *attId, const XML_Char *uri, Binding **bindingsPtr)
{
  Binding *b;
  int len;
  for (len = 0; uri[len]; len++)
    ;
  if (namespaceSeparator)
    len++;
  if (freeBindingList) {
    b = freeBindingList;
    if (len > b->uriAlloc) {
      b->uri = (XML_Char *)realloc(b->uri, sizeof(XML_Char) * (len + EXPAND_SPARE));
      if (!b->uri)
	return 0;
      b->uriAlloc = len + EXPAND_SPARE;
    }
    freeBindingList = b->nextTagBinding;
  }
  else {
    b = (Binding *)malloc(sizeof(Binding));
    if (!b)
      return 0;
    b->uri = (XML_Char *)malloc(sizeof(XML_Char) * (len + EXPAND_SPARE));
    if (!b->uri) {
      free(b);
      return 0;
    }
    b->uriAlloc = len + EXPAND_SPARE;
  }
  b->uriLen = len;
  memcpy(b->uri, uri, len * sizeof(XML_Char));
  if (namespaceSeparator)
    b->uri[len - 1] = namespaceSeparator;
  b->prefix = prefix;
  b->attId = attId;
  b->prevPrefixBinding = prefix->binding;
  if (*uri == XML_T('\0') && prefix == &dtd.defaultPrefix)
    prefix->binding = 0;
  else
    prefix->binding = b;
  b->nextTagBinding = *bindingsPtr;
  *bindingsPtr = b;
  if (namespaceDeclHandler)
    namespaceDeclHandler->startNamespaceDecl(prefix->name,
					     prefix->binding ? uri : 0);
  return 1;
}

/* The idea here is to avoid using stack for each CDATA section when
the whole file is parsed with one call. */

XML_Parser::Error
XML_ParserImpl::cdataSectionProcessor(const char *start,
				      const char *end,
				      const char **endPtr)
{
  Error result = doCdataSection(encoding, &start, end, endPtr);
  if (start) {
    processor = contentProcessor;
    return contentProcessor(start, end, endPtr);
  }
  return result;
}

/* startPtr gets set to non-null is the section is closed, and to null if
the section is not yet closed. */

XML_Parser::Error
XML_ParserImpl::doCdataSection(const ENCODING *enc,
			       const char **startPtr,
			       const char *end,
			       const char **nextPtr)
{
  const char *s = *startPtr;
  const char **eventPP;
  const char **eventEndPP;
  if (enc == encoding) {
    eventPP = &eventPtr;
    *eventPP = s;
    eventEndPP = &eventEndPtr;
  }
  else {
    eventPP = &(openInternalEntities->internalEventPtr);
    eventEndPP = &(openInternalEntities->internalEventEndPtr);
  }
  *eventPP = s;
  *startPtr = 0;
  for (;;) {
    const char *next;
    int tok = XmlCdataSectionTok(enc, s, end, &next);
    *eventEndPP = next;
    switch (tok) {
    case XML_TOK_CDATA_SECT_CLOSE:
      if (cdataSectionHandler)
	cdataSectionHandler->endCdataSection();
#if 0
      /* see comment under XML_TOK_CDATA_SECT_OPEN */
      else if (characterDataHandler)
	characterDataHandler->characterData(dataBuf, 0);
#endif
      else if (defaultHandler)
	reportDefault(enc, s, next);
      *startPtr = next;
      return ERROR_NONE;
    case XML_TOK_DATA_NEWLINE:
      if (characterDataHandler) {
	XML_Char c = 0xA;
	characterDataHandler->characterData(&c, 1);
      }
      else if (defaultHandler)
	reportDefault(enc, s, next);
      break;
    case XML_TOK_DATA_CHARS:
      if (characterDataHandler) {
	if (MUST_CONVERT(enc, s)) {
	  for (;;) {
  	    ICHAR *dataPtr = (ICHAR *)dataBuf;
	    XmlConvert(enc, &s, next, &dataPtr, (ICHAR *)dataBufEnd);
	    *eventEndPP = next;
	    characterDataHandler->characterData(dataBuf, dataPtr - (ICHAR *)dataBuf);
	    if (s == next)
	      break;
	    *eventPP = s;
	  }
	}
	else
	  characterDataHandler->characterData((XML_Char *)s,
					      (XML_Char *)next - (XML_Char *)s);
      }
      else if (defaultHandler)
	reportDefault(enc, s, next);
      break;
    case XML_TOK_INVALID:
      *eventPP = next;
      return ERROR_INVALID_TOKEN;
    case XML_TOK_PARTIAL_CHAR:
      if (nextPtr) {
	*nextPtr = s;
	return ERROR_NONE;
      }
      return ERROR_PARTIAL_CHAR;
    case XML_TOK_PARTIAL:
    case XML_TOK_NONE:
      if (nextPtr) {
	*nextPtr = s;
	return ERROR_NONE;
      }
      return ERROR_UNCLOSED_CDATA_SECTION;
    default:
      abort();
    }
    *eventPP = s = next;
  }
  /* not reached */
}

#ifdef XML_DTD

/* The idea here is to avoid using stack for each IGNORE section when
the whole file is parsed with one call. */

XML_Parser::Error
XML_ParserImpl::ignoreSectionProcessor(const char *start,
				       const char *end,
				       const char **endPtr)
{
  Error result = doIgnoreSection(encoding, &start, end, endPtr);
  if (start) {
    processor = prologProcessor;
    return prologProcessor(start, end, endPtr);
  }
  return result;
}

/* startPtr gets set to non-null is the section is closed, and to null if
the section is not yet closed. */

XML_Parser::Error
XML_ParserImpl::doIgnoreSection(const ENCODING *enc,
				const char **startPtr,
				const char *end,
				const char **nextPtr)
{
  const char *next;
  int tok;
  const char *s = *startPtr;
  const char **eventPP;
  const char **eventEndPP;
  if (enc == encoding) {
    eventPP = &eventPtr;
    *eventPP = s;
    eventEndPP = &eventEndPtr;
  }
  else {
    eventPP = &(openInternalEntities->internalEventPtr);
    eventEndPP = &(openInternalEntities->internalEventEndPtr);
  }
  *eventPP = s;
  *startPtr = 0;
  tok = XmlIgnoreSectionTok(enc, s, end, &next);
  *eventEndPP = next;
  switch (tok) {
  case XML_TOK_IGNORE_SECT:
    if (defaultHandler)
      reportDefault(enc, s, next);
    *startPtr = next;
    return ERROR_NONE;
  case XML_TOK_INVALID:
    *eventPP = next;
    return ERROR_INVALID_TOKEN;
  case XML_TOK_PARTIAL_CHAR:
    if (nextPtr) {
      *nextPtr = s;
      return ERROR_NONE;
    }
    return ERROR_PARTIAL_CHAR;
  case XML_TOK_PARTIAL:
  case XML_TOK_NONE:
    if (nextPtr) {
      *nextPtr = s;
      return ERROR_NONE;
    }
    return ERROR_SYNTAX; /* ERROR_UNCLOSED_IGNORE_SECTION */
  default:
    abort();
  }
  /* not reached */
}

#endif /* XML_DTD */

XML_Parser::Error
XML_ParserImpl::initializeEncoding()
{
  const char *s;
#ifdef XML_UNICODE
  char encodingBuf[128];
  if (!protocolEncodingName)
    s = 0;
  else {
    int i;
    for (i = 0; protocolEncodingName[i]; i++) {
      if (i == sizeof(encodingBuf) - 1
	  || protocolEncodingName[i] >= 0x80
	  || protocolEncodingName[i] < 0) {
	encodingBuf[0] = '\0';
	break;
      }
      encodingBuf[i] = (char)protocolEncodingName[i];
    }
    encodingBuf[i] = '\0';
    s = encodingBuf;
  }
#else
  s = protocolEncodingName;
#endif
  if ((ns ? XmlInitEncodingNS : XmlInitEncoding)(&initEncoding, &encoding, s))
    return ERROR_NONE;
  return handleUnknownEncoding(protocolEncodingName);
}

XML_Parser::Error
XML_ParserImpl::processXmlDecl(int isGeneralTextEntity,
			       const char *s, const char *next)
{
  const char *encodingName = 0;
  const ENCODING *newEncoding = 0;
  const char *version;
  int standalone = -1;
  if (!(ns
        ? XmlParseXmlDeclNS
	: XmlParseXmlDecl)(isGeneralTextEntity,
		           encoding,
		           s,
		           next,
		           &eventPtr,
		           &version,
		           &encodingName,
		           &newEncoding,
		           &standalone))
    return ERROR_SYNTAX;
  if (!isGeneralTextEntity && standalone == 1) {
    dtd.standalone = 1;
#ifdef XML_DTD
    if (paramEntityParsing == PARAM_ENTITY_PARSING_UNLESS_STANDALONE)
      paramEntityParsing = PARAM_ENTITY_PARSING_NEVER;
#endif /* XML_DTD */
  }
  if (defaultHandler)
    reportDefault(encoding, s, next);
  if (!protocolEncodingName) {
    if (newEncoding) {
      if (newEncoding->minBytesPerChar != encoding->minBytesPerChar) {
	eventPtr = encodingName;
	return ERROR_INCORRECT_ENCODING;
      }
      encoding = newEncoding;
    }
    else if (encodingName) {
      Error result;
      const XML_Char *s = tempPool.storeString(encoding,
					       encodingName,
					       encodingName
					       + XmlNameLength(encoding, encodingName));
      if (!s)
	return ERROR_NO_MEMORY;
      result = handleUnknownEncoding(s);
      tempPool.discard();
      if (result == ERROR_UNKNOWN_ENCODING)
	eventPtr = encodingName;
      return result;
    }
  }
  return ERROR_NONE;
}

static int convertFunction(void *userData, const char *s)
{
  return ((XML_Encoding *)userData)->convert(s);
}

XML_Parser::Error
XML_ParserImpl::handleUnknownEncoding(const XML_Char *encodingName)
{
  if (unknownEncodingHandler) {
    XML_Encoding *info = unknownEncodingHandler->unknownEncoding(encodingName);
    if (info) {
      ENCODING *enc;
      unknownEncodingMem = malloc(XmlSizeOfUnknownEncoding());
      if (!unknownEncodingMem) {
	info->release();
	return ERROR_NO_MEMORY;
      }
      enc = (ns
	     ? XmlInitUnknownEncodingNS
	     : XmlInitUnknownEncoding)(unknownEncodingMem,
				       info->map,
				       convertFunction,
				       info);
      if (enc) {
	unknownEncoding = info;
	encoding = enc;
	return ERROR_NONE;
      }
      info->release();
    }
  }
  return ERROR_UNKNOWN_ENCODING;
}

XML_Parser::Error
XML_ParserImpl::prologInitProcessor(const char *s,
				    const char *end,
				    const char **nextPtr)
{
  Error result = initializeEncoding();
  if (result != ERROR_NONE)
    return result;
  processor = prologProcessor;
  return prologProcessor(s, end, nextPtr);
}

XML_Parser::Error
XML_ParserImpl::prologProcessor(const char *s,
				const char *end,
				const char **nextPtr)
{
  const char *next;
  int tok = XmlPrologTok(encoding, s, end, &next);
  return doProlog(encoding, s, end, tok, next, nextPtr);
}

XML_Parser::Error
XML_ParserImpl::doProlog(const ENCODING *enc,
			 const char *s,
			 const char *end,
			 int tok,
			 const char *next,
			 const char **nextPtr)
{
#ifdef XML_DTD
  static const XML_Char externalSubsetName[] = { '#' , '\0' };
#endif /* XML_DTD */

  const char **eventPP;
  const char **eventEndPP;
  if (enc == encoding) {
    eventPP = &eventPtr;
    eventEndPP = &eventEndPtr;
  }
  else {
    eventPP = &(openInternalEntities->internalEventPtr);
    eventEndPP = &(openInternalEntities->internalEventEndPtr);
  }
  for (;;) {
    int role;
    *eventPP = s;
    *eventEndPP = next;
    if (tok <= 0) {
      if (nextPtr != 0 && tok != XML_TOK_INVALID) {
	*nextPtr = s;
	return ERROR_NONE;
      }
      switch (tok) {
      case XML_TOK_INVALID:
	*eventPP = next;
	return ERROR_INVALID_TOKEN;
      case XML_TOK_PARTIAL:
	return ERROR_UNCLOSED_TOKEN;
      case XML_TOK_PARTIAL_CHAR:
	return ERROR_PARTIAL_CHAR;
      case XML_TOK_NONE:
#ifdef XML_DTD
	if (enc != encoding)
	  return ERROR_NONE;
	if (parentParser) {
	  if (XmlTokenRole(&prologState, XML_TOK_NONE, end, end, enc)
	      == XML_ROLE_ERROR)
	    return ERROR_SYNTAX;
	  hadExternalDoctype = 0;
	  return ERROR_NONE;
	}
#endif /* XML_DTD */
	return ERROR_NO_ELEMENTS;
      default:
	tok = -tok;
	next = end;
	break;
      }
    }
    role = XmlTokenRole(&prologState, tok, s, next, enc);
    switch (role) {
    case XML_ROLE_XML_DECL:
      {
	Error result = processXmlDecl(0, s, next);
	if (result != ERROR_NONE)
	  return result;
	enc = encoding;
      }
      break;
    case XML_ROLE_DOCTYPE_NAME:
      if (doctypeDeclHandler) {
	const XML_Char *name = tempPool.storeString(enc, s, next);
	if (!name)
	  return ERROR_NO_MEMORY;
	doctypeDeclHandler->startDoctypeDecl(name);
	tempPool.clear();
      }
      break;
#ifdef XML_DTD
    case XML_ROLE_TEXT_DECL:
      {
	Error result = processXmlDecl(1, s, next);
	if (result != ERROR_NONE)
	  return result;
	enc = encoding;
      }
      break;
#endif /* XML_DTD */
    case XML_ROLE_DOCTYPE_PUBLIC_ID:
#ifdef XML_DTD
      declEntity = (Entity *)dtd.paramEntities.lookup(externalSubsetName,
						      sizeof(Entity));
      if (!declEntity)
	return ERROR_NO_MEMORY;
#endif /* XML_DTD */
      /* fall through */
    case XML_ROLE_ENTITY_PUBLIC_ID:
      if (!XmlIsPublicId(enc, s, next, eventPP))
	return ERROR_SYNTAX;
      if (declEntity) {
	XML_Char *tem = dtd.pool.storeString(enc,
					     s + enc->minBytesPerChar,
					     next - enc->minBytesPerChar);
	if (!tem)
	  return ERROR_NO_MEMORY;
	normalizePublicId(tem);
	declEntity->publicId = tem;
	dtd.pool.finish();
      }
      break;
    case XML_ROLE_DOCTYPE_CLOSE:
      if (dtd.complete && hadExternalDoctype) {
	dtd.complete = 0;
#ifdef XML_DTD
	if (paramEntityParsing && externalEntityRefHandler) {
	  Entity *entity = (Entity *)dtd.paramEntities.lookup(externalSubsetName,
							      0);
	  if (!externalEntityRefHandler->externalEntityRef(this,
					                   0,
							   entity->base,
							   entity->systemId,
							   entity->publicId))
	   return ERROR_EXTERNAL_ENTITY_HANDLING;
	}
#endif /* XML_DTD */
	if (!dtd.complete
	    && !dtd.standalone
	    && notStandaloneHandler
	    && !notStandaloneHandler->notStandalone())
	  return ERROR_NOT_STANDALONE;
      }
      if (doctypeDeclHandler)
	doctypeDeclHandler->endDoctypeDecl();
      break;
    case XML_ROLE_INSTANCE_START:
      processor = contentProcessor;
      return contentProcessor(s, end, nextPtr);
    case XML_ROLE_ATTLIST_ELEMENT_NAME:
      {
	const XML_Char *name = dtd.pool.storeString(enc, s, next);
	if (!name)
	  return ERROR_NO_MEMORY;
	declElementType = (ElementType *)dtd.elementTypes.lookup(name, sizeof(ElementType));
	if (!declElementType)
	  return ERROR_NO_MEMORY;
	if (declElementType->name != name)
	  dtd.pool.discard();
	else {
	  dtd.pool.finish();
	  if (!setElementTypePrefix(declElementType))
            return ERROR_NO_MEMORY;
	}
	break;
      }
    case XML_ROLE_ATTRIBUTE_NAME:
      declAttributeId = getAttributeId(enc, s, next);
      if (!declAttributeId)
	return ERROR_NO_MEMORY;
      declAttributeIsCdata = 0;
      break;
    case XML_ROLE_ATTRIBUTE_TYPE_CDATA:
      declAttributeIsCdata = 1;
      break;
    case XML_ROLE_IMPLIED_ATTRIBUTE_VALUE:
    case XML_ROLE_REQUIRED_ATTRIBUTE_VALUE:
      if (dtd.complete
	  && !defineAttribute(declElementType, declAttributeId, declAttributeIsCdata, 0))
	return ERROR_NO_MEMORY;
      break;
    case XML_ROLE_DEFAULT_ATTRIBUTE_VALUE:
    case XML_ROLE_FIXED_ATTRIBUTE_VALUE:
      {
	const XML_Char *attVal;
	Error result
	  = storeAttributeValue(enc, declAttributeIsCdata,
				s + enc->minBytesPerChar,
			        next - enc->minBytesPerChar,
			        dtd.pool);
	if (result)
	  return result;
	attVal = dtd.pool.start();
	dtd.pool.finish();
	if (dtd.complete
	    && !defineAttribute(declElementType, declAttributeId, declAttributeIsCdata, attVal))
	  return ERROR_NO_MEMORY;
	break;
      }
    case XML_ROLE_ENTITY_VALUE:
      {
	Error result = storeEntityValue(enc,
						 s + enc->minBytesPerChar,
						 next - enc->minBytesPerChar);
	if (declEntity) {
	  declEntity->textPtr = dtd.pool.start();
	  declEntity->textLen = dtd.pool.length();
	  dtd.pool.finish();
	}
	else
	  dtd.pool.discard();
	if (result != ERROR_NONE)
	  return result;
      }
      break;
    case XML_ROLE_DOCTYPE_SYSTEM_ID:
      if (!dtd.standalone
#ifdef XML_DTD
	  && !paramEntityParsing
#endif /* XML_DTD */
	  && notStandaloneHandler
	  && !notStandaloneHandler->notStandalone())
	return ERROR_NOT_STANDALONE;
      hadExternalDoctype = 1;
#ifndef XML_DTD
      break;
#else /* XML_DTD */
      if (!declEntity) {
	declEntity = (Entity *)dtd.paramEntities.lookup(externalSubsetName,
							sizeof(Entity));
	if (!declEntity)
	  return ERROR_NO_MEMORY;
      }
      /* fall through */
#endif /* XML_DTD */
    case XML_ROLE_ENTITY_SYSTEM_ID:
      if (declEntity) {
	declEntity->systemId = dtd.pool.storeString(enc,
	                                       s + enc->minBytesPerChar,
	  				       next - enc->minBytesPerChar);
	if (!declEntity->systemId)
	  return ERROR_NO_MEMORY;
	declEntity->base = curBase;
	dtd.pool.finish();
      }
      break;
    case XML_ROLE_ENTITY_NOTATION_NAME:
      if (declEntity) {
	declEntity->notation = dtd.pool.storeString(enc, s, next);
	if (!declEntity->notation)
	  return ERROR_NO_MEMORY;
	dtd.pool.finish();
	if (unparsedEntityDeclHandler) {
	  *eventEndPP = s;
	  unparsedEntityDeclHandler->unparsedEntityDecl(declEntity->name,
							declEntity->base,
							declEntity->systemId,
							declEntity->publicId,
							declEntity->notation);
	}

      }
      break;
    case XML_ROLE_GENERAL_ENTITY_NAME:
      {
	const XML_Char *name;
	if (XmlPredefinedEntityName(enc, s, next)) {
	  declEntity = 0;
	  break;
	}
	name = dtd.pool.storeString(enc, s, next);
	if (!name)
	  return ERROR_NO_MEMORY;
	if (dtd.complete) {
	  declEntity = (Entity *)dtd.generalEntities.lookup(name, sizeof(Entity));
	  if (!declEntity)
	    return ERROR_NO_MEMORY;
	  if (declEntity->name != name) {
	    dtd.pool.discard();
	    declEntity = 0;
	  }
	  else
	    dtd.pool.finish();
	}
	else {
	  dtd.pool.discard();
	  declEntity = 0;
	}
      }
      break;
    case XML_ROLE_PARAM_ENTITY_NAME:
#ifdef XML_DTD
      if (dtd.complete) {
	const XML_Char *name = dtd.pool.storeString(enc, s, next);
	if (!name)
	  return ERROR_NO_MEMORY;
	declEntity = (Entity *)dtd.paramEntities.lookup(name, sizeof(Entity));
	if (!declEntity)
	  return ERROR_NO_MEMORY;
	if (declEntity->name != name) {
	  dtd.pool.discard();
	  declEntity = 0;
	}
	else
	  dtd.pool.finish();
      }
#else /* not XML_DTD */
      declEntity = 0;
#endif /* not XML_DTD */
      break;
    case XML_ROLE_NOTATION_NAME:
      declNotationPublicId = 0;
      declNotationName = 0;
      if (notationDeclHandler) {
	declNotationName = tempPool.storeString(enc, s, next);
	if (!declNotationName)
	  return ERROR_NO_MEMORY;
	tempPool.finish();
      }
      break;
    case XML_ROLE_NOTATION_PUBLIC_ID:
      if (!XmlIsPublicId(enc, s, next, eventPP))
	return ERROR_SYNTAX;
      if (declNotationName) {
	XML_Char *tem = tempPool.storeString(enc,
	                                     s + enc->minBytesPerChar,
					     next - enc->minBytesPerChar);
	if (!tem)
	  return ERROR_NO_MEMORY;
	normalizePublicId(tem);
	declNotationPublicId = tem;
	tempPool.finish();
      }
      break;
    case XML_ROLE_NOTATION_SYSTEM_ID:
      if (declNotationName && notationDeclHandler) {
	const XML_Char *systemId
	  = tempPool.storeString(enc,
			    s + enc->minBytesPerChar,
	  		    next - enc->minBytesPerChar);
	if (!systemId)
	  return ERROR_NO_MEMORY;
	*eventEndPP = s;
	notationDeclHandler->notationDecl(declNotationName,
					  curBase,
					  systemId,
					  declNotationPublicId);
      }
      tempPool.clear();
      break;
    case XML_ROLE_NOTATION_NO_SYSTEM_ID:
      if (declNotationPublicId && notationDeclHandler) {
	*eventEndPP = s;
	notationDeclHandler->notationDecl(declNotationName,
					  curBase,
					  0,
					  declNotationPublicId);
      }
      tempPool.clear();
      break;
    case XML_ROLE_ERROR:
      switch (tok) {
      case XML_TOK_PARAM_ENTITY_REF:
	return ERROR_PARAM_ENTITY_REF;
      case XML_TOK_XML_DECL:
	return ERROR_MISPLACED_XML_PI;
      default:
	return ERROR_SYNTAX;
      }
#ifdef XML_DTD
    case XML_ROLE_IGNORE_SECT:
      {
	Error result;
	if (defaultHandler)
	  reportDefault(enc, s, next);
	result = doIgnoreSection(enc, &next, end, nextPtr);
	if (!next) {
	  processor = ignoreSectionProcessor;
	  return result;
	}
      }
      break;
#endif /* XML_DTD */
    case XML_ROLE_GROUP_OPEN:
      if (prologState.level >= groupSize) {
	if (groupSize)
	  groupConnector = (char *)realloc(groupConnector, groupSize *= 2);
	else
	  groupConnector = (char *)malloc(groupSize = 32);
	if (!groupConnector)
	  return ERROR_NO_MEMORY;
      }
      groupConnector[prologState.level] = 0;
      break;
    case XML_ROLE_GROUP_SEQUENCE:
      if (groupConnector[prologState.level] == '|')
	return ERROR_SYNTAX;
      groupConnector[prologState.level] = ',';
      break;
    case XML_ROLE_GROUP_CHOICE:
      if (groupConnector[prologState.level] == ',')
	return ERROR_SYNTAX;
      groupConnector[prologState.level] = '|';
      break;
    case XML_ROLE_PARAM_ENTITY_REF:
#ifdef XML_DTD
    case XML_ROLE_INNER_PARAM_ENTITY_REF:
      if (paramEntityParsing
	  && (dtd.complete || role == XML_ROLE_INNER_PARAM_ENTITY_REF)) {
	const XML_Char *name;
	Entity *entity;
	name = dtd.pool.storeString(enc,
				s + enc->minBytesPerChar,
				next - enc->minBytesPerChar);
	if (!name)
	  return ERROR_NO_MEMORY;
	entity = (Entity *)dtd.paramEntities.lookup(name, 0);
	dtd.pool.discard();
	if (!entity) {
	  /* FIXME what to do if !dtd.complete? */
	  return ERROR_UNDEFINED_ENTITY;
	}
	if (entity->open)
	  return ERROR_RECURSIVE_ENTITY_REF;
	if (entity->textPtr) {
	  Error result;
	  result = processInternalParamEntity(entity);
	  if (result != ERROR_NONE)
	    return result;
	  break;
	}
	if (role == XML_ROLE_INNER_PARAM_ENTITY_REF)
	  return ERROR_PARAM_ENTITY_REF;
	if (externalEntityRefHandler) {
	  dtd.complete = 0;
	  entity->open = 1;
	  if (!externalEntityRefHandler->externalEntityRef(this,
	                                                   0,
							   entity->base,
							   entity->systemId,
							   entity->publicId)) {
	    entity->open = 0;
	    return ERROR_EXTERNAL_ENTITY_HANDLING;
	  }
	  entity->open = 0;
	  if (dtd.complete)
	    break;
	}
      }
#endif /* XML_DTD */
      if (!dtd.standalone
	  && notStandaloneHandler
	  && !notStandaloneHandler->notStandalone())
	return ERROR_NOT_STANDALONE;
      dtd.complete = 0;
      if (defaultHandler)
	reportDefault(enc, s, next);
      break;
    case XML_ROLE_NONE:
      switch (tok) {
      case XML_TOK_PI:
	if (!reportProcessingInstruction(enc, s, next))
	  return ERROR_NO_MEMORY;
	break;
      case XML_TOK_COMMENT:
	if (!reportComment(enc, s, next))
	  return ERROR_NO_MEMORY;
	break;
      }
      break;
    }
    if (defaultHandler) {
      switch (tok) {
      case XML_TOK_PI:
      case XML_TOK_COMMENT:
      case XML_TOK_BOM:
      case XML_TOK_XML_DECL:
#ifdef XML_DTD
      case XML_TOK_IGNORE_SECT:
#endif /* XML_DTD */
      case XML_TOK_PARAM_ENTITY_REF:
	break;
      default:
#ifdef XML_DTD
	if (role != XML_ROLE_IGNORE_SECT)
#endif /* XML_DTD */
	  reportDefault(enc, s, next);
      }
    }
    s = next;
    tok = XmlPrologTok(enc, s, end, &next);
  }
  /* not reached */
}

XML_Parser::Error
XML_ParserImpl::epilogProcessor(const char *s,
				const char *end,
				const char **nextPtr)
{
  processor = epilogProcessor;
  eventPtr = s;
  for (;;) {
    const char *next;
    int tok = XmlPrologTok(encoding, s, end, &next);
    eventEndPtr = next;
    switch (tok) {
    case -XML_TOK_PROLOG_S:
      if (defaultHandler) {
	eventEndPtr = end;
	reportDefault(encoding, s, end);
      }
      /* fall through */
    case XML_TOK_NONE:
      if (nextPtr)
	*nextPtr = end;
      return ERROR_NONE;
    case XML_TOK_PROLOG_S:
      if (defaultHandler)
	reportDefault(encoding, s, next);
      break;
    case XML_TOK_PI:
      if (!reportProcessingInstruction(encoding, s, next))
	return ERROR_NO_MEMORY;
      break;
    case XML_TOK_COMMENT:
      if (!reportComment(encoding, s, next))
	return ERROR_NO_MEMORY;
      break;
    case XML_TOK_INVALID:
      eventPtr = next;
      return ERROR_INVALID_TOKEN;
    case XML_TOK_PARTIAL:
      if (nextPtr) {
	*nextPtr = s;
	return ERROR_NONE;
      }
      return ERROR_UNCLOSED_TOKEN;
    case XML_TOK_PARTIAL_CHAR:
      if (nextPtr) {
	*nextPtr = s;
	return ERROR_NONE;
      }
      return ERROR_PARTIAL_CHAR;
    default:
      return ERROR_JUNK_AFTER_DOC_ELEMENT;
    }
    eventPtr = s = next;
  }
}

#ifdef XML_DTD

XML_Parser::Error
XML_ParserImpl::processInternalParamEntity(Entity *entity)
{
  const char *s, *end, *next;
  int tok;
  Error result;
  OpenInternalEntity openEntity;
  entity->open = 1;
  openEntity.next = openInternalEntities;
  openInternalEntities = &openEntity;
  openEntity.entity = entity;
  openEntity.internalEventPtr = 0;
  openEntity.internalEventEndPtr = 0;
  s = (char *)entity->textPtr;
  end = (char *)(entity->textPtr + entity->textLen);
  tok = XmlPrologTok(internalEncoding, s, end, &next);
  result = doProlog(internalEncoding, s, end, tok, next, 0);
  entity->open = 0;
  openInternalEntities = openEntity.next;
  return result;
}

#endif /* XML_DTD */

XML_Parser::Error
XML_ParserImpl::errorProcessor(const char *s,
			       const char *end,
			       const char **nextPtr)
{
  return errorCode;
}

XML_Parser::Error
XML_ParserImpl::storeAttributeValue(const ENCODING *enc, int isCdata,
				    const char *ptr, const char *end,
				    StringPool &pool)
{
  Error result = appendAttributeValue(enc, isCdata, ptr, end, pool);
  if (result)
    return result;
  if (!isCdata && pool.length() && pool.lastChar() == 0x20)
    pool.chop();
  if (!pool.appendChar(XML_T('\0')))
    return ERROR_NO_MEMORY;
  return ERROR_NONE;
}

XML_Parser::Error
XML_ParserImpl::appendAttributeValue(const ENCODING *enc, int isCdata,
				     const char *ptr, const char *end,
				     StringPool &pool)
{
  for (;;) {
    const char *next;
    int tok = XmlAttributeValueTok(enc, ptr, end, &next);
    switch (tok) {
    case XML_TOK_NONE:
      return ERROR_NONE;
    case XML_TOK_INVALID:
      if (enc == encoding)
	eventPtr = next;
      return ERROR_INVALID_TOKEN;
    case XML_TOK_PARTIAL:
      if (enc == encoding)
	eventPtr = ptr;
      return ERROR_INVALID_TOKEN;
    case XML_TOK_CHAR_REF:
      {
	XML_Char buf[XML_ENCODE_MAX];
	int i;
	int n = XmlCharRefNumber(enc, ptr);
	if (n < 0) {
	  if (enc == encoding)
	    eventPtr = ptr;
      	  return ERROR_BAD_CHAR_REF;
	}
	if (!isCdata
	    && n == 0x20 /* space */
	    && (pool.length() == 0 || pool.lastChar() == 0x20))
	  break;
	n = XmlEncode(n, (ICHAR *)buf);
	if (!n) {
	  if (enc == encoding)
	    eventPtr = ptr;
	  return ERROR_BAD_CHAR_REF;
	}
	for (i = 0; i < n; i++) {
	  if (!pool.appendChar(buf[i]))
	    return ERROR_NO_MEMORY;
	}
      }
      break;
    case XML_TOK_DATA_CHARS:
      if (!pool.append(enc, ptr, next))
	return ERROR_NO_MEMORY;
      break;
      break;
    case XML_TOK_TRAILING_CR:
      next = ptr + enc->minBytesPerChar;
      /* fall through */
    case XML_TOK_ATTRIBUTE_VALUE_S:
    case XML_TOK_DATA_NEWLINE:
      if (!isCdata && (pool.length() == 0 || pool.lastChar() == 0x20))
	break;
      if (!pool.appendChar(0x20))
	return ERROR_NO_MEMORY;
      break;
    case XML_TOK_ENTITY_REF:
      {
	const XML_Char *name;
	Entity *entity;
	XML_Char ch = XmlPredefinedEntityName(enc,
					      ptr + enc->minBytesPerChar,
					      next - enc->minBytesPerChar);
	if (ch) {
	  if (!pool.appendChar(ch))
  	    return ERROR_NO_MEMORY;
	  break;
	}
	name = temp2Pool.storeString(enc,
				     ptr + enc->minBytesPerChar,
				     next - enc->minBytesPerChar);
	if (!name)
	  return ERROR_NO_MEMORY;
	entity = (Entity *)dtd.generalEntities.lookup(name, 0);
	temp2Pool.discard();
	if (!entity) {
	  if (dtd.complete) {
	    if (enc == encoding)
	      eventPtr = ptr;
	    return ERROR_UNDEFINED_ENTITY;
	  }
	}
	else if (entity->open) {
	  if (enc == encoding)
	    eventPtr = ptr;
	  return ERROR_RECURSIVE_ENTITY_REF;
	}
	else if (entity->notation) {
	  if (enc == encoding)
	    eventPtr = ptr;
	  return ERROR_BINARY_ENTITY_REF;
	}
	else if (!entity->textPtr) {
	  if (enc == encoding)
	    eventPtr = ptr;
  	  return ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF;
	}
	else {
	  Error result;
	  const XML_Char *textEnd = entity->textPtr + entity->textLen;
	  entity->open = 1;
	  result = appendAttributeValue(internalEncoding, isCdata, (char *)entity->textPtr, (char *)textEnd, pool);
	  entity->open = 0;
	  if (result)
	    return result;
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

XML_Parser::Error
XML_ParserImpl::storeEntityValue(const ENCODING *enc,
				 const char *entityTextPtr,
				 const char *entityTextEnd)
{
  StringPool &pool = dtd.pool;
  for (;;) {
    const char *next;
    int tok = XmlEntityValueTok(enc, entityTextPtr, entityTextEnd, &next);
    switch (tok) {
    case XML_TOK_PARAM_ENTITY_REF:
#ifdef XML_DTD
      if (parentParser || enc != encoding) {
	Error result;
	const XML_Char *name;
	Entity *entity;
	name = tempPool.storeString(enc,
				    entityTextPtr + enc->minBytesPerChar,
				    next - enc->minBytesPerChar);
	if (!name)
	  return ERROR_NO_MEMORY;
	entity = (Entity *)dtd.paramEntities.lookup(name, 0);
	tempPool.discard();
	if (!entity) {
	  if (enc == encoding)
	    eventPtr = entityTextPtr;
	  return ERROR_UNDEFINED_ENTITY;
	}
	if (entity->open) {
	  if (enc == encoding)
	    eventPtr = entityTextPtr;
	  return ERROR_RECURSIVE_ENTITY_REF;
	}
	if (entity->systemId) {
	  if (enc == encoding)
	    eventPtr = entityTextPtr;
	  return ERROR_PARAM_ENTITY_REF;
	}
	entity->open = 1;
	result = storeEntityValue(internalEncoding,
				  (char *)entity->textPtr,
				  (char *)(entity->textPtr + entity->textLen));
	entity->open = 0;
	if (result)
	  return result;
	break;
      }
#endif /* XML_DTD */
      eventPtr = entityTextPtr;
      return ERROR_SYNTAX;
    case XML_TOK_NONE:
      return ERROR_NONE;
    case XML_TOK_ENTITY_REF:
    case XML_TOK_DATA_CHARS:
      if (!pool.append(enc, entityTextPtr, next))
	return ERROR_NO_MEMORY;
      break;
    case XML_TOK_TRAILING_CR:
      next = entityTextPtr + enc->minBytesPerChar;
      /* fall through */
    case XML_TOK_DATA_NEWLINE:
      if (!pool.appendChar(0xA))
	return ERROR_NO_MEMORY;
      break;
    case XML_TOK_CHAR_REF:
      {
	XML_Char buf[XML_ENCODE_MAX];
	int i;
	int n = XmlCharRefNumber(enc, entityTextPtr);
	if (n < 0) {
	  if (enc == encoding)
	    eventPtr = entityTextPtr;
	  return ERROR_BAD_CHAR_REF;
	}
	n = XmlEncode(n, (ICHAR *)buf);
	if (!n) {
	  if (enc == encoding)
	    eventPtr = entityTextPtr;
	  return ERROR_BAD_CHAR_REF;
	}
	for (i = 0; i < n; i++)
	  if (!pool.appendChar(buf[i]))
	    return ERROR_NO_MEMORY;
      }
      break;
    case XML_TOK_PARTIAL:
      if (enc == encoding)
	eventPtr = entityTextPtr;
      return ERROR_INVALID_TOKEN;
    case XML_TOK_INVALID:
      if (enc == encoding)
	eventPtr = next;
      return ERROR_INVALID_TOKEN;
    default:
      abort();
    }
    entityTextPtr = next;
  }
  /* not reached */
}

void XML_ParserImpl::normalizeLines(XML_Char *s)
{
  XML_Char *p;
  for (;; s++) {
    if (*s == XML_T('\0'))
      return;
    if (*s == 0xD)
      break;
  }
  p = s;
  do {
    if (*s == 0xD) {
      *p++ = 0xA;
      if (*++s == 0xA)
        s++;
    }
    else
      *p++ = *s++;
  } while (*s);
  *p = XML_T('\0');
}

int
XML_ParserImpl::reportProcessingInstruction(const ENCODING *enc, const char *start, const char *end)
{
  const XML_Char *target;
  XML_Char *data;
  const char *tem;
  if (!processingInstructionHandler) {
    if (defaultHandler)
      reportDefault(enc, start, end);
    return 1;
  }
  start += enc->minBytesPerChar * 2;
  tem = start + XmlNameLength(enc, start);
  target = tempPool.storeString(enc, start, tem);
  if (!target)
    return 0;
  tempPool.finish();
  data = tempPool.storeString(enc,
			      XmlSkipS(enc, tem),
			      end - enc->minBytesPerChar*2);
  if (!data)
    return 0;
  normalizeLines(data);
  processingInstructionHandler->processingInstruction(target, data);
  tempPool.clear();
  return 1;
}

int
XML_ParserImpl::reportComment(const ENCODING *enc, const char *start, const char *end)
{
  XML_Char *data;
  if (!commentHandler) {
    if (defaultHandler)
      reportDefault(enc, start, end);
    return 1;
  }
  data = tempPool.storeString(enc,
			      start + enc->minBytesPerChar * 4, 
			      end - enc->minBytesPerChar * 3);
  if (!data)
    return 0;
  normalizeLines(data);
  commentHandler->comment(data);
  tempPool.clear();
  return 1;
}

void
XML_ParserImpl::reportDefault(const ENCODING *enc, const char *s, const char *end)
{
  if (MUST_CONVERT(enc, s)) {
    const char **eventPP;
    const char **eventEndPP;
    if (enc == encoding) {
      eventPP = &eventPtr;
      eventEndPP = &eventEndPtr;
    }
    else {
      eventPP = &(openInternalEntities->internalEventPtr);
      eventEndPP = &(openInternalEntities->internalEventEndPtr);
    }
    do {
      ICHAR *dataPtr = (ICHAR *)dataBuf;
      XmlConvert(enc, &s, end, &dataPtr, (ICHAR *)dataBufEnd);
      *eventEndPP = s;
      defaultHandler->doDefault(dataBuf, dataPtr - (ICHAR *)dataBuf);
      *eventPP = s;
    } while (s != end);
  }
  else
    defaultHandler->doDefault((XML_Char *)s, (XML_Char *)end - (XML_Char *)s);
}


int
XML_ParserImpl::defineAttribute(ElementType *type, AttributeId *attId, int isCdata, const XML_Char *value)
{
  DefaultAttribute *att;
  if (value) {
    /* The handling of default attributes gets messed up if we have
       a default which duplicates a non-default. */
    int i;
    for (i = 0; i < type->nDefaultAtts; i++)
      if (attId == type->defaultAtts[i].id)
	return 1;
  }
  if (type->nDefaultAtts == type->allocDefaultAtts) {
    if (type->allocDefaultAtts == 0) {
      type->allocDefaultAtts = 8;
      type->defaultAtts = (DefaultAttribute *)malloc(type->allocDefaultAtts * sizeof(DefaultAttribute));
    }
    else {
      type->allocDefaultAtts *= 2;
      type->defaultAtts = (DefaultAttribute *)realloc(type->defaultAtts,
				  type->allocDefaultAtts*sizeof(DefaultAttribute));
    }
    if (!type->defaultAtts)
      return 0;
  }
  att = type->defaultAtts + type->nDefaultAtts;
  att->id = attId;
  att->value = value;
  att->isCdata = isCdata;
  if (!isCdata)
    attId->maybeTokenized = 1;
  type->nDefaultAtts += 1;
  return 1;
}

int
XML_ParserImpl::setElementTypePrefix(ElementType *elementType)
{
  const XML_Char *name;
  for (name = elementType->name; *name; name++) {
    if (*name == XML_T(':')) {
      Prefix *prefix;
      const XML_Char *s;
      for (s = elementType->name; s != name; s++) {
	if (!dtd.pool.appendChar(*s))
	  return 0;
      }
      if (!dtd.pool.appendChar(XML_T('\0')))
	return 0;
      prefix = (Prefix *)dtd.prefixes.lookup(dtd.pool.start(), sizeof(Prefix));
      if (!prefix)
	return 0;
      if (prefix->name == dtd.pool.start())
	dtd.pool.finish();
      else
	dtd.pool.discard();
      elementType->prefix = prefix;
    }
  }
  return 1;
}

AttributeId *
XML_ParserImpl::getAttributeId(const ENCODING *enc, const char *start, const char *end)
{
  AttributeId *id;
  const XML_Char *name;
  if (!dtd.pool.appendChar(XML_T('\0')))
    return 0;
  name = dtd.pool.storeString(enc, start, end);
  if (!name)
    return 0;
  ++name;
  id = (AttributeId *)dtd.attributeIds.lookup(name, sizeof(AttributeId));
  if (!id)
    return 0;
  if (id->name != name)
    dtd.pool.discard();
  else {
    dtd.pool.finish();
    if (!ns)
      ;
    else if (name[0] == 'x'
	     && name[1] == 'm'
	     && name[2] == 'l'
	     && name[3] == 'n'
	     && name[4] == 's'
	     && (name[5] == XML_T('\0') || name[5] == XML_T(':'))) {
      if (name[5] == '\0')
	id->prefix = &dtd.defaultPrefix;
      else
	id->prefix = (Prefix *)dtd.prefixes.lookup(name + 6, sizeof(Prefix));
      id->xmlns = 1;
    }
    else {
      int i;
      for (i = 0; name[i]; i++) {
	if (name[i] == XML_T(':')) {
	  int j;
	  for (j = 0; j < i; j++) {
	    if (!dtd.pool.appendChar(name[j]))
	      return 0;
	  }
	  if (!dtd.pool.appendChar(XML_T('\0')))
	    return 0;
	  id->prefix = (Prefix *)dtd.prefixes.lookup(dtd.pool.start(), sizeof(Prefix));
	  if (id->prefix->name == dtd.pool.start())
	    dtd.pool.finish();
	  else
	    dtd.pool.discard();
	  break;
	}
      }
    }
  }
  return id;
}

const XML_Char CONTEXT_SEP = XML_T('\f');

const XML_Char *XML_ParserImpl::getContext()
{
  HashTableIter iter;
  int needSep = 0;

  if (dtd.defaultPrefix.binding) {
    int i;
    int len;
    if (!tempPool.appendChar(XML_T('=')))
      return 0;
    len = dtd.defaultPrefix.binding->uriLen;
    if (namespaceSeparator != XML_T('\0'))
      len--;
    for (i = 0; i < len; i++)
      if (!tempPool.appendChar(dtd.defaultPrefix.binding->uri[i]))
  	return 0;
    needSep = 1;
  }

  iter.init(dtd.prefixes);
  for (;;) {
    int i;
    int len;
    const XML_Char *s;
    Prefix *prefix = (Prefix *)iter.next();
    if (!prefix)
      break;
    if (!prefix->binding)
      continue;
    if (needSep && !tempPool.appendChar(CONTEXT_SEP))
      return 0;
    for (s = prefix->name; *s; s++)
      if (!tempPool.appendChar(*s))
        return 0;
    if (!tempPool.appendChar(XML_T('=')))
      return 0;
    len = prefix->binding->uriLen;
    if (namespaceSeparator != XML_T('\0'))
      len--;
    for (i = 0; i < len; i++)
      if (!tempPool.appendChar(prefix->binding->uri[i]))
        return 0;
    needSep = 1;
  }


  iter.init(dtd.generalEntities);
  for (;;) {
    const XML_Char *s;
    Entity *e = (Entity *)iter.next();
    if (!e)
      break;
    if (!e->open)
      continue;
    if (needSep && !tempPool.appendChar(CONTEXT_SEP))
      return 0;
    for (s = e->name; *s; s++)
      if (!tempPool.appendChar(*s))
        return 0;
    needSep = 1;
  }

  if (!tempPool.appendChar(XML_T('\0')))
    return 0;
  return tempPool.start();
}

int XML_ParserImpl::setContext(const XML_Char *context)
{
  const XML_Char *s = context;

  while (*context != XML_T('\0')) {
    if (*s == CONTEXT_SEP || *s == XML_T('\0')) {
      Entity *e;
      if (!tempPool.appendChar(XML_T('\0')))
	return 0;
      e = (Entity *)dtd.generalEntities.lookup(tempPool.start(), 0);
      if (e)
	e->open = 1;
      if (*s != XML_T('\0'))
	s++;
      context = s;
      tempPool.discard();
    }
    else if (*s == '=') {
      Prefix *prefix;
      if (tempPool.length() == 0)
	prefix = &dtd.defaultPrefix;
      else {
	if (!tempPool.appendChar(XML_T('\0')))
	  return 0;
	prefix = (Prefix *)dtd.prefixes.lookup(tempPool.start(), sizeof(Prefix));
	if (!prefix)
	  return 0;
        if (prefix->name == tempPool.start())
          tempPool.finish();
        else
	  tempPool.discard();
      }
      for (context = s + 1; *context != CONTEXT_SEP && *context != XML_T('\0'); context++)
        if (!tempPool.appendChar(*context))
          return 0;
      if (!tempPool.appendChar(XML_T('\0')))
	return 0;
      if (!addBinding(prefix, 0, tempPool.start(), &inheritedBindings))
	return 0;
      tempPool.discard();
      if (*context != XML_T('\0'))
	++context;
      s = context;
    }
    else {
      if (!tempPool.appendChar(*s))
	return 0;
      s++;
    }
  }
  return 1;
}


void XML_ParserImpl::normalizePublicId(XML_Char *publicId)
{
  XML_Char *p = publicId;
  XML_Char *s;
  for (s = publicId; *s; s++) {
    switch (*s) {
    case 0x20:
    case 0xD:
    case 0xA:
      if (p != publicId && p[-1] != 0x20)
	*p++ = 0x20;
      break;
    default:
      *p++ = *s;
    }
  }
  if (p != publicId && p[-1] == 0x20)
    --p;
  *p = XML_T('\0');
}

Dtd::Dtd()
{
  complete = 1;
  standalone = 0;
  defaultPrefix.name = 0;
  defaultPrefix.binding = 0;
}

#ifdef XML_DTD

void Dtd::swap(Dtd &d1, Dtd &d2)
{
  Dtd tem;
  memcpy(&tem, &d1, sizeof(Dtd));
  memcpy(&d1, &d2, sizeof(Dtd));
  memcpy(&d2, &tem, sizeof(Dtd));
}

#endif /* XML_DTD */

Dtd::~Dtd()
{
  HashTableIter iter;
  iter.init(elementTypes);
  for (;;) {
    ElementType *e = (ElementType *)iter.next();
    if (!e)
      break;
    if (e->allocDefaultAtts != 0)
      free(e->defaultAtts);
  }
}

/* Do a deep copy of the Dtd.  Return 0 for out of memory; non-zero otherwise.
The new Dtd has already been initialized. */

int Dtd::copy(Dtd &newDtd, const Dtd &oldDtd)
{
  HashTableIter iter;

  /* Copy the prefix table. */

  iter.init(oldDtd.prefixes);
  for (;;) {
    const XML_Char *name;
    const Prefix *oldP = (Prefix *)iter.next();
    if (!oldP)
      break;
    name = newDtd.pool.copyString(oldP->name);
    if (!name)
      return 0;
    if (!newDtd.prefixes.lookup(name, sizeof(Prefix)))
      return 0;
  }

  iter.init(oldDtd.attributeIds);

  /* Copy the attribute id table. */

  for (;;) {
    AttributeId *newA;
    const XML_Char *name;
    const AttributeId *oldA = (AttributeId *)iter.next();

    if (!oldA)
      break;
    /* Remember to allocate the scratch byte before the name. */
    if (!newDtd.pool.appendChar(XML_T('\0')))
      return 0;
    name = newDtd.pool.copyString(oldA->name);
    if (!name)
      return 0;
    ++name;
    newA = (AttributeId *)newDtd.attributeIds.lookup(name, sizeof(AttributeId));
    if (!newA)
      return 0;
    newA->maybeTokenized = oldA->maybeTokenized;
    if (oldA->prefix) {
      newA->xmlns = oldA->xmlns;
      if (oldA->prefix == &oldDtd.defaultPrefix)
	newA->prefix = &newDtd.defaultPrefix;
      else
	newA->prefix = (Prefix *)newDtd.prefixes.lookup(oldA->prefix->name, 0);
    }
  }

  /* Copy the element type table. */

  iter.init(oldDtd.elementTypes);

  for (;;) {
    int i;
    ElementType *newE;
    const XML_Char *name;
    const ElementType *oldE = (ElementType *)iter.next();
    if (!oldE)
      break;
    name = newDtd.pool.copyString(oldE->name);
    if (!name)
      return 0;
    newE = (ElementType *)newDtd.elementTypes.lookup(name, sizeof(ElementType));
    if (!newE)
      return 0;
    if (oldE->nDefaultAtts) {
      newE->defaultAtts = (DefaultAttribute *)malloc(oldE->nDefaultAtts * sizeof(DefaultAttribute));
      if (!newE->defaultAtts)
	return 0;
    }
    newE->allocDefaultAtts = newE->nDefaultAtts = oldE->nDefaultAtts;
    if (oldE->prefix)
      newE->prefix = (Prefix *)newDtd.prefixes.lookup(oldE->prefix->name, 0);
    for (i = 0; i < newE->nDefaultAtts; i++) {
      newE->defaultAtts[i].id = (AttributeId *)newDtd.attributeIds.lookup(oldE->defaultAtts[i].id->name, 0);
      newE->defaultAtts[i].isCdata = oldE->defaultAtts[i].isCdata;
      if (oldE->defaultAtts[i].value) {
	newE->defaultAtts[i].value = newDtd.pool.copyString(oldE->defaultAtts[i].value);
	if (!newE->defaultAtts[i].value)
  	  return 0;
      }
      else
	newE->defaultAtts[i].value = 0;
    }
  }

  /* Copy the entity tables. */
  if (!copyEntityTable(newDtd.generalEntities,
		       newDtd.pool,
		       oldDtd.generalEntities))
      return 0;

#ifdef XML_DTD
  if (!copyEntityTable(newDtd.paramEntities,
		       newDtd.pool,
		       oldDtd.paramEntities))
      return 0;
#endif /* XML_DTD */

  newDtd.complete = oldDtd.complete;
  newDtd.standalone = oldDtd.standalone;
  return 1;
}

int Dtd::copyEntityTable(HashTable &newTable,
			 StringPool &newPool,
			 const HashTable &oldTable)
{
  HashTableIter iter;
  const XML_Char *cachedOldBase = 0;
  const XML_Char *cachedNewBase = 0;

  iter.init(oldTable);

  for (;;) {
    Entity *newE;
    const XML_Char *name;
    const Entity *oldE = (Entity *)iter.next();
    if (!oldE)
      break;
    name = newPool.copyString(oldE->name);
    if (!name)
      return 0;
    newE = (Entity *)newTable.lookup(name, sizeof(Entity));
    if (!newE)
      return 0;
    if (oldE->systemId) {
      const XML_Char *tem = newPool.copyString(oldE->systemId);
      if (!tem)
	return 0;
      newE->systemId = tem;
      if (oldE->base) {
	if (oldE->base == cachedOldBase)
	  newE->base = cachedNewBase;
	else {
	  cachedOldBase = oldE->base;
	  tem = newPool.copyString(cachedOldBase);
	  if (!tem)
	    return 0;
	  cachedNewBase = newE->base = tem;
	}
      }
    }
    else {
      const XML_Char *tem = newPool.copyStringN(oldE->textPtr, oldE->textLen);
      if (!tem)
	return 0;
      newE->textPtr = tem;
      newE->textLen = oldE->textLen;
    }
    if (oldE->notation) {
      const XML_Char *tem = newPool.copyString(oldE->notation);
      if (!tem)
	return 0;
      newE->notation = tem;
    }
  }
  return 1;
}


StringPool::StringPool()
{
  blocks_ = 0;
  freeBlocks_ = 0;
  start_ = 0;
  ptr_ = 0;
  end_ = 0;
}

StringPool::~StringPool()
{
  Block *p = blocks_;
  while (p) {
    Block *tem = p->next;
    free(p);
    p = tem;
  }
  blocks_ = 0;
  p = freeBlocks_;
  while (p) {
    Block *tem = p->next;
    free(p);
    p = tem;
  }
  freeBlocks_ = 0;
  ptr_ = 0;
  start_ = 0;
  end_ = 0;
}


void StringPool::clear()
{
  if (!freeBlocks_)
    freeBlocks_ = blocks_;
  else {
    Block *p = blocks_;
    while (p) {
      Block *tem = p->next;
      p->next = freeBlocks_;
      freeBlocks_ = p;
      p = tem;
    }
  }
  blocks_ = 0;
  start_ = 0;
  ptr_ = 0;
  end_ = 0;
}

XML_Char *StringPool::append(const ENCODING *enc,
			      const char *ptr, const char *end)
{
  if (!ptr_ && !grow())
    return 0;
  for (;;) {
    XmlConvert(enc, &ptr, end, (ICHAR **)&(ptr_), (ICHAR *)end_);
    if (ptr == end)
      break;
    if (!grow())
      return 0;
  }
  return start_;
}

const XML_Char *StringPool::copyString(const XML_Char *s)
{
  do {
    if (!appendChar(*s))
      return 0;
  } while (*s++);
  s = start_;
  finish();
  return s;
}

const XML_Char *StringPool::copyStringN(const XML_Char *s, int n)
{
  if (!ptr_ && !grow())
    return 0;
  for (; n > 0; --n, s++) {
    if (!appendChar(*s))
      return 0;

  }
  s = start_;
  finish();
  return s;
}

XML_Char *StringPool::storeString(const ENCODING *enc,
				   const char *ptr, const char *end)
{
  if (!append(enc, ptr, end))
    return 0;
  if (ptr_ == end_ && !grow())
    return 0;
  *(ptr_)++ = 0;
  return start_;
}

int StringPool::grow()
{
  if (freeBlocks_) {
    if (start_ == 0) {
      blocks_ = freeBlocks_;
      freeBlocks_ = freeBlocks_->next;
      blocks_->next = 0;
      start_ = blocks_->s();
      end_ = start_ + blocks_->size;
      ptr_ = start_;
      return 1;
    }
    if (end_ - start_ < freeBlocks_->size) {
      Block *tem = freeBlocks_->next;
      freeBlocks_->next = blocks_;
      blocks_ = freeBlocks_;
      freeBlocks_ = tem;
      memcpy(blocks_->s(), start_, (end_ - start_) * sizeof(XML_Char));
      ptr_ = blocks_->s() + (ptr_ - start_);
      start_ = blocks_->s();
      end_ = start_ + blocks_->size;
      return 1;
    }
  }
  if (blocks_ && start_ == blocks_->s()) {
    int blockSize = (end_ - start_)*2;
    Block *tem = (Block *)malloc(sizeof(Block)
				 + blockSize*sizeof(XML_Char));
    if (!tem)
      return 0;
    memcpy(tem->s(), blocks_->s(), (end_ - start_)*sizeof(XML_Char));
    blocks_ = tem;
    if (!blocks_)
      return 0;
    blocks_->size = blockSize;
    ptr_ = blocks_->s() + (ptr_ - start_);
    start_ = blocks_->s();
    end_ = start_ + blockSize;
  }
  else {
    Block *tem;
    int blockSize = end_ - start_;
    if (blockSize < INIT_BLOCK_SIZE)
      blockSize = INIT_BLOCK_SIZE;
    else
      blockSize *= 2;
    tem = (Block *)malloc(sizeof(Block)
			  + blockSize*sizeof(XML_Char));
    if (!tem)
      return 0;
    tem->size = blockSize;
    tem->next = blocks_;
    blocks_ = tem;
    memcpy(tem->s(), start_, (ptr_ - start_) * sizeof(XML_Char));
    ptr_ = tem->s() + (ptr_ - start_);
    start_ = tem->s();
    end_ = tem->s() + blockSize;
  }
  return 1;
}

const int INIT_SIZE = 64;

int HashTable::keyeq(Key s1, Key s2)
{
  for (; *s1 == *s2; s1++, s2++)
    if (*s1 == 0)
      return 1;
  return 0;
}

unsigned long HashTable::hash(Key s)
{
  unsigned long h = 0;
  while (*s)
    h = (h << 5) + h + (unsigned char)*s++;
  return h;
}

Named *HashTable::lookup(Key name, size_t createSize)
{
  size_t i;
  if (size_ == 0) {
    if (!createSize)
      return 0;
    v_ = (Named **)calloc(INIT_SIZE, sizeof(Named *));
    if (!v_)
      return 0;
    size_ = INIT_SIZE;
    usedLim_ = INIT_SIZE / 2;
    i = hash(name) & (size_ - 1);
  }
  else {
    unsigned long h = hash(name);
    for (i = h & (size_ - 1);
         v_[i];
         i == 0 ? i = size_ - 1 : --i) {
      if (keyeq(name, v_[i]->name))
	return v_[i];
    }
    if (!createSize)
      return 0;
    if (used_ == usedLim_) {
      /* check for overflow */
      size_t newSize = size_ * 2;
      Named **newV = (Named **)calloc(newSize, sizeof(Named *));
      if (!newV)
	return 0;
      for (i = 0; i < size_; i++)
	if (v_[i]) {
	  size_t j;
	  for (j = hash(v_[i]->name) & (newSize - 1);
	       newV[j];
	       j == 0 ? j = newSize - 1 : --j)
	    ;
	  newV[j] = v_[i];
	}
      free(v_);
      v_ = newV;
      size_ = newSize;
      usedLim_ = newSize/2;
      for (i = h & (size_ - 1);
	   v_[i];
	   i == 0 ? i = size_ - 1 : --i)
	;
    }
  }
  v_[i] = (Named *)calloc(1, createSize);
  if (!v_[i])
    return 0;
  v_[i]->name = name;
  used_++;
  return v_[i];
}

HashTable::~HashTable()
{
  size_t i;
  for (i = 0; i < size_; i++) {
    Named *p = v_[i];
    if (p)
      free(p);
  }
  free(v_);
}

HashTable::HashTable()
{
  size_ = 0;
  usedLim_ = 0;
  used_ = 0;
  v_ = 0;
}

HashTableIter::HashTableIter()
{
  p_ = 0;
  end_ = 0;
}

HashTableIter::HashTableIter(const HashTable &table)
{
  p_ = table.v_;
  end_ = p_ + table.size_;
}

void HashTableIter::init(const HashTable &table)
{
  p_ = table.v_;
  end_ = p_ + table.size_;
}

Named *HashTableIter::next()
{
  while (p_ != end_) {
    Named *tem = *p_++;
    if (tem)
      return tem;
  }
  return 0;
}
