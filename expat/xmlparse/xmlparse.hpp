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

#ifndef XmlParse_INCLUDED
#define XmlParse_INCLUDED 1

#ifndef XMLPARSEAPI
#define XMLPARSEAPI /* as nothing */
#endif

#ifdef XML_UNICODE_WCHAR_T

/* XML_UNICODE_WCHAR_T will work only if sizeof(wchar_t) == 2 and wchar_t
uses Unicode. */
/* Information is UTF-16 encoded as wchar_ts */

#ifndef XML_UNICODE
#define XML_UNICODE
#endif

#include <stddef.h>
typedef wchar_t XML_Char;
typedef wchar_t XML_LChar;

#else /* not XML_UNICODE_WCHAR_T */

#ifdef XML_UNICODE

/* Information is UTF-16 encoded as unsigned shorts */
typedef unsigned short XML_Char;
typedef char XML_LChar;

#else /* not XML_UNICODE */

/* Information is UTF-8 encoded. */
typedef char XML_Char;
typedef char XML_LChar;

#endif /* not XML_UNICODE */

#endif /* not XML_UNICODE_WCHAR_T */

class XML_ElementHandler {
 public:
  /* atts is array of name/value pairs, terminated by 0; names and
     values are 0 terminated. */
  virtual void startElement(const XML_Char *name, const XML_Char **atts) = 0;
  virtual void endElement(const XML_Char *name) = 0;
};

class XML_CharacterDataHandler {
 public:
  /* s is not 0 terminated. */
  virtual void characterData(const XML_Char *s, int len) = 0;
};

class XML_ProcessingInstructionHandler {
 public:
  /* target and data are 0 terminated */
  virtual void processingInstruction(const XML_Char *target, const XML_Char *data) = 0;
};

/* data is 0 terminated */
class XML_CommentHandler {
  public:
  virtual void comment(const XML_Char *data) = 0;
};

class XML_CdataSectionHandler {
 public:
  virtual void startCdataSection() = 0;
  virtual void endCdataSection() = 0;
};

class XML_DefaultHandler {
 public:
  /* This is called for any characters in the XML document for which
     there is no applicable handler.  This includes both characters
     that are part of markup which is of a kind that is not reported
     (comments, markup declarations), or characters that are part of a
     construct which could be reported but for which no handler has
     been supplied. The characters are passed exactly as they were in
     the XML document except that they will be encoded in UTF-8.  Line
     boundaries are not normalized.  Note that a byte order mark
     character is not passed to the default handler.  There are no
     guarantees about how characters are divided between calls to the
     default handler: for example, a comment might be split between
     multiple calls. */
  virtual void doDefault(const XML_Char *s, int len) = 0;
};

class XML_DoctypeDeclHandler {
 public:
  /* This is called for the start of the DOCTYPE declaration when the
     name of the DOCTYPE is encountered. */
  virtual void startDoctypeDecl(const XML_Char *doctypeName) = 0;
  /* This is called for the start of the DOCTYPE declaration when the
     closing > is encountered, but after processing any external subset. */
  virtual void endDoctypeDecl() = 0;
};

class XML_UnparsedEntityDeclHandler {
 public:
  /* This is called for a declaration of an unparsed (NDATA)
     entity.  The base argument is whatever was set by XML_SetBase.
     The entityName, systemId and notationName arguments will never be null.
     The other arguments may be. */
  virtual void unparsedEntityDecl(const XML_Char *entityName,
				  const XML_Char *base,
				  const XML_Char *systemId,
				  const XML_Char *publicId,
				  const XML_Char *notationName) = 0;
};

class XML_NotationDeclHandler {
 public:
  /* This is called for a declaration of notation.  The base argument
     is whatever was set by XML_SetBase.  The notationName will never
     be null.  The other arguments can be. */
  virtual void notationDecl(const XML_Char *notationName,
			    const XML_Char *base,
			    const XML_Char *systemId,
			    const XML_Char *publicId) = 0;
};

class XML_NamespaceDeclHandler {
 public:
  /* When namespace processing is enabled, these are called once for
     each namespace declaration. The call to the start and end element
     handlers occur between the calls to the start and end namespace
     declaration handlers. For an xmlns attribute, prefix will be
     null.  For an xmlns="" attribute, uri will be null. */
  
  virtual void startNamespaceDecl(const XML_Char *prefix,
				  const XML_Char *uri) = 0;

  virtual void endNamespaceDecl(const XML_Char *prefix) = 0;
};

class XML_NotStandaloneHandler {
 public:
  /* This is called if the document is not standalone (it has an
     external subset or a reference to a parameter entity, but does
     not have standalone="yes"). If this handler returns 0, then
     processing will not continue, and the parser will return a
     ERROR_NOT_STANDALONE error. */
  virtual int notStandalone() = 0;
};

class XML_Parser;

class XML_ExternalEntityRefHandler {
 public:
  /* This is called for a reference to an external parsed general
     entity.  The referenced entity is not automatically parsed.  The
     application can parse it immediately or later using
     externalEntityParserCreate.  The parser argument is the parser
     parsing the entity containing the reference; it can be passed as
     the parser argument to externalEntityParserCreate.  The systemId
     argument is the system identifier as specified in the entity
     declaration; it will not be null.  The base argument is the
     system identifier that should be used as the base for resolving
     systemId if systemId was relative; this is set by setBase; it may
     be null.  The publicId argument is the public identifier as
     specified in the entity declaration, or null if none was
     specified; the whitespace in the public identifier will have been
     normalized as required by the XML spec.  The context argument
     specifies the parsing context in the format expected by the
     context argument to externalEntityParserCreate; context is valid
     only until the handler returns, so if the referenced entity is to
     be parsed later, it must be copied.  The handler should return 0
     if processing should not continue because of a fatal error in the
     handling of the external entity.  In this case the calling parser
     will return an ERROR_EXTERNAL_ENTITY_HANDLING error.  Note that
     unlike other handlers the first argument is the parser, not
     userData. */

  virtual int externalEntityRef(XML_Parser *parser,
				const XML_Char *context,
				const XML_Char *base,
				const XML_Char *systemId,
				const XML_Char *publicId) = 0;
};
  
/* This object is returned by the XML_UnknownEncodingHandler
to provide information to the parser about encodings that are unknown
to the parser.
The map[b] member gives information about byte sequences
whose first byte is b.
If map[b] is c where c is >= 0, then b by itself encodes the Unicode scalar value c.
If map[b] is -1, then the byte sequence is malformed.
If map[b] is -n, where n >= 2, then b is the first byte of an n-byte
sequence that encodes a single Unicode scalar value.
The data member will be passed as the first argument to the convert function.
The convert function is used to convert multibyte sequences;
s will point to a n-byte sequence where map[(unsigned char)*s] == -n.
The convert function must return the Unicode scalar value
represented by this byte sequence or -1 if the byte sequence is malformed.
The convert function may be null if the encoding is a single-byte encoding,
that is if map[b] >= -1 for all bytes b.
When the parser is finished with the encoding, then if release is not null,
it will call release passing it the data member;
once release has been called, the convert function will not be called again.

Expat places certain restrictions on the encodings that are supported
using this mechanism.

1. Every ASCII character that can appear in a well-formed XML document,
other than the characters

  $@\^`{}~

must be represented by a single byte, and that byte must be the
same byte that represents that character in ASCII.

2. No character may require more than 4 bytes to encode.

3. All characters encoded must have Unicode scalar values <= 0xFFFF,
(ie characters that would be encoded by surrogates in UTF-16
are  not allowed).  Note that this restriction doesn't apply to
the built-in support for UTF-8 and UTF-16.

4. No Unicode character may be encoded by more than one distinct sequence
of bytes. */

class XMLPARSEAPI XML_Encoding {
public:
  int map[256];
  virtual int convert(const char *s);
  virtual void release();
};

class XML_UnknownEncodingHandler {
public:
  /* This is called for an encoding that is unknown to the parser.
     The encodingHandlerData argument is that which was passed as the
     second argument to XML_SetUnknownEncodingHandler.  The name
     argument gives the name of the encoding as specified in the
     encoding declaration.  If the callback can provide information
     about the encoding, it must fill in the XML_Encoding structure,
     and return 1.  Otherwise it must return 0.  If info does not
     describe a suitable encoding, then the parser will return an
     XML_UNKNOWN_ENCODING error. */
  virtual XML_Encoding *unknownEncoding(const XML_Char *name) = 0;
};

class XMLPARSEAPI XML_Parser {
 public:
  /* Constructs a new parser; encoding is the encoding specified by
     the external protocol or null if there is none specified. */
  static XML_Parser *parserCreate(const XML_Char *encoding = 0);

  /* Constructs a new parser and namespace processor.  Element type
     names and attribute names that belong to a namespace will be
     expanded; unprefixed attribute names are never expanded;
     unprefixed element type names are expanded only if there is a
     default namespace. The expanded name is the concatenation of the
     namespace URI, the namespace separator character, and the local
     part of the name.  If the namespace separator is '\0' then the
     namespace URI and the local part will be concatenated without any
     separator.  When a namespace is not declared, the name and prefix
     will be passed through without expansion. */

  static XML_Parser *parserCreateNS(const XML_Char *encoding,
				    XML_Char namespaceSeparator);

  virtual void release() = 0;
  virtual void setElementHandler(XML_ElementHandler *handler) = 0;
  virtual void setCharacterDataHandler(XML_CharacterDataHandler *handler) = 0;
  virtual void setProcessingInstructionHandler(XML_ProcessingInstructionHandler *handler) = 0;
  virtual void setCommentHandler(XML_CommentHandler *handler) = 0;
  virtual void setCdataSectionHandler(XML_CdataSectionHandler *handler) = 0;

  /* This sets the default handler and also inhibits expansion of
     internal entities.  The entity reference will be passed to the
     default handler. */

  virtual void setDefaultHandler(XML_DefaultHandler *handler) = 0;

  /* This sets the default handler but does not inhibit expansion of
     internal entities.  The entity reference will not be passed to
     the default handler. */

  virtual void setDefaultHandlerExpand(XML_DefaultHandler *handler) = 0;

  virtual void setDoctypeDeclHandler(XML_DoctypeDeclHandler *handler) = 0;
  virtual void setUnparsedEntityDeclHandler(XML_UnparsedEntityDeclHandler *handler) = 0;
  virtual void setNotationDeclHandler(XML_NotationDeclHandler *handler) = 0;
  virtual void setNamespaceDeclHandler(XML_NamespaceDeclHandler *handler) = 0;
  virtual void setNotStandaloneHandler(XML_NotStandaloneHandler *handler) = 0;
  virtual void setExternalEntityRefHandler(XML_ExternalEntityRefHandler *handler) = 0;
  virtual void setUnknownEncodingHandler(XML_UnknownEncodingHandler *handler) = 0;

  /* This can be called within a handler for a start element, end
     element, processing instruction or character data.  It causes the
     corresponding markup to be passed to the default handler. */
  virtual void defaultCurrent() = 0;

  /* This is equivalent to supplying an encoding argument
     to XML_ParserCreate. It must not be called after XML_Parse
     or XML_ParseBuffer. */
  virtual int setEncoding(const XML_Char *encoding) = 0;

  /* Sets the base to be used for resolving relative URIs in system
     identifiers in declarations.  Resolving relative identifiers is
     left to the application: this value will be passed through as the
     base argument to the XML_ExternalEntityRefHandler,
     XML_NotationDeclHandler and XML_UnparsedEntityDeclHandler. The
     base argument will be copied.  Returns zero if out of memory,
     non-zero otherwise. */

  virtual int setBase(const XML_Char *base) = 0;

  virtual const XML_Char *getBase() = 0;

  /* Returns the number of the attributes passed in last call to the
     XML_StartElementHandler that were specified in the start-tag rather
     than defaulted. */
  virtual int getSpecifiedAttributeCount() = 0;

  /* Parses some input. Returns 0 if a fatal error is detected.
     The last call to XML_Parse must have isFinal true;
     len may be zero for this call (or any other). */
  virtual int parse(const char *s, int len, int isFinal) = 0;

  virtual char *getBuffer(int len) = 0;

  virtual int parseBuffer(int len, int isFinal) = 0;

  /* Creates an XML_Parser object that can parse an external general
     entity; context is a '\0'-terminated string specifying the parse
     context; encoding is a '\0'-terminated string giving the name of
     the externally specified encoding, or null if there is no
     externally specified encoding.  The context string consists of a
     sequence of tokens separated by formfeeds (\f); a token
     consisting of a name specifies that the general entity of the
     name is open; a token of the form prefix=uri specifies the
     namespace for a particular prefix; a token of the form =uri
     specifies the default namespace.  This can be called at any point
     after the first call to an ExternalEntityRefHandler so longer as
     the parser has not yet been freed.  The new parser is completely
     independent and may safely be used in a separate thread.  The
     handlers and userData are initialized from the parser argument.
     Returns 0 if out of memory.  Otherwise returns a new XML_Parser
     object. */

  virtual XML_Parser *externalEntityParserCreate(const XML_Char *context,
						 const XML_Char *encoding) = 0;

  enum ParamEntityParsing {
    PARAM_ENTITY_PARSING_NEVER,
    PARAM_ENTITY_PARSING_UNLESS_STANDALONE,
    PARAM_ENTITY_PARSING_ALWAYS
  };

  /* Controls parsing of parameter entities (including the external
     DTD subset). If parsing of parameter entities is enabled, then
     references to external parameter entities (including the external
     DTD subset) will be passed to the handler set with
     XML_SetExternalEntityRefHandler.  The context passed will be 0.
     Unlike external general entities, external parameter entities can
     only be parsed synchronously.  If the external parameter entity
     is to be parsed, it must be parsed during the call to the
     external entity ref handler: the complete sequence of
     XML_ExternalEntityParserCreate, XML_Parse/XML_ParseBuffer and
     XML_ParserFree calls must be made during this call.  After
     XML_ExternalEntityParserCreate has been called to create the
     parser for the external parameter entity (context must be 0 for
     this call), it is illegal to make any calls on the old parser
     until XML_ParserFree has been called on the newly created parser.
     If the library has been compiled without support for parameter
     entity parsing (ie without XML_DTD being defined), then
     XML_SetParamEntityParsing will return 0 if parsing of parameter
     entities is requested; otherwise it will return non-zero. */

  virtual int setParamEntityParsing(ParamEntityParsing parsing) = 0;

  enum Error {
    ERROR_NONE,
    ERROR_NO_MEMORY,
    ERROR_SYNTAX,
    ERROR_NO_ELEMENTS,
    ERROR_INVALID_TOKEN,
    ERROR_UNCLOSED_TOKEN,
    ERROR_PARTIAL_CHAR,
    ERROR_TAG_MISMATCH,
    ERROR_DUPLICATE_ATTRIBUTE,
    ERROR_JUNK_AFTER_DOC_ELEMENT,
    ERROR_PARAM_ENTITY_REF,
    ERROR_UNDEFINED_ENTITY,
    ERROR_RECURSIVE_ENTITY_REF,
    ERROR_ASYNC_ENTITY,
    ERROR_BAD_CHAR_REF,
    ERROR_BINARY_ENTITY_REF,
    ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF,
    ERROR_MISPLACED_XML_PI,
    ERROR_UNKNOWN_ENCODING,
    ERROR_INCORRECT_ENCODING,
    ERROR_UNCLOSED_CDATA_SECTION,
    ERROR_EXTERNAL_ENTITY_HANDLING,
    ERROR_NOT_STANDALONE
  };

  /* If parse or XML_parseBuffer have returned 0, then XML_GetErrorCode
     returns information about the error. */
  
  virtual Error getErrorCode() = 0;

  /* These functions return information about the current parse
     location.  They may be called when XML_Parse or XML_ParseBuffer
     return 0; in this case the location is the location of the
     character at which the error was detected.  They may also be
     called from any other callback called to report some parse event;
     in this the location is the location of the first of the sequence
     of characters that generated the event. */

  virtual int getCurrentLineNumber() = 0;
  virtual int getCurrentColumnNumber() = 0;
  virtual long getCurrentByteIndex() = 0;

  /* Return the number of bytes in the current event.
     Returns 0 if the event is in an internal entity. */
  virtual int getCurrentByteCount() = 0;

  /* Returns a string describing the error. */
  static const XML_LChar *errorString(int code);
protected:
  ~XML_Parser() { }
};

#endif /* not XmlParse_INCLUDED */
