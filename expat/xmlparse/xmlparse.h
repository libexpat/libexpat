/*
The contents of this file are subject to the Mozilla Public License
Version 1.0 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations
under the License.

The Original Code is expat.

The Initial Developer of the Original Code is James Clark.
Portions created by James Clark are Copyright (C) 1998
James Clark. All Rights Reserved.

Contributor(s):
*/

#ifndef XmlParse_INCLUDED
#define XmlParse_INCLUDED 1

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XMLPARSEAPI
#define XMLPARSEAPI /* as nothing */
#endif

typedef void *XML_Parser;

#ifdef XML_UNICODE
/* Information is UTF-16 encoded. */
#include <stddef.h>
typedef wchar_t XML_Char;
#else
/* Information is UTF-8 encoded. */
typedef char XML_Char;
#endif

/* Constructs a new parser; encoding is the externally specified encoding,
or null if there is no externally specified encoding. */

XML_Parser XMLPARSEAPI
XML_ParserCreate(const XML_Char *encoding);


/* atts is array of name/value pairs, terminated by NULL;
   names and values are '\0' terminated. */

typedef void (*XML_StartElementHandler)(void *userData,
					const XML_Char *name,
					const XML_Char **atts);

typedef void (*XML_EndElementHandler)(void *userData,
				      const XML_Char *name);

typedef void (*XML_CharacterDataHandler)(void *userData,
					 const XML_Char *s,
					 int len);

/* target and data are '\0' terminated */
typedef void (*XML_ProcessingInstructionHandler)(void *userData,
						 const XML_Char *target,
						 const XML_Char *data);

/* Reports a reference to an external parsed general entity.
The referenced entity is not automatically parsed.
The application can parse it immediately or later using
XML_ExternalEntityParserCreate.
The parser argument is the parser parsing the entity containing the reference;
it can be passed as the parser argument to XML_ExternalEntityParserCreate.
The systemId argument is the system identifier as specified in the entity declaration;
it will not be null.
The base argument is the system identifier that should be used as the base for
resolving systemId if systemId was relative; this is set by XML_SetBase;
it may be null.
The publicId argument is the public identifier as specified in the entity declaration,
or null if none was specified; the whitespace in the public identifier
will have been normalized as required by the XML spec.
The openEntityNames argument is a space-separated list of the names of the entities
that are open for the parse of this entity (including the name of the referenced
entity); this can be passed as the openEntityNames argument to
XML_ExternalEntityParserCreate; openEntityNames is valid only until the handler
returns, so if the referenced entity is to be parsed later, it must be copied.
The handler should return 0 if processing should not continue because of
a fatal error in the handling of the external entity.
In this case the calling parser will return an XML_ERROR_EXTERNAL_ENTITY_HANDLING
error.
Note that unlike other handlers the first argument is the parser, not userData. */

typedef int (*XML_ExternalEntityRefHandler)(XML_Parser parser,
					    const XML_Char *openEntityNames,
					    const XML_Char *base,
					    const XML_Char *systemId,
					    const XML_Char *publicId);

void XMLPARSEAPI
XML_SetElementHandler(XML_Parser parser,
		      XML_StartElementHandler start,
		      XML_EndElementHandler end);

void XMLPARSEAPI
XML_SetCharacterDataHandler(XML_Parser parser,
			    XML_CharacterDataHandler handler);

void XMLPARSEAPI
XML_SetProcessingInstructionHandler(XML_Parser parser,
				    XML_ProcessingInstructionHandler handler);

void XMLPARSEAPI
XML_SetExternalEntityRefHandler(XML_Parser parser,
				XML_ExternalEntityRefHandler handler);


/* This value is passed as the userData argument to callbacks. */
void XMLPARSEAPI
XML_SetUserData(XML_Parser parser, void *userData);

/* Returns the last value set by XML_SetUserData or null. */
void XMLPARSEAPI *
XML_GetUserData(XML_Parser parser);

/* Sets the base to be used for resolving relative URIs in system identifiers in
declarations.  Resolving relative identifiers is left to the application:
this value will be passed through as the base argumentto the ExternalEntityRefHandler.
The base argument will be copied. Returns zero if out of memory, non-zero otherwise. */

int XMLPARSEAPI
XML_SetBase(XML_Parser parser, const XML_Char *base);

/* Parses some input. Returns 0 if a fatal error is detected.
The last call to XML_Parse must have isFinal true;
len may be zero for this call (or any other). */
int XMLPARSEAPI
XML_Parse(XML_Parser parser, const char *s, int len, int isFinal);

void XMLPARSEAPI *
XML_GetBuffer(XML_Parser parser, int len);

int XMLPARSEAPI
XML_ParseBuffer(XML_Parser parser, int len, int isFinal);

/* Creates an XML_Parser object that can parse an external general entity;
openEntityNames is a space-separated list of the names of the entities that are open
for the parse of this entity (including the name of this one);
encoding is the externally specified encoding,
or null if there is no externally specified encoding.
This can be called at any point after the first call to an ExternalEntityRefHandler
so longer as the parser has not yet been freed.
The new parser is completely independent and may safely be used in a separate thread.
The handlers and userData are initialized from the parser argument.
Returns 0 if out of memory.  Otherwise returns a new XML_Parser object. */
XML_Parser XMLPARSEAPI
XML_ExternalEntityParserCreate(XML_Parser parser,
			       const XML_Char *openEntityNames,
			       const XML_Char *encoding);

/* If XML_Parser or XML_ParseEnd have returned 0, then XML_GetError*
returns information about the error. */

enum XML_Error {
  XML_ERROR_NONE,
  XML_ERROR_NO_MEMORY,
  XML_ERROR_SYNTAX,
  XML_ERROR_NO_ELEMENTS,
  XML_ERROR_INVALID_TOKEN,
  XML_ERROR_UNCLOSED_TOKEN,
  XML_ERROR_PARTIAL_CHAR,
  XML_ERROR_TAG_MISMATCH,
  XML_ERROR_DUPLICATE_ATTRIBUTE,
  XML_ERROR_JUNK_AFTER_DOC_ELEMENT,
  XML_ERROR_PARAM_ENTITY_REF,
  XML_ERROR_UNDEFINED_ENTITY,
  XML_ERROR_RECURSIVE_ENTITY_REF,
  XML_ERROR_ASYNC_ENTITY,
  XML_ERROR_BAD_CHAR_REF,
  XML_ERROR_BINARY_ENTITY_REF,
  XML_ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF,
  XML_ERROR_MISPLACED_XML_PI,
  XML_ERROR_UNKNOWN_ENCODING,
  XML_ERROR_INCORRECT_ENCODING,
  XML_ERROR_UNCLOSED_CDATA_SECTION,
  XML_ERROR_EXTERNAL_ENTITY_HANDLING
};

int XMLPARSEAPI XML_GetErrorCode(XML_Parser parser);
int XMLPARSEAPI XML_GetErrorLineNumber(XML_Parser parser);
int XMLPARSEAPI XML_GetErrorColumnNumber(XML_Parser parser);
long XMLPARSEAPI XML_GetErrorByteIndex(XML_Parser parser);

void XMLPARSEAPI
XML_ParserFree(XML_Parser parser);

const XML_Char XMLPARSEAPI *
XML_ErrorString(int code);

#ifdef __cplusplus
}
#endif

#endif /* not XmlParse_INCLUDED */
