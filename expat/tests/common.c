/* Commonly used functions for the Expat test suite
                            __  __            _
                         ___\ \/ /_ __   __ _| |_
                        / _ \\  /| '_ \ / _` | __|
                       |  __//  \| |_) | (_| | |_
                        \___/_/\_\ .__/ \__,_|\__|
                                 |_| XML parser

   Copyright (c) 2001-2006 Fred L. Drake, Jr. <fdrake@users.sourceforge.net>
   Copyright (c) 2003      Greg Stein <gstein@users.sourceforge.net>
   Copyright (c) 2005-2007 Steven Solie <steven@solie.ca>
   Copyright (c) 2005-2012 Karl Waclawek <karl@waclawek.net>
   Copyright (c) 2016-2022 Sebastian Pipping <sebastian@pipping.org>
   Copyright (c) 2017-2018 Rhodri James <rhodri@wildebeest.org.uk>
   Copyright (c) 2017      Joe Orton <jorton@redhat.com>
   Copyright (c) 2017      José Gutiérrez de la Concha <jose@zeroc.com>
   Copyright (c) 2018      Marco Maggi <marco.maggi-ipsu@poste.it>
   Copyright (c) 2019      David Loffredo <loffredo@steptools.com>
   Copyright (c) 2020      Tim Gates <tim.gates@iress.com>
   Copyright (c) 2021      Dong-hee Na <donghee.na@python.org>
   Licensed under the MIT license:

   Permission is  hereby granted,  free of charge,  to any  person obtaining
   a  copy  of  this  software   and  associated  documentation  files  (the
   "Software"),  to  deal in  the  Software  without restriction,  including
   without  limitation the  rights  to use,  copy,  modify, merge,  publish,
   distribute, sublicense, and/or sell copies of the Software, and to permit
   persons  to whom  the Software  is  furnished to  do so,  subject to  the
   following conditions:

   The above copyright  notice and this permission notice  shall be included
   in all copies or substantial portions of the Software.

   THE  SOFTWARE  IS  PROVIDED  "AS  IS",  WITHOUT  WARRANTY  OF  ANY  KIND,
   EXPRESS  OR IMPLIED,  INCLUDING  BUT  NOT LIMITED  TO  THE WARRANTIES  OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
   NO EVENT SHALL THE AUTHORS OR  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR  OTHER LIABILITY, WHETHER  IN AN  ACTION OF CONTRACT,  TORT OR
   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include <stdio.h>
#include <string.h>

#include "expat.h"
#include "internal.h" /* for UNUSED_P */
#include "minicheck.h"
#include "chardata.h"
#include "common.h"


void
tcase_add_test__ifdef_xml_dtd(TCase *tc, tcase_test_function test) {
#ifdef XML_DTD
  tcase_add_test(tc, test);
#else
  UNUSED_P(tc);
  UNUSED_P(test);
#endif
}

void
basic_setup(void) {
  g_parser = XML_ParserCreate(NULL);
  if (g_parser == NULL)
    fail("Parser not created.");
}

void
basic_teardown(void) {
  if (g_parser != NULL) {
    XML_ParserFree(g_parser);
    g_parser = NULL;
  }
}

/* Generate a failure using the parser state to create an error message;
   this should be used when the parser reports an error we weren't
   expecting.
*/
void
_xml_failure(XML_Parser parser, const char *file, int line) {
  char buffer[1024];
  enum XML_Error err = XML_GetErrorCode(parser);
  sprintf(buffer,
          "    %d: %" XML_FMT_STR " (line %" XML_FMT_INT_MOD
          "u, offset %" XML_FMT_INT_MOD "u)\n    reported from %s, line %d\n",
          err, XML_ErrorString(err), XML_GetCurrentLineNumber(parser),
          XML_GetCurrentColumnNumber(parser), file, line);
  _fail_unless(0, file, line, buffer);
}

enum XML_Status
_XML_Parse_SINGLE_BYTES(XML_Parser parser, const char *s, int len,
                        int isFinal) {
  enum XML_Status res = XML_STATUS_ERROR;
  int offset = 0;

  if (len == 0) {
    return XML_Parse(parser, s, len, isFinal);
  }

  for (; offset < len; offset++) {
    const int innerIsFinal = (offset == len - 1) && isFinal;
    const char c = s[offset]; /* to help out-of-bounds detection */
    res = XML_Parse(parser, &c, sizeof(char), innerIsFinal);
    if (res != XML_STATUS_OK) {
      return res;
    }
  }
  return res;
}

void
_expect_failure(const char *text, enum XML_Error errorCode,
                const char *errorMessage, const char *file, int lineno) {
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_OK)
    /* Hackish use of _fail_unless() macro, but let's us report
       the right filename and line number. */
    _fail_unless(0, file, lineno, errorMessage);
  if (XML_GetErrorCode(g_parser) != errorCode)
    _xml_failure(g_parser, file, lineno);
}


int XMLCALL
external_entity_optioner(XML_Parser parser, const XML_Char *context,
                         const XML_Char *base, const XML_Char *systemId,
                         const XML_Char *publicId) {
  ExtOption *options = (ExtOption *)XML_GetUserData(parser);
  XML_Parser ext_parser;

  UNUSED_P(base);
  UNUSED_P(publicId);
  while (options->parse_text != NULL) {
    if (! xcstrcmp(systemId, options->system_id)) {
      enum XML_Status rc;
      ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
      if (ext_parser == NULL)
        return XML_STATUS_ERROR;
      rc = _XML_Parse_SINGLE_BYTES(ext_parser, options->parse_text,
                                   (int)strlen(options->parse_text), XML_TRUE);
      XML_ParserFree(ext_parser);
      return rc;
    }
    options++;
  }
  fail("No suitable option found");
  return XML_STATUS_ERROR;
}


/*
 * Parameter entity evaluation support.
 */

static const XML_Char *entity_name_to_match = NULL;
static const XML_Char *entity_value_to_match = NULL;
static int entity_match_flag = ENTITY_MATCH_NOT_FOUND;

void XMLCALL
param_entity_match_handler(void *userData, const XML_Char *entityName,
                           int is_parameter_entity, const XML_Char *value,
                           int value_length, const XML_Char *base,
                           const XML_Char *systemId, const XML_Char *publicId,
                           const XML_Char *notationName) {
  UNUSED_P(userData);
  UNUSED_P(base);
  UNUSED_P(systemId);
  UNUSED_P(publicId);
  UNUSED_P(notationName);
  if (! is_parameter_entity || entity_name_to_match == NULL
      || entity_value_to_match == NULL) {
    return;
  }
  if (! xcstrcmp(entityName, entity_name_to_match)) {
    /* The cast here is safe because we control the horizontal and
     * the vertical, and we therefore know our strings are never
     * going to overflow an int.
     */
    if (value_length != (int)xcstrlen(entity_value_to_match)
        || xcstrncmp(value, entity_value_to_match, value_length)) {
      entity_match_flag = ENTITY_MATCH_FAIL;
    } else {
      entity_match_flag = ENTITY_MATCH_SUCCESS;
    }
  }
  /* Else leave the match flag alone */
}

void
param_entity_match_init(const XML_Char *name, const XML_Char *value)
{
    entity_name_to_match = name;
    entity_value_to_match = value;
    entity_match_flag = ENTITY_MATCH_NOT_FOUND;
}

int
get_param_entity_match_flag(void)
{
    return entity_match_flag;
}

/*
 * Character data support for handlers, built on top of the code in
 * chardata.c
 */
void XMLCALL
accumulate_characters(void *userData, const XML_Char *s, int len) {
  CharData_AppendXMLChars((CharData *)userData, s, len);
}

void XMLCALL
accumulate_attribute(void *userData, const XML_Char *name,
                     const XML_Char **atts) {
  CharData *storage = (CharData *)userData;
  UNUSED_P(name);
  /* Check there are attributes to deal with */
  if (atts == NULL)
    return;

  while (storage->count < 0 && atts[0] != NULL) {
    /* "accumulate" the value of the first attribute we see */
    CharData_AppendXMLChars(storage, atts[1], -1);
    atts += 2;
  }
}

void
_run_character_check(const char *text, const XML_Char *expected,
                     const char *file, int line) {
  CharData storage;

  CharData_Init(&storage);
  XML_SetUserData(g_parser, &storage);
  XML_SetCharacterDataHandler(g_parser, accumulate_characters);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    _xml_failure(g_parser, file, line);
  CharData_CheckXMLChars(&storage, expected);
}

void
_run_attribute_check(const char *text, const XML_Char *expected,
                     const char *file, int line) {
  CharData storage;

  CharData_Init(&storage);
  XML_SetUserData(g_parser, &storage);
  XML_SetStartElementHandler(g_parser, accumulate_attribute);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    _xml_failure(g_parser, file, line);
  CharData_CheckXMLChars(&storage, expected);
}

void XMLCALL
ext_accumulate_characters(void *userData, const XML_Char *s, int len) {
  ExtTest *test_data = (ExtTest *)userData;
  accumulate_characters(test_data->storage, s, len);
}

void
_run_ext_character_check(const char *text, ExtTest *test_data,
                         const XML_Char *expected, const char *file, int line) {
  CharData *const storage = (CharData *)malloc(sizeof(CharData));

  CharData_Init(storage);
  test_data->storage = storage;
  XML_SetUserData(g_parser, test_data);
  XML_SetCharacterDataHandler(g_parser, ext_accumulate_characters);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    _xml_failure(g_parser, file, line);
  CharData_CheckXMLChars(storage, expected);

  free(storage);
}