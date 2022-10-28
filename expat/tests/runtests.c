/* Run the Expat test suite
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
   Copyright (c) 2017-2022 Rhodri James <rhodri@wildebeest.org.uk>
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

#include <expat_config.h>

#if defined(NDEBUG)
#  undef NDEBUG /* because test suite relies on assert(...) at the moment */
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h> /* ptrdiff_t */
#include <ctype.h>
#include <limits.h>
#include <stdint.h> /* intptr_t uint64_t */

#if ! defined(__cplusplus)
#  include <stdbool.h>
#endif

#include "expat.h"
#include "chardata.h"
#include "structdata.h"
#include "internal.h"
#include "minicheck.h"
#include "memcheck.h"
#include "common.h"
#include "dummy.h"
#include "handlers.h"
#include "ascii.h" /* for ASCII_xxx */

#include "basic_tests.h"

XML_Parser g_parser = NULL;

/* Test the recursive parse interacts with a not standalone handler */
static int XMLCALL
external_entity_not_standalone(XML_Parser parser, const XML_Char *context,
                               const XML_Char *base, const XML_Char *systemId,
                               const XML_Char *publicId) {
  const char *text1 = "<!ELEMENT doc EMPTY>\n"
                      "<!ENTITY % e1 SYSTEM 'bar'>\n"
                      "%e1;\n";
  const char *text2 = "<!ATTLIST doc a1 CDATA 'value'>";
  XML_Parser ext_parser;

  UNUSED_P(base);
  UNUSED_P(publicId);
  if (systemId == NULL)
    return XML_STATUS_OK;
  ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
  if (ext_parser == NULL)
    fail("Could not create external entity parser");
  if (! xcstrcmp(systemId, XCS("foo"))) {
    XML_SetNotStandaloneHandler(ext_parser, reject_not_standalone_handler);
    if (_XML_Parse_SINGLE_BYTES(ext_parser, text1, (int)strlen(text1), XML_TRUE)
        != XML_STATUS_ERROR)
      fail("Expected not standalone rejection");
    if (XML_GetErrorCode(ext_parser) != XML_ERROR_NOT_STANDALONE)
      xml_failure(ext_parser);
    XML_SetNotStandaloneHandler(ext_parser, NULL);
    XML_ParserFree(ext_parser);
    return XML_STATUS_ERROR;
  } else if (! xcstrcmp(systemId, XCS("bar"))) {
    if (_XML_Parse_SINGLE_BYTES(ext_parser, text2, (int)strlen(text2), XML_TRUE)
        == XML_STATUS_ERROR)
      xml_failure(ext_parser);
  }

  XML_ParserFree(ext_parser);
  return XML_STATUS_OK;
}

START_TEST(test_ext_entity_not_standalone) {
  const char *text = "<!DOCTYPE doc SYSTEM 'foo'>\n"
                     "<doc></doc>";

  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_not_standalone);
  expect_failure(text, XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                 "Standalone rejection not caught");
}
END_TEST

static int XMLCALL
external_entity_value_aborter(XML_Parser parser, const XML_Char *context,
                              const XML_Char *base, const XML_Char *systemId,
                              const XML_Char *publicId) {
  const char *text1 = "<!ELEMENT doc EMPTY>\n"
                      "<!ENTITY % e1 SYSTEM '004-2.ent'>\n"
                      "<!ENTITY % e2 '%e1;'>\n"
                      "%e1;\n";
  const char *text2 = "<?xml version='1.0' encoding='utf-8'?>";
  XML_Parser ext_parser;

  UNUSED_P(base);
  UNUSED_P(publicId);
  if (systemId == NULL)
    return XML_STATUS_OK;
  ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
  if (ext_parser == NULL)
    fail("Could not create external entity parser");
  if (! xcstrcmp(systemId, XCS("004-1.ent"))) {
    if (_XML_Parse_SINGLE_BYTES(ext_parser, text1, (int)strlen(text1), XML_TRUE)
        == XML_STATUS_ERROR)
      xml_failure(ext_parser);
  }
  if (! xcstrcmp(systemId, XCS("004-2.ent"))) {
    XML_SetXmlDeclHandler(ext_parser, entity_suspending_xdecl_handler);
    XML_SetUserData(ext_parser, ext_parser);
    if (_XML_Parse_SINGLE_BYTES(ext_parser, text2, (int)strlen(text2), XML_TRUE)
        != XML_STATUS_ERROR)
      fail("Aborted parse not faulted");
    if (XML_GetErrorCode(ext_parser) != XML_ERROR_ABORTED)
      xml_failure(ext_parser);
  }

  XML_ParserFree(ext_parser);
  return XML_STATUS_OK;
}

START_TEST(test_ext_entity_value_abort) {
  const char *text = "<!DOCTYPE doc SYSTEM '004-1.ent'>\n"
                     "<doc></doc>\n";

  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_value_aborter);
  g_resumable = XML_FALSE;
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

START_TEST(test_bad_public_doctype) {
  const char *text = "<?xml version='1.0' encoding='utf-8'?>\n"
                     "<!DOCTYPE doc PUBLIC '{BadName}' 'test'>\n"
                     "<doc></doc>";

  /* Setting a handler provokes a particular code path */
  XML_SetDoctypeDeclHandler(g_parser, dummy_start_doctype_handler,
                            dummy_end_doctype_handler);
  expect_failure(text, XML_ERROR_PUBLICID, "Bad Public ID not failed");
}
END_TEST

/* Test based on ibm/valid/P32/ibm32v04.xml */
START_TEST(test_attribute_enum_value) {
  const char *text = "<?xml version='1.0' standalone='no'?>\n"
                     "<!DOCTYPE animal SYSTEM 'test.dtd'>\n"
                     "<animal>This is a \n    <a/>  \n\nyellow tiger</animal>";
  ExtTest dtd_data
      = {"<!ELEMENT animal (#PCDATA|a)*>\n"
         "<!ELEMENT a EMPTY>\n"
         "<!ATTLIST animal xml:space (default|preserve) 'preserve'>",
         NULL, NULL};
  const XML_Char *expected = XCS("This is a \n      \n\nyellow tiger");

  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader);
  XML_SetUserData(g_parser, &dtd_data);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  /* An attribute list handler provokes a different code path */
  XML_SetAttlistDeclHandler(g_parser, dummy_attlist_decl_handler);
  run_ext_character_check(text, &dtd_data, expected);
}
END_TEST

/* Slightly bizarrely, the library seems to silently ignore entity
 * definitions for predefined entities, even when they are wrong.  The
 * language of the XML 1.0 spec is somewhat unhelpful as to what ought
 * to happen, so this is currently treated as acceptable.
 */
START_TEST(test_predefined_entity_redefinition) {
  const char *text = "<!DOCTYPE doc [\n"
                     "<!ENTITY apos 'foo'>\n"
                     "]>\n"
                     "<doc>&apos;</doc>";
  run_character_check(text, XCS("'"));
}
END_TEST

/* Test that the parser stops processing the DTD after an unresolved
 * parameter entity is encountered.
 */
START_TEST(test_dtd_stop_processing) {
  const char *text = "<!DOCTYPE doc [\n"
                     "%foo;\n"
                     "<!ENTITY bar 'bas'>\n"
                     "]><doc/>";

  XML_SetEntityDeclHandler(g_parser, dummy_entity_decl_handler);
  init_dummy_handlers();
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  if (get_dummy_handler_flags() != 0)
    fail("DTD processing still going after undefined PE");
}
END_TEST

/* Test public notations with no system ID */
START_TEST(test_public_notation_no_sysid) {
  const char *text = "<!DOCTYPE doc [\n"
                     "<!NOTATION note PUBLIC 'foo'>\n"
                     "<!ELEMENT doc EMPTY>\n"
                     "]>\n<doc/>";

  init_dummy_handlers();
  XML_SetNotationDeclHandler(g_parser, dummy_notation_decl_handler);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  if (get_dummy_handler_flags() != DUMMY_NOTATION_DECL_HANDLER_FLAG)
    fail("Notation declaration handler not called");
}
END_TEST

static void XMLCALL
record_element_start_handler(void *userData, const XML_Char *name,
                             const XML_Char **atts) {
  UNUSED_P(atts);
  CharData_AppendXMLChars((CharData *)userData, name, (int)xcstrlen(name));
}

START_TEST(test_nested_groups) {
  const char *text
      = "<!DOCTYPE doc [\n"
        "<!ELEMENT doc "
        /* Sixteen elements per line */
        "(e,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,"
        "(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?"
        "))))))))))))))))))))))))))))))))>\n"
        "<!ELEMENT e EMPTY>"
        "]>\n"
        "<doc><e/></doc>";
  CharData storage;

  CharData_Init(&storage);
  XML_SetElementDeclHandler(g_parser, dummy_element_decl_handler);
  XML_SetStartElementHandler(g_parser, record_element_start_handler);
  XML_SetUserData(g_parser, &storage);
  init_dummy_handlers();
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, XCS("doce"));
  if (get_dummy_handler_flags() != DUMMY_ELEMENT_DECL_HANDLER_FLAG)
    fail("Element handler not fired");
}
END_TEST

START_TEST(test_group_choice) {
  const char *text = "<!DOCTYPE doc [\n"
                     "<!ELEMENT doc (a|b|c)+>\n"
                     "<!ELEMENT a EMPTY>\n"
                     "<!ELEMENT b (#PCDATA)>\n"
                     "<!ELEMENT c ANY>\n"
                     "]>\n"
                     "<doc>\n"
                     "<a/>\n"
                     "<b attr='foo'>This is a foo</b>\n"
                     "<c></c>\n"
                     "</doc>\n";

  XML_SetElementDeclHandler(g_parser, dummy_element_decl_handler);
  init_dummy_handlers();
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  if (get_dummy_handler_flags() != DUMMY_ELEMENT_DECL_HANDLER_FLAG)
    fail("Element handler flag not raised");
}
END_TEST

static int XMLCALL
external_entity_public(XML_Parser parser, const XML_Char *context,
                       const XML_Char *base, const XML_Char *systemId,
                       const XML_Char *publicId) {
  const char *text1 = (const char *)XML_GetUserData(parser);
  const char *text2 = "<!ATTLIST doc a CDATA 'value'>";
  const char *text = NULL;
  XML_Parser ext_parser;
  int parse_res;

  UNUSED_P(base);
  ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
  if (ext_parser == NULL)
    return XML_STATUS_ERROR;
  if (systemId != NULL && ! xcstrcmp(systemId, XCS("http://example.org/"))) {
    text = text1;
  } else if (publicId != NULL && ! xcstrcmp(publicId, XCS("foo"))) {
    text = text2;
  } else
    fail("Unexpected parameters to external entity parser");
  assert(text != NULL);
  parse_res
      = _XML_Parse_SINGLE_BYTES(ext_parser, text, (int)strlen(text), XML_TRUE);
  XML_ParserFree(ext_parser);
  return parse_res;
}

START_TEST(test_standalone_parameter_entity) {
  const char *text = "<?xml version='1.0' standalone='yes'?>\n"
                     "<!DOCTYPE doc SYSTEM 'http://example.org/' [\n"
                     "<!ENTITY % entity '<!ELEMENT doc (#PCDATA)>'>\n"
                     "%entity;\n"
                     "]>\n"
                     "<doc></doc>";
  char dtd_data[] = "<!ENTITY % e1 'foo'>\n";

  XML_SetUserData(g_parser, dtd_data);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_public);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Test skipping of parameter entity in an external DTD */
/* Derived from ibm/invalid/P69/ibm69i01.xml */
START_TEST(test_skipped_parameter_entity) {
  const char *text = "<?xml version='1.0'?>\n"
                     "<!DOCTYPE root SYSTEM 'http://example.org/dtd.ent' [\n"
                     "<!ELEMENT root (#PCDATA|a)* >\n"
                     "]>\n"
                     "<root></root>";
  ExtTest dtd_data = {"%pe2;", NULL, NULL};

  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader);
  XML_SetUserData(g_parser, &dtd_data);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetSkippedEntityHandler(g_parser, dummy_skip_handler);
  init_dummy_handlers();
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  if (get_dummy_handler_flags() != DUMMY_SKIP_HANDLER_FLAG)
    fail("Skip handler not executed");
}
END_TEST

/* Test recursive parameter entity definition rejected in external DTD */
START_TEST(test_recursive_external_parameter_entity) {
  const char *text = "<?xml version='1.0'?>\n"
                     "<!DOCTYPE root SYSTEM 'http://example.org/dtd.ent' [\n"
                     "<!ELEMENT root (#PCDATA|a)* >\n"
                     "]>\n"
                     "<root></root>";
  ExtFaults dtd_data = {"<!ENTITY % pe2 '&#37;pe2;'>\n%pe2;",
                        "Recursive external parameter entity not faulted", NULL,
                        XML_ERROR_RECURSIVE_ENTITY_REF};

  XML_SetExternalEntityRefHandler(g_parser, external_entity_faulter);
  XML_SetUserData(g_parser, &dtd_data);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  expect_failure(text, XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                 "Recursive external parameter not spotted");
}
END_TEST

/* Test undefined parameter entity in external entity handler */
static int XMLCALL
external_entity_devaluer(XML_Parser parser, const XML_Char *context,
                         const XML_Char *base, const XML_Char *systemId,
                         const XML_Char *publicId) {
  const char *text = "<!ELEMENT doc EMPTY>\n"
                     "<!ENTITY % e1 SYSTEM 'bar'>\n"
                     "%e1;\n";
  XML_Parser ext_parser;
  intptr_t clear_handler = (intptr_t)XML_GetUserData(parser);

  UNUSED_P(base);
  UNUSED_P(publicId);
  if (systemId == NULL || ! xcstrcmp(systemId, XCS("bar")))
    return XML_STATUS_OK;
  if (xcstrcmp(systemId, XCS("foo")))
    fail("Unexpected system ID");
  ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
  if (ext_parser == NULL)
    fail("Could note create external entity parser");
  if (clear_handler)
    XML_SetExternalEntityRefHandler(ext_parser, NULL);
  if (_XML_Parse_SINGLE_BYTES(ext_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(ext_parser);

  XML_ParserFree(ext_parser);
  return XML_STATUS_OK;
}

START_TEST(test_undefined_ext_entity_in_external_dtd) {
  const char *text = "<!DOCTYPE doc SYSTEM 'foo'>\n"
                     "<doc></doc>\n";

  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_devaluer);
  XML_SetUserData(g_parser, (void *)(intptr_t)XML_FALSE);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);

  /* Now repeat without the external entity ref handler invoking
   * another copy of itself.
   */
  XML_ParserReset(g_parser, NULL);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_devaluer);
  XML_SetUserData(g_parser, (void *)(intptr_t)XML_TRUE);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

static void XMLCALL
aborting_xdecl_handler(void *userData, const XML_Char *version,
                       const XML_Char *encoding, int standalone) {
  UNUSED_P(userData);
  UNUSED_P(version);
  UNUSED_P(encoding);
  UNUSED_P(standalone);
  XML_StopParser(g_parser, g_resumable);
  XML_SetXmlDeclHandler(g_parser, NULL);
}

/* Test suspending the parse on receiving an XML declaration works */
START_TEST(test_suspend_xdecl) {
  const char *text = long_character_data_text;

  XML_SetXmlDeclHandler(g_parser, aborting_xdecl_handler);
  g_resumable = XML_TRUE;
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      != XML_STATUS_SUSPENDED)
    xml_failure(g_parser);
  if (XML_GetErrorCode(g_parser) != XML_ERROR_NONE)
    xml_failure(g_parser);
  /* Attempt to start a new parse while suspended */
  if (XML_Parse(g_parser, text, (int)strlen(text), XML_TRUE)
      != XML_STATUS_ERROR)
    fail("Attempt to parse while suspended not faulted");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_SUSPENDED)
    fail("Suspended parse not faulted with correct error");
}
END_TEST

/* Test aborting the parse in an epilog works */
static void XMLCALL
selective_aborting_default_handler(void *userData, const XML_Char *s, int len) {
  const XML_Char *match = (const XML_Char *)userData;

  if (match == NULL
      || (xcstrlen(match) == (unsigned)len && ! xcstrncmp(match, s, len))) {
    XML_StopParser(g_parser, g_resumable);
    XML_SetDefaultHandler(g_parser, NULL);
  }
}

START_TEST(test_abort_epilog) {
  const char *text = "<doc></doc>\n\r\n";
  XML_Char match[] = XCS("\r");

  XML_SetDefaultHandler(g_parser, selective_aborting_default_handler);
  XML_SetUserData(g_parser, match);
  g_resumable = XML_FALSE;
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      != XML_STATUS_ERROR)
    fail("Abort not triggered");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_ABORTED)
    xml_failure(g_parser);
}
END_TEST

/* Test a different code path for abort in the epilog */
START_TEST(test_abort_epilog_2) {
  const char *text = "<doc></doc>\n";
  XML_Char match[] = XCS("\n");

  XML_SetDefaultHandler(g_parser, selective_aborting_default_handler);
  XML_SetUserData(g_parser, match);
  g_resumable = XML_FALSE;
  expect_failure(text, XML_ERROR_ABORTED, "Abort not triggered");
}
END_TEST

/* Test suspension from the epilog */
START_TEST(test_suspend_epilog) {
  const char *text = "<doc></doc>\n";
  XML_Char match[] = XCS("\n");

  XML_SetDefaultHandler(g_parser, selective_aborting_default_handler);
  XML_SetUserData(g_parser, match);
  g_resumable = XML_TRUE;
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      != XML_STATUS_SUSPENDED)
    xml_failure(g_parser);
}
END_TEST

static void XMLCALL
suspending_end_handler(void *userData, const XML_Char *s) {
  UNUSED_P(s);
  XML_StopParser((XML_Parser)userData, 1);
}

START_TEST(test_suspend_in_sole_empty_tag) {
  const char *text = "<doc/>";
  enum XML_Status rc;

  XML_SetEndElementHandler(g_parser, suspending_end_handler);
  XML_SetUserData(g_parser, g_parser);
  rc = _XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE);
  if (rc == XML_STATUS_ERROR)
    xml_failure(g_parser);
  else if (rc != XML_STATUS_SUSPENDED)
    fail("Suspend not triggered");
  rc = XML_ResumeParser(g_parser);
  if (rc == XML_STATUS_ERROR)
    xml_failure(g_parser);
  else if (rc != XML_STATUS_OK)
    fail("Resume failed");
}
END_TEST

START_TEST(test_unfinished_epilog) {
  const char *text = "<doc></doc><";

  expect_failure(text, XML_ERROR_UNCLOSED_TOKEN,
                 "Incomplete epilog entry not faulted");
}
END_TEST

START_TEST(test_partial_char_in_epilog) {
  const char *text = "<doc></doc>\xe2\x82";

  /* First check that no fault is raised if the parse is not finished */
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_FALSE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  /* Now check that it is faulted once we finish */
  if (XML_ParseBuffer(g_parser, 0, XML_TRUE) != XML_STATUS_ERROR)
    fail("Partial character in epilog not faulted");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_PARTIAL_CHAR)
    xml_failure(g_parser);
}
END_TEST

/* Test resuming a parse suspended in entity substitution */
static void XMLCALL
start_element_suspender(void *userData, const XML_Char *name,
                        const XML_Char **atts) {
  UNUSED_P(userData);
  UNUSED_P(atts);
  if (! xcstrcmp(name, XCS("suspend")))
    XML_StopParser(g_parser, XML_TRUE);
  if (! xcstrcmp(name, XCS("abort")))
    XML_StopParser(g_parser, XML_FALSE);
}

START_TEST(test_suspend_resume_internal_entity) {
  const char *text
      = "<!DOCTYPE doc [\n"
        "<!ENTITY foo '<suspend>Hi<suspend>Ho</suspend></suspend>'>\n"
        "]>\n"
        "<doc>&foo;</doc>\n";
  const XML_Char *expected1 = XCS("Hi");
  const XML_Char *expected2 = XCS("HiHo");
  CharData storage;

  CharData_Init(&storage);
  XML_SetStartElementHandler(g_parser, start_element_suspender);
  XML_SetCharacterDataHandler(g_parser, accumulate_characters);
  XML_SetUserData(g_parser, &storage);
  if (XML_Parse(g_parser, text, (int)strlen(text), XML_TRUE)
      != XML_STATUS_SUSPENDED)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, XCS(""));
  if (XML_ResumeParser(g_parser) != XML_STATUS_SUSPENDED)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected1);
  if (XML_ResumeParser(g_parser) != XML_STATUS_OK)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected2);
}
END_TEST

static void XMLCALL
suspending_comment_handler(void *userData, const XML_Char *data) {
  UNUSED_P(data);
  XML_Parser parser = (XML_Parser)userData;
  XML_StopParser(parser, XML_TRUE);
}

START_TEST(test_suspend_resume_internal_entity_issue_629) {
  const char *const text
      = "<!DOCTYPE a [<!ENTITY e '<!--COMMENT-->a'>]><a>&e;<b>\n"
        "<"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "/>"
        "</b></a>";
  const size_t firstChunkSizeBytes = 54;

  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, parser);
  XML_SetCommentHandler(parser, suspending_comment_handler);

  if (XML_Parse(parser, text, (int)firstChunkSizeBytes, XML_FALSE)
      != XML_STATUS_SUSPENDED)
    xml_failure(parser);
  if (XML_ResumeParser(parser) != XML_STATUS_OK)
    xml_failure(parser);
  if (XML_Parse(parser, text + firstChunkSizeBytes,
                (int)(strlen(text) - firstChunkSizeBytes), XML_TRUE)
      != XML_STATUS_OK)
    xml_failure(parser);
  XML_ParserFree(parser);
}
END_TEST

/* Test syntax error is caught at parse resumption */
START_TEST(test_resume_entity_with_syntax_error) {
  const char *text = "<!DOCTYPE doc [\n"
                     "<!ENTITY foo '<suspend>Hi</wombat>'>\n"
                     "]>\n"
                     "<doc>&foo;</doc>\n";

  XML_SetStartElementHandler(g_parser, start_element_suspender);
  if (XML_Parse(g_parser, text, (int)strlen(text), XML_TRUE)
      != XML_STATUS_SUSPENDED)
    xml_failure(g_parser);
  if (XML_ResumeParser(g_parser) != XML_STATUS_ERROR)
    fail("Syntax error in entity not faulted");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_TAG_MISMATCH)
    xml_failure(g_parser);
}
END_TEST

/* Test suspending and resuming in a parameter entity substitution */
static void XMLCALL
element_decl_suspender(void *userData, const XML_Char *name,
                       XML_Content *model) {
  UNUSED_P(userData);
  UNUSED_P(name);
  XML_StopParser(g_parser, XML_TRUE);
  XML_FreeContentModel(g_parser, model);
}

START_TEST(test_suspend_resume_parameter_entity) {
  const char *text = "<!DOCTYPE doc [\n"
                     "<!ENTITY % foo '<!ELEMENT doc (#PCDATA)*>'>\n"
                     "%foo;\n"
                     "]>\n"
                     "<doc>Hello, world</doc>";
  const XML_Char *expected = XCS("Hello, world");
  CharData storage;

  CharData_Init(&storage);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetElementDeclHandler(g_parser, element_decl_suspender);
  XML_SetCharacterDataHandler(g_parser, accumulate_characters);
  XML_SetUserData(g_parser, &storage);
  if (XML_Parse(g_parser, text, (int)strlen(text), XML_TRUE)
      != XML_STATUS_SUSPENDED)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, XCS(""));
  if (XML_ResumeParser(g_parser) != XML_STATUS_OK)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test attempting to use parser after an error is faulted */
START_TEST(test_restart_on_error) {
  const char *text = "<$doc><doc></doc>";

  if (XML_Parse(g_parser, text, (int)strlen(text), XML_TRUE)
      != XML_STATUS_ERROR)
    fail("Invalid tag name not faulted");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_INVALID_TOKEN)
    xml_failure(g_parser);
  if (XML_Parse(g_parser, NULL, 0, XML_TRUE) != XML_STATUS_ERROR)
    fail("Restarting invalid parse not faulted");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_INVALID_TOKEN)
    xml_failure(g_parser);
}
END_TEST

/* Test that angle brackets in an attribute default value are faulted */
START_TEST(test_reject_lt_in_attribute_value) {
  const char *text = "<!DOCTYPE doc [<!ATTLIST doc a CDATA '<bar>'>]>\n"
                     "<doc></doc>";

  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Bad attribute default not faulted");
}
END_TEST

START_TEST(test_reject_unfinished_param_in_att_value) {
  const char *text = "<!DOCTYPE doc [<!ATTLIST doc a CDATA '&foo'>]>\n"
                     "<doc></doc>";

  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Bad attribute default not faulted");
}
END_TEST

START_TEST(test_trailing_cr_in_att_value) {
  const char *text = "<doc a='value\r'/>";

  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Try parsing a general entity within a parameter entity in a
 * standalone internal DTD.  Covers a corner case in the parser.
 */
START_TEST(test_standalone_internal_entity) {
  const char *text = "<?xml version='1.0' standalone='yes' ?>\n"
                     "<!DOCTYPE doc [\n"
                     "  <!ELEMENT doc (#PCDATA)>\n"
                     "  <!ENTITY % pe '<!ATTLIST doc att2 CDATA \"&ge;\">'>\n"
                     "  <!ENTITY ge 'AttDefaultValue'>\n"
                     "  %pe;\n"
                     "]>\n"
                     "<doc att2='any'/>";

  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Test that a reference to an unknown external entity is skipped */
START_TEST(test_skipped_external_entity) {
  const char *text = "<!DOCTYPE doc SYSTEM 'http://example.org/'>\n"
                     "<doc></doc>\n";
  ExtTest test_data = {"<!ELEMENT doc EMPTY>\n"
                       "<!ENTITY % e2 '%e1;'>\n",
                       NULL, NULL};

  XML_SetUserData(g_parser, &test_data);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Test a different form of unknown external entity */
typedef struct ext_hdlr_data {
  const char *parse_text;
  XML_ExternalEntityRefHandler handler;
} ExtHdlrData;

static int XMLCALL
external_entity_oneshot_loader(XML_Parser parser, const XML_Char *context,
                               const XML_Char *base, const XML_Char *systemId,
                               const XML_Char *publicId) {
  ExtHdlrData *test_data = (ExtHdlrData *)XML_GetUserData(parser);
  XML_Parser ext_parser;

  UNUSED_P(base);
  UNUSED_P(systemId);
  UNUSED_P(publicId);
  ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
  if (ext_parser == NULL)
    fail("Could not create external entity parser.");
  /* Use the requested entity parser for further externals */
  XML_SetExternalEntityRefHandler(ext_parser, test_data->handler);
  if (_XML_Parse_SINGLE_BYTES(ext_parser, test_data->parse_text,
                              (int)strlen(test_data->parse_text), XML_TRUE)
      == XML_STATUS_ERROR) {
    xml_failure(ext_parser);
  }

  XML_ParserFree(ext_parser);
  return XML_STATUS_OK;
}

START_TEST(test_skipped_null_loaded_ext_entity) {
  const char *text = "<!DOCTYPE doc SYSTEM 'http://example.org/one.ent'>\n"
                     "<doc />";
  ExtHdlrData test_data
      = {"<!ENTITY % pe1 SYSTEM 'http://example.org/two.ent'>\n"
         "<!ENTITY % pe2 '%pe1;'>\n"
         "%pe2;\n",
         external_entity_null_loader};

  XML_SetUserData(g_parser, &test_data);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_oneshot_loader);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

START_TEST(test_skipped_unloaded_ext_entity) {
  const char *text = "<!DOCTYPE doc SYSTEM 'http://example.org/one.ent'>\n"
                     "<doc />";
  ExtHdlrData test_data
      = {"<!ENTITY % pe1 SYSTEM 'http://example.org/two.ent'>\n"
         "<!ENTITY % pe2 '%pe1;'>\n"
         "%pe2;\n",
         NULL};

  XML_SetUserData(g_parser, &test_data);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_oneshot_loader);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Test that a parameter entity value ending with a carriage return
 * has it translated internally into a newline.
 */
START_TEST(test_param_entity_with_trailing_cr) {
#define PARAM_ENTITY_NAME "pe"
#define PARAM_ENTITY_CORE_VALUE "<!ATTLIST doc att CDATA \"default\">"
  const char *text = "<!DOCTYPE doc SYSTEM 'http://example.org/'>\n"
                     "<doc/>";
  ExtTest test_data
      = {"<!ENTITY % " PARAM_ENTITY_NAME " '" PARAM_ENTITY_CORE_VALUE "\r'>\n"
         "%" PARAM_ENTITY_NAME ";\n",
         NULL, NULL};

  XML_SetUserData(g_parser, &test_data);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader);
  XML_SetEntityDeclHandler(g_parser, param_entity_match_handler);
  param_entity_match_init(XCS(PARAM_ENTITY_NAME),
                          XCS(PARAM_ENTITY_CORE_VALUE) XCS("\n"));
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  int entity_match_flag = get_param_entity_match_flag();
  if (entity_match_flag == ENTITY_MATCH_FAIL)
    fail("Parameter entity CR->NEWLINE conversion failed");
  else if (entity_match_flag == ENTITY_MATCH_NOT_FOUND)
    fail("Parameter entity not parsed");
}
#undef PARAM_ENTITY_NAME
#undef PARAM_ENTITY_CORE_VALUE
END_TEST

START_TEST(test_invalid_character_entity) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY entity '&#x110000;'>\n"
                     "]>\n"
                     "<doc>&entity;</doc>";

  expect_failure(text, XML_ERROR_BAD_CHAR_REF,
                 "Out of range character reference not faulted");
}
END_TEST

START_TEST(test_invalid_character_entity_2) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY entity '&#xg0;'>\n"
                     "]>\n"
                     "<doc>&entity;</doc>";

  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Out of range character reference not faulted");
}
END_TEST

START_TEST(test_invalid_character_entity_3) {
  const char text[] =
      /* <!DOCTYPE doc [\n */
      "\0<\0!\0D\0O\0C\0T\0Y\0P\0E\0 \0d\0o\0c\0 \0[\0\n"
      /* U+0E04 = KHO KHWAI
       * U+0E08 = CHO CHAN */
      /* <!ENTITY entity '&\u0e04\u0e08;'>\n */
      "\0<\0!\0E\0N\0T\0I\0T\0Y\0 \0e\0n\0t\0i\0t\0y\0 "
      "\0'\0&\x0e\x04\x0e\x08\0;\0'\0>\0\n"
      /* ]>\n */
      "\0]\0>\0\n"
      /* <doc>&entity;</doc> */
      "\0<\0d\0o\0c\0>\0&\0e\0n\0t\0i\0t\0y\0;\0<\0/\0d\0o\0c\0>";

  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      != XML_STATUS_ERROR)
    fail("Invalid start of entity name not faulted");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_UNDEFINED_ENTITY)
    xml_failure(g_parser);
}
END_TEST

START_TEST(test_invalid_character_entity_4) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY entity '&#1114112;'>\n" /* = &#x110000 */
                     "]>\n"
                     "<doc>&entity;</doc>";

  expect_failure(text, XML_ERROR_BAD_CHAR_REF,
                 "Out of range character reference not faulted");
}
END_TEST

/* Test that processing instructions are picked up by a default handler */
START_TEST(test_pi_handled_in_default) {
  const char *text = "<?test processing instruction?>\n<doc/>";
  const XML_Char *expected = XCS("<?test processing instruction?>\n<doc/>");
  CharData storage;

  CharData_Init(&storage);
  XML_SetDefaultHandler(g_parser, accumulate_characters);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test that comments are picked up by a default handler */
START_TEST(test_comment_handled_in_default) {
  const char *text = "<!-- This is a comment -->\n<doc/>";
  const XML_Char *expected = XCS("<!-- This is a comment -->\n<doc/>");
  CharData storage;

  CharData_Init(&storage);
  XML_SetDefaultHandler(g_parser, accumulate_characters);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test PIs that look almost but not quite like XML declarations */
static void XMLCALL
accumulate_pi_characters(void *userData, const XML_Char *target,
                         const XML_Char *data) {
  CharData *storage = (CharData *)userData;

  CharData_AppendXMLChars(storage, target, -1);
  CharData_AppendXMLChars(storage, XCS(": "), 2);
  CharData_AppendXMLChars(storage, data, -1);
  CharData_AppendXMLChars(storage, XCS("\n"), 1);
}

START_TEST(test_pi_yml) {
  const char *text = "<?yml something like data?><doc/>";
  const XML_Char *expected = XCS("yml: something like data\n");
  CharData storage;

  CharData_Init(&storage);
  XML_SetProcessingInstructionHandler(g_parser, accumulate_pi_characters);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_pi_xnl) {
  const char *text = "<?xnl nothing like data?><doc/>";
  const XML_Char *expected = XCS("xnl: nothing like data\n");
  CharData storage;

  CharData_Init(&storage);
  XML_SetProcessingInstructionHandler(g_parser, accumulate_pi_characters);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_pi_xmm) {
  const char *text = "<?xmm everything like data?><doc/>";
  const XML_Char *expected = XCS("xmm: everything like data\n");
  CharData storage;

  CharData_Init(&storage);
  XML_SetProcessingInstructionHandler(g_parser, accumulate_pi_characters);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_utf16_pi) {
  const char text[] =
      /* <?{KHO KHWAI}{CHO CHAN}?>
       * where {KHO KHWAI} = U+0E04
       * and   {CHO CHAN}  = U+0E08
       */
      "<\0?\0\x04\x0e\x08\x0e?\0>\0"
      /* <q/> */
      "<\0q\0/\0>\0";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x0e04\x0e08: \n");
#else
  const XML_Char *expected = XCS("\xe0\xb8\x84\xe0\xb8\x88: \n");
#endif
  CharData storage;

  CharData_Init(&storage);
  XML_SetProcessingInstructionHandler(g_parser, accumulate_pi_characters);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_utf16_be_pi) {
  const char text[] =
      /* <?{KHO KHWAI}{CHO CHAN}?>
       * where {KHO KHWAI} = U+0E04
       * and   {CHO CHAN}  = U+0E08
       */
      "\0<\0?\x0e\x04\x0e\x08\0?\0>"
      /* <q/> */
      "\0<\0q\0/\0>";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x0e04\x0e08: \n");
#else
  const XML_Char *expected = XCS("\xe0\xb8\x84\xe0\xb8\x88: \n");
#endif
  CharData storage;

  CharData_Init(&storage);
  XML_SetProcessingInstructionHandler(g_parser, accumulate_pi_characters);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test that comments can be picked up and translated */
static void XMLCALL
accumulate_comment(void *userData, const XML_Char *data) {
  CharData *storage = (CharData *)userData;

  CharData_AppendXMLChars(storage, data, -1);
}

START_TEST(test_utf16_be_comment) {
  const char text[] =
      /* <!-- Comment A --> */
      "\0<\0!\0-\0-\0 \0C\0o\0m\0m\0e\0n\0t\0 \0A\0 \0-\0-\0>\0\n"
      /* <doc/> */
      "\0<\0d\0o\0c\0/\0>";
  const XML_Char *expected = XCS(" Comment A ");
  CharData storage;

  CharData_Init(&storage);
  XML_SetCommentHandler(g_parser, accumulate_comment);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_utf16_le_comment) {
  const char text[] =
      /* <!-- Comment B --> */
      "<\0!\0-\0-\0 \0C\0o\0m\0m\0e\0n\0t\0 \0B\0 \0-\0-\0>\0\n\0"
      /* <doc/> */
      "<\0d\0o\0c\0/\0>\0";
  const XML_Char *expected = XCS(" Comment B ");
  CharData storage;

  CharData_Init(&storage);
  XML_SetCommentHandler(g_parser, accumulate_comment);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test that the unknown encoding handler with map entries that expect
 * conversion but no conversion function is faulted
 */
static int XMLCALL
failing_converter(void *data, const char *s) {
  UNUSED_P(data);
  UNUSED_P(s);
  /* Always claim to have failed */
  return -1;
}

static int XMLCALL
prefix_converter(void *data, const char *s) {
  UNUSED_P(data);
  /* If the first byte is 0xff, raise an error */
  if (s[0] == (char)-1)
    return -1;
  /* Just add the low bits of the first byte to the second */
  return (s[1] + (s[0] & 0x7f)) & 0x01ff;
}

static int XMLCALL
MiscEncodingHandler(void *data, const XML_Char *encoding, XML_Encoding *info) {
  int i;
  int high_map = -2; /* Assume a 2-byte sequence */

  if (! xcstrcmp(encoding, XCS("invalid-9"))
      || ! xcstrcmp(encoding, XCS("ascii-like"))
      || ! xcstrcmp(encoding, XCS("invalid-len"))
      || ! xcstrcmp(encoding, XCS("invalid-a"))
      || ! xcstrcmp(encoding, XCS("invalid-surrogate"))
      || ! xcstrcmp(encoding, XCS("invalid-high")))
    high_map = -1;

  for (i = 0; i < 128; ++i)
    info->map[i] = i;
  for (; i < 256; ++i)
    info->map[i] = high_map;

  /* If required, put an invalid value in the ASCII entries */
  if (! xcstrcmp(encoding, XCS("invalid-9")))
    info->map[9] = 5;
  /* If required, have a top-bit set character starts a 5-byte sequence */
  if (! xcstrcmp(encoding, XCS("invalid-len")))
    info->map[0x81] = -5;
  /* If required, make a top-bit set character a valid ASCII character */
  if (! xcstrcmp(encoding, XCS("invalid-a")))
    info->map[0x82] = 'a';
  /* If required, give a top-bit set character a forbidden value,
   * what would otherwise be the first of a surrogate pair.
   */
  if (! xcstrcmp(encoding, XCS("invalid-surrogate")))
    info->map[0x83] = 0xd801;
  /* If required, give a top-bit set character too high a value */
  if (! xcstrcmp(encoding, XCS("invalid-high")))
    info->map[0x84] = 0x010101;

  info->data = data;
  info->release = NULL;
  if (! xcstrcmp(encoding, XCS("failing-conv")))
    info->convert = failing_converter;
  else if (! xcstrcmp(encoding, XCS("prefix-conv")))
    info->convert = prefix_converter;
  else
    info->convert = NULL;
  return XML_STATUS_OK;
}

START_TEST(test_missing_encoding_conversion_fn) {
  const char *text = "<?xml version='1.0' encoding='no-conv'?>\n"
                     "<doc>\x81</doc>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  /* MiscEncodingHandler sets up an encoding with every top-bit-set
   * character introducing a two-byte sequence.  For this, it
   * requires a convert function.  The above function call doesn't
   * pass one through, so when BadEncodingHandler actually gets
   * called it should supply an invalid encoding.
   */
  expect_failure(text, XML_ERROR_UNKNOWN_ENCODING,
                 "Encoding with missing convert() not faulted");
}
END_TEST

START_TEST(test_failing_encoding_conversion_fn) {
  const char *text = "<?xml version='1.0' encoding='failing-conv'?>\n"
                     "<doc>\x81</doc>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  /* BadEncodingHandler sets up an encoding with every top-bit-set
   * character introducing a two-byte sequence.  For this, it
   * requires a convert function.  The above function call passes
   * one that insists all possible sequences are invalid anyway.
   */
  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Encoding with failing convert() not faulted");
}
END_TEST

/* Test unknown encoding conversions */
START_TEST(test_unknown_encoding_success) {
  const char *text = "<?xml version='1.0' encoding='prefix-conv'?>\n"
                     /* Equivalent to <eoc>Hello, world</eoc> */
                     "<\x81\x64\x80oc>Hello, world</\x81\x64\x80oc>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  run_character_check(text, XCS("Hello, world"));
}
END_TEST

/* Test bad name character in unknown encoding */
START_TEST(test_unknown_encoding_bad_name) {
  const char *text = "<?xml version='1.0' encoding='prefix-conv'?>\n"
                     "<\xff\x64oc>Hello, world</\xff\x64oc>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Bad name start in unknown encoding not faulted");
}
END_TEST

/* Test bad mid-name character in unknown encoding */
START_TEST(test_unknown_encoding_bad_name_2) {
  const char *text = "<?xml version='1.0' encoding='prefix-conv'?>\n"
                     "<d\xffoc>Hello, world</d\xffoc>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Bad name in unknown encoding not faulted");
}
END_TEST

/* Test element name that is long enough to fill the conversion buffer
 * in an unknown encoding, finishing with an encoded character.
 */
START_TEST(test_unknown_encoding_long_name_1) {
  const char *text = "<?xml version='1.0' encoding='prefix-conv'?>\n"
                     "<abcdefghabcdefghabcdefghijkl\x80m\x80n\x80o\x80p>"
                     "Hi"
                     "</abcdefghabcdefghabcdefghijkl\x80m\x80n\x80o\x80p>";
  const XML_Char *expected = XCS("abcdefghabcdefghabcdefghijklmnop");
  CharData storage;

  CharData_Init(&storage);
  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  XML_SetStartElementHandler(g_parser, record_element_start_handler);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test element name that is long enough to fill the conversion buffer
 * in an unknown encoding, finishing with an simple character.
 */
START_TEST(test_unknown_encoding_long_name_2) {
  const char *text = "<?xml version='1.0' encoding='prefix-conv'?>\n"
                     "<abcdefghabcdefghabcdefghijklmnop>"
                     "Hi"
                     "</abcdefghabcdefghabcdefghijklmnop>";
  const XML_Char *expected = XCS("abcdefghabcdefghabcdefghijklmnop");
  CharData storage;

  CharData_Init(&storage);
  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  XML_SetStartElementHandler(g_parser, record_element_start_handler);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_invalid_unknown_encoding) {
  const char *text = "<?xml version='1.0' encoding='invalid-9'?>\n"
                     "<doc>Hello world</doc>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  expect_failure(text, XML_ERROR_UNKNOWN_ENCODING,
                 "Invalid unknown encoding not faulted");
}
END_TEST

START_TEST(test_unknown_ascii_encoding_ok) {
  const char *text = "<?xml version='1.0' encoding='ascii-like'?>\n"
                     "<doc>Hello, world</doc>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  run_character_check(text, XCS("Hello, world"));
}
END_TEST

START_TEST(test_unknown_ascii_encoding_fail) {
  const char *text = "<?xml version='1.0' encoding='ascii-like'?>\n"
                     "<doc>Hello, \x80 world</doc>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Invalid character not faulted");
}
END_TEST

START_TEST(test_unknown_encoding_invalid_length) {
  const char *text = "<?xml version='1.0' encoding='invalid-len'?>\n"
                     "<doc>Hello, world</doc>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  expect_failure(text, XML_ERROR_UNKNOWN_ENCODING,
                 "Invalid unknown encoding not faulted");
}
END_TEST

START_TEST(test_unknown_encoding_invalid_topbit) {
  const char *text = "<?xml version='1.0' encoding='invalid-a'?>\n"
                     "<doc>Hello, world</doc>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  expect_failure(text, XML_ERROR_UNKNOWN_ENCODING,
                 "Invalid unknown encoding not faulted");
}
END_TEST

START_TEST(test_unknown_encoding_invalid_surrogate) {
  const char *text = "<?xml version='1.0' encoding='invalid-surrogate'?>\n"
                     "<doc>Hello, \x82 world</doc>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Invalid unknown encoding not faulted");
}
END_TEST

START_TEST(test_unknown_encoding_invalid_high) {
  const char *text = "<?xml version='1.0' encoding='invalid-high'?>\n"
                     "<doc>Hello, world</doc>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  expect_failure(text, XML_ERROR_UNKNOWN_ENCODING,
                 "Invalid unknown encoding not faulted");
}
END_TEST

START_TEST(test_unknown_encoding_invalid_attr_value) {
  const char *text = "<?xml version='1.0' encoding='prefix-conv'?>\n"
                     "<doc attr='\xff\x30'/>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Invalid attribute valid not faulted");
}
END_TEST

/* Test an external entity parser set to use latin-1 detects UTF-16
 * BOMs correctly.
 */
enum ee_parse_flags { EE_PARSE_NONE = 0x00, EE_PARSE_FULL_BUFFER = 0x01 };

typedef struct ExtTest2 {
  const char *parse_text;
  int parse_len;
  const XML_Char *encoding;
  CharData *storage;
  enum ee_parse_flags flags;
} ExtTest2;

static int XMLCALL
external_entity_loader2(XML_Parser parser, const XML_Char *context,
                        const XML_Char *base, const XML_Char *systemId,
                        const XML_Char *publicId) {
  ExtTest2 *test_data = (ExtTest2 *)XML_GetUserData(parser);
  XML_Parser extparser;

  UNUSED_P(base);
  UNUSED_P(systemId);
  UNUSED_P(publicId);
  extparser = XML_ExternalEntityParserCreate(parser, context, NULL);
  if (extparser == NULL)
    fail("Coulr not create external entity parser");
  if (test_data->encoding != NULL) {
    if (! XML_SetEncoding(extparser, test_data->encoding))
      fail("XML_SetEncoding() ignored for external entity");
  }
  if (test_data->flags & EE_PARSE_FULL_BUFFER) {
    if (XML_Parse(extparser, test_data->parse_text, test_data->parse_len,
                  XML_TRUE)
        == XML_STATUS_ERROR) {
      xml_failure(extparser);
    }
  } else if (_XML_Parse_SINGLE_BYTES(extparser, test_data->parse_text,
                                     test_data->parse_len, XML_TRUE)
             == XML_STATUS_ERROR) {
    xml_failure(extparser);
  }

  XML_ParserFree(extparser);
  return XML_STATUS_OK;
}

/* Test that UTF-16 BOM does not select UTF-16 given explicit encoding */
static void XMLCALL
ext2_accumulate_characters(void *userData, const XML_Char *s, int len) {
  ExtTest2 *test_data = (ExtTest2 *)userData;
  accumulate_characters(test_data->storage, s, len);
}

START_TEST(test_ext_entity_latin1_utf16le_bom) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";
  ExtTest2 test_data
      = {/* If UTF-16, 0xfeff is the BOM and 0x204c is black left bullet */
         /* If Latin-1, 0xff = Y-diaeresis, 0xfe = lowercase thorn,
          *   0x4c = L and 0x20 is a space
          */
         "\xff\xfe\x4c\x20", 4, XCS("iso-8859-1"), NULL, EE_PARSE_NONE};
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x00ff\x00feL ");
#else
  /* In UTF-8, y-diaeresis is 0xc3 0xbf, lowercase thorn is 0xc3 0xbe */
  const XML_Char *expected = XCS("\xc3\xbf\xc3\xbeL ");
#endif
  CharData storage;

  CharData_Init(&storage);
  test_data.storage = &storage;
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader2);
  XML_SetUserData(g_parser, &test_data);
  XML_SetCharacterDataHandler(g_parser, ext2_accumulate_characters);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_ext_entity_latin1_utf16be_bom) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";
  ExtTest2 test_data
      = {/* If UTF-16, 0xfeff is the BOM and 0x204c is black left bullet */
         /* If Latin-1, 0xff = Y-diaeresis, 0xfe = lowercase thorn,
          *   0x4c = L and 0x20 is a space
          */
         "\xfe\xff\x20\x4c", 4, XCS("iso-8859-1"), NULL, EE_PARSE_NONE};
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x00fe\x00ff L");
#else
  /* In UTF-8, y-diaeresis is 0xc3 0xbf, lowercase thorn is 0xc3 0xbe */
  const XML_Char *expected = XCS("\xc3\xbe\xc3\xbf L");
#endif
  CharData storage;

  CharData_Init(&storage);
  test_data.storage = &storage;
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader2);
  XML_SetUserData(g_parser, &test_data);
  XML_SetCharacterDataHandler(g_parser, ext2_accumulate_characters);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Parsing the full buffer rather than a byte at a time makes a
 * difference to the encoding scanning code, so repeat the above tests
 * without breaking them down by byte.
 */
START_TEST(test_ext_entity_latin1_utf16le_bom2) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";
  ExtTest2 test_data
      = {/* If UTF-16, 0xfeff is the BOM and 0x204c is black left bullet */
         /* If Latin-1, 0xff = Y-diaeresis, 0xfe = lowercase thorn,
          *   0x4c = L and 0x20 is a space
          */
         "\xff\xfe\x4c\x20", 4, XCS("iso-8859-1"), NULL, EE_PARSE_FULL_BUFFER};
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x00ff\x00feL ");
#else
  /* In UTF-8, y-diaeresis is 0xc3 0xbf, lowercase thorn is 0xc3 0xbe */
  const XML_Char *expected = XCS("\xc3\xbf\xc3\xbeL ");
#endif
  CharData storage;

  CharData_Init(&storage);
  test_data.storage = &storage;
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader2);
  XML_SetUserData(g_parser, &test_data);
  XML_SetCharacterDataHandler(g_parser, ext2_accumulate_characters);
  if (XML_Parse(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_ext_entity_latin1_utf16be_bom2) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";
  ExtTest2 test_data
      = {/* If UTF-16, 0xfeff is the BOM and 0x204c is black left bullet */
         /* If Latin-1, 0xff = Y-diaeresis, 0xfe = lowercase thorn,
          *   0x4c = L and 0x20 is a space
          */
         "\xfe\xff\x20\x4c", 4, XCS("iso-8859-1"), NULL, EE_PARSE_FULL_BUFFER};
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x00fe\x00ff L");
#else
  /* In UTF-8, y-diaeresis is 0xc3 0xbf, lowercase thorn is 0xc3 0xbe */
  const XML_Char *expected = "\xc3\xbe\xc3\xbf L";
#endif
  CharData storage;

  CharData_Init(&storage);
  test_data.storage = &storage;
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader2);
  XML_SetUserData(g_parser, &test_data);
  XML_SetCharacterDataHandler(g_parser, ext2_accumulate_characters);
  if (XML_Parse(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test little-endian UTF-16 given an explicit big-endian encoding */
START_TEST(test_ext_entity_utf16_be) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";
  ExtTest2 test_data
      = {"<\0e\0/\0>\0", 8, XCS("utf-16be"), NULL, EE_PARSE_NONE};
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x3c00\x6500\x2f00\x3e00");
#else
  const XML_Char *expected = XCS("\xe3\xb0\x80"   /* U+3C00 */
                                 "\xe6\x94\x80"   /* U+6500 */
                                 "\xe2\xbc\x80"   /* U+2F00 */
                                 "\xe3\xb8\x80"); /* U+3E00 */
#endif
  CharData storage;

  CharData_Init(&storage);
  test_data.storage = &storage;
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader2);
  XML_SetUserData(g_parser, &test_data);
  XML_SetCharacterDataHandler(g_parser, ext2_accumulate_characters);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test big-endian UTF-16 given an explicit little-endian encoding */
START_TEST(test_ext_entity_utf16_le) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";
  ExtTest2 test_data
      = {"\0<\0e\0/\0>", 8, XCS("utf-16le"), NULL, EE_PARSE_NONE};
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x3c00\x6500\x2f00\x3e00");
#else
  const XML_Char *expected = XCS("\xe3\xb0\x80"   /* U+3C00 */
                                 "\xe6\x94\x80"   /* U+6500 */
                                 "\xe2\xbc\x80"   /* U+2F00 */
                                 "\xe3\xb8\x80"); /* U+3E00 */
#endif
  CharData storage;

  CharData_Init(&storage);
  test_data.storage = &storage;
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader2);
  XML_SetUserData(g_parser, &test_data);
  XML_SetCharacterDataHandler(g_parser, ext2_accumulate_characters);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test little-endian UTF-16 given no explicit encoding.
 * The existing default encoding (UTF-8) is assumed to hold without a
 * BOM to contradict it, so the entity value will in fact provoke an
 * error because 0x00 is not a valid XML character.  We parse the
 * whole buffer in one go rather than feeding it in byte by byte to
 * exercise different code paths in the initial scanning routines.
 */
typedef struct ExtFaults2 {
  const char *parse_text;
  int parse_len;
  const char *fail_text;
  const XML_Char *encoding;
  enum XML_Error error;
} ExtFaults2;

static int XMLCALL
external_entity_faulter2(XML_Parser parser, const XML_Char *context,
                         const XML_Char *base, const XML_Char *systemId,
                         const XML_Char *publicId) {
  ExtFaults2 *test_data = (ExtFaults2 *)XML_GetUserData(parser);
  XML_Parser extparser;

  UNUSED_P(base);
  UNUSED_P(systemId);
  UNUSED_P(publicId);
  extparser = XML_ExternalEntityParserCreate(parser, context, NULL);
  if (extparser == NULL)
    fail("Could not create external entity parser");
  if (test_data->encoding != NULL) {
    if (! XML_SetEncoding(extparser, test_data->encoding))
      fail("XML_SetEncoding() ignored for external entity");
  }
  if (XML_Parse(extparser, test_data->parse_text, test_data->parse_len,
                XML_TRUE)
      != XML_STATUS_ERROR)
    fail(test_data->fail_text);
  if (XML_GetErrorCode(extparser) != test_data->error)
    xml_failure(extparser);

  XML_ParserFree(extparser);
  return XML_STATUS_ERROR;
}

START_TEST(test_ext_entity_utf16_unknown) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";
  ExtFaults2 test_data
      = {"a\0b\0c\0", 6, "Invalid character in entity not faulted", NULL,
         XML_ERROR_INVALID_TOKEN};

  XML_SetExternalEntityRefHandler(g_parser, external_entity_faulter2);
  XML_SetUserData(g_parser, &test_data);
  expect_failure(text, XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                 "Invalid character should not have been accepted");
}
END_TEST

/* Test not-quite-UTF-8 BOM (0xEF 0xBB 0xBF) */
START_TEST(test_ext_entity_utf8_non_bom) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";
  ExtTest2 test_data
      = {"\xef\xbb\x80", /* Arabic letter DAD medial form, U+FEC0 */
         3, NULL, NULL, EE_PARSE_NONE};
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\xfec0");
#else
  const XML_Char *expected = XCS("\xef\xbb\x80");
#endif
  CharData storage;

  CharData_Init(&storage);
  test_data.storage = &storage;
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader2);
  XML_SetUserData(g_parser, &test_data);
  XML_SetCharacterDataHandler(g_parser, ext2_accumulate_characters);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test that UTF-8 in a CDATA section is correctly passed through */
START_TEST(test_utf8_in_cdata_section) {
  const char *text = "<doc><![CDATA[one \xc3\xa9 two]]></doc>";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("one \x00e9 two");
#else
  const XML_Char *expected = XCS("one \xc3\xa9 two");
#endif

  run_character_check(text, expected);
}
END_TEST

/* Test that little-endian UTF-16 in a CDATA section is handled */
START_TEST(test_utf8_in_cdata_section_2) {
  const char *text = "<doc><![CDATA[\xc3\xa9]\xc3\xa9two]]></doc>";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x00e9]\x00e9two");
#else
  const XML_Char *expected = XCS("\xc3\xa9]\xc3\xa9two");
#endif

  run_character_check(text, expected);
}
END_TEST

START_TEST(test_utf8_in_start_tags) {
  struct test_case {
    bool goodName;
    bool goodNameStart;
    const char *tagName;
  };

  // The idea with the tests below is this:
  // We want to cover 1-, 2- and 3-byte sequences, 4-byte sequences
  // go to isNever and are hence not a concern.
  //
  // We start with a character that is a valid name character
  // (or even name-start character, see XML 1.0r4 spec) and then we flip
  // single bits at places where (1) the result leaves the UTF-8 encoding space
  // and (2) we stay in the same n-byte sequence family.
  //
  // The flipped bits are highlighted in angle brackets in comments,
  // e.g. "[<1>011 1001]" means we had [0011 1001] but we now flipped
  // the most significant bit to 1 to leave UTF-8 encoding space.
  struct test_case cases[] = {
      // 1-byte UTF-8: [0xxx xxxx]
      {true, true, "\x3A"},   // [0011 1010] = ASCII colon ':'
      {false, false, "\xBA"}, // [<1>011 1010]
      {true, false, "\x39"},  // [0011 1001] = ASCII nine '9'
      {false, false, "\xB9"}, // [<1>011 1001]

      // 2-byte UTF-8: [110x xxxx] [10xx xxxx]
      {true, true, "\xDB\xA5"},   // [1101 1011] [1010 0101] =
                                  // Arabic small waw U+06E5
      {false, false, "\x9B\xA5"}, // [1<0>01 1011] [1010 0101]
      {false, false, "\xDB\x25"}, // [1101 1011] [<0>010 0101]
      {false, false, "\xDB\xE5"}, // [1101 1011] [1<1>10 0101]
      {true, false, "\xCC\x81"},  // [1100 1100] [1000 0001] =
                                  // combining char U+0301
      {false, false, "\x8C\x81"}, // [1<0>00 1100] [1000 0001]
      {false, false, "\xCC\x01"}, // [1100 1100] [<0>000 0001]
      {false, false, "\xCC\xC1"}, // [1100 1100] [1<1>00 0001]

      // 3-byte UTF-8: [1110 xxxx] [10xx xxxx] [10xxxxxx]
      {true, true, "\xE0\xA4\x85"},   // [1110 0000] [1010 0100] [1000 0101] =
                                      // Devanagari Letter A U+0905
      {false, false, "\xA0\xA4\x85"}, // [1<0>10 0000] [1010 0100] [1000 0101]
      {false, false, "\xE0\x24\x85"}, // [1110 0000] [<0>010 0100] [1000 0101]
      {false, false, "\xE0\xE4\x85"}, // [1110 0000] [1<1>10 0100] [1000 0101]
      {false, false, "\xE0\xA4\x05"}, // [1110 0000] [1010 0100] [<0>000 0101]
      {false, false, "\xE0\xA4\xC5"}, // [1110 0000] [1010 0100] [1<1>00 0101]
      {true, false, "\xE0\xA4\x81"},  // [1110 0000] [1010 0100] [1000 0001] =
                                      // combining char U+0901
      {false, false, "\xA0\xA4\x81"}, // [1<0>10 0000] [1010 0100] [1000 0001]
      {false, false, "\xE0\x24\x81"}, // [1110 0000] [<0>010 0100] [1000 0001]
      {false, false, "\xE0\xE4\x81"}, // [1110 0000] [1<1>10 0100] [1000 0001]
      {false, false, "\xE0\xA4\x01"}, // [1110 0000] [1010 0100] [<0>000 0001]
      {false, false, "\xE0\xA4\xC1"}, // [1110 0000] [1010 0100] [1<1>00 0001]
  };
  const bool atNameStart[] = {true, false};

  size_t i = 0;
  char doc[1024];
  size_t failCount = 0;

  for (; i < sizeof(cases) / sizeof(cases[0]); i++) {
    size_t j = 0;
    for (; j < sizeof(atNameStart) / sizeof(atNameStart[0]); j++) {
      const bool expectedSuccess
          = atNameStart[j] ? cases[i].goodNameStart : cases[i].goodName;
      snprintf(doc, sizeof(doc), "<%s%s><!--", atNameStart[j] ? "" : "a",
               cases[i].tagName);
      XML_Parser parser = XML_ParserCreate(NULL);

      const enum XML_Status status
          = XML_Parse(parser, doc, (int)strlen(doc), /*isFinal=*/XML_FALSE);

      bool success = true;
      if ((status == XML_STATUS_OK) != expectedSuccess) {
        success = false;
      }
      if ((status == XML_STATUS_ERROR)
          && (XML_GetErrorCode(parser) != XML_ERROR_INVALID_TOKEN)) {
        success = false;
      }

      if (! success) {
        fprintf(
            stderr,
            "FAIL case %2u (%sat name start, %u-byte sequence, error code %d)\n",
            (unsigned)i + 1u, atNameStart[j] ? "    " : "not ",
            (unsigned)strlen(cases[i].tagName), XML_GetErrorCode(parser));
        failCount++;
      }

      XML_ParserFree(parser);
    }
  }

  if (failCount > 0) {
    fail("UTF-8 regression detected");
  }
}
END_TEST

/* Test trailing spaces in elements are accepted */
static void XMLCALL
record_element_end_handler(void *userData, const XML_Char *name) {
  CharData *storage = (CharData *)userData;

  CharData_AppendXMLChars(storage, XCS("/"), 1);
  CharData_AppendXMLChars(storage, name, -1);
}

START_TEST(test_trailing_spaces_in_elements) {
  const char *text = "<doc   >Hi</doc >";
  const XML_Char *expected = XCS("doc/doc");
  CharData storage;

  CharData_Init(&storage);
  XML_SetElementHandler(g_parser, record_element_start_handler,
                        record_element_end_handler);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_utf16_attribute) {
  const char text[] =
      /* <d {KHO KHWAI}{CHO CHAN}='a'/>
       * where {KHO KHWAI} = U+0E04 = 0xe0 0xb8 0x84 in UTF-8
       * and   {CHO CHAN}  = U+0E08 = 0xe0 0xb8 0x88 in UTF-8
       */
      "<\0d\0 \0\x04\x0e\x08\x0e=\0'\0a\0'\0/\0>\0";
  const XML_Char *expected = XCS("a");
  CharData storage;

  CharData_Init(&storage);
  XML_SetStartElementHandler(g_parser, accumulate_attribute);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_utf16_second_attr) {
  /* <d a='1' {KHO KHWAI}{CHO CHAN}='2'/>
   * where {KHO KHWAI} = U+0E04 = 0xe0 0xb8 0x84 in UTF-8
   * and   {CHO CHAN}  = U+0E08 = 0xe0 0xb8 0x88 in UTF-8
   */
  const char text[] = "<\0d\0 \0a\0=\0'\0\x31\0'\0 \0"
                      "\x04\x0e\x08\x0e=\0'\0\x32\0'\0/\0>\0";
  const XML_Char *expected = XCS("1");
  CharData storage;

  CharData_Init(&storage);
  XML_SetStartElementHandler(g_parser, accumulate_attribute);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_attr_after_solidus) {
  const char *text = "<doc attr1='a' / attr2='b'>";

  expect_failure(text, XML_ERROR_INVALID_TOKEN, "Misplaced / not faulted");
}
END_TEST

static void XMLCALL
accumulate_entity_decl(void *userData, const XML_Char *entityName,
                       int is_parameter_entity, const XML_Char *value,
                       int value_length, const XML_Char *base,
                       const XML_Char *systemId, const XML_Char *publicId,
                       const XML_Char *notationName) {
  CharData *storage = (CharData *)userData;

  UNUSED_P(is_parameter_entity);
  UNUSED_P(base);
  UNUSED_P(systemId);
  UNUSED_P(publicId);
  UNUSED_P(notationName);
  CharData_AppendXMLChars(storage, entityName, -1);
  CharData_AppendXMLChars(storage, XCS("="), 1);
  CharData_AppendXMLChars(storage, value, value_length);
  CharData_AppendXMLChars(storage, XCS("\n"), 1);
}

START_TEST(test_utf16_pe) {
  /* <!DOCTYPE doc [
   * <!ENTITY % {KHO KHWAI}{CHO CHAN} '<!ELEMENT doc (#PCDATA)>'>
   * %{KHO KHWAI}{CHO CHAN};
   * ]>
   * <doc></doc>
   *
   * where {KHO KHWAI} = U+0E04 = 0xe0 0xb8 0x84 in UTF-8
   * and   {CHO CHAN}  = U+0E08 = 0xe0 0xb8 0x88 in UTF-8
   */
  const char text[] = "\0<\0!\0D\0O\0C\0T\0Y\0P\0E\0 \0d\0o\0c\0 \0[\0\n"
                      "\0<\0!\0E\0N\0T\0I\0T\0Y\0 \0%\0 \x0e\x04\x0e\x08\0 "
                      "\0'\0<\0!\0E\0L\0E\0M\0E\0N\0T\0 "
                      "\0d\0o\0c\0 \0(\0#\0P\0C\0D\0A\0T\0A\0)\0>\0'\0>\0\n"
                      "\0%\x0e\x04\x0e\x08\0;\0\n"
                      "\0]\0>\0\n"
                      "\0<\0d\0o\0c\0>\0<\0/\0d\0o\0c\0>";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x0e04\x0e08=<!ELEMENT doc (#PCDATA)>\n");
#else
  const XML_Char *expected
      = XCS("\xe0\xb8\x84\xe0\xb8\x88=<!ELEMENT doc (#PCDATA)>\n");
#endif
  CharData storage;

  CharData_Init(&storage);
  XML_SetUserData(g_parser, &storage);
  XML_SetEntityDeclHandler(g_parser, accumulate_entity_decl);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test that duff attribute description keywords are rejected */
START_TEST(test_bad_attr_desc_keyword) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ATTLIST doc attr CDATA #!IMPLIED>\n"
                     "]>\n"
                     "<doc />";

  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Bad keyword !IMPLIED not faulted");
}
END_TEST

/* Test that an invalid attribute description keyword consisting of
 * UTF-16 characters with their top bytes non-zero are correctly
 * faulted
 */
START_TEST(test_bad_attr_desc_keyword_utf16) {
  /* <!DOCTYPE d [
   * <!ATTLIST d a CDATA #{KHO KHWAI}{CHO CHAN}>
   * ]><d/>
   *
   * where {KHO KHWAI} = U+0E04 = 0xe0 0xb8 0x84 in UTF-8
   * and   {CHO CHAN}  = U+0E08 = 0xe0 0xb8 0x88 in UTF-8
   */
  const char text[]
      = "\0<\0!\0D\0O\0C\0T\0Y\0P\0E\0 \0d\0 \0[\0\n"
        "\0<\0!\0A\0T\0T\0L\0I\0S\0T\0 \0d\0 \0a\0 \0C\0D\0A\0T\0A\0 "
        "\0#\x0e\x04\x0e\x08\0>\0\n"
        "\0]\0>\0<\0d\0/\0>";

  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      != XML_STATUS_ERROR)
    fail("Invalid UTF16 attribute keyword not faulted");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_SYNTAX)
    xml_failure(g_parser);
}
END_TEST

/* Test that invalid syntax in a <!DOCTYPE> is rejected.  Do this
 * using prefix-encoding (see above) to trigger specific code paths
 */
START_TEST(test_bad_doctype) {
  const char *text = "<?xml version='1.0' encoding='prefix-conv'?>\n"
                     "<!DOCTYPE doc [ \x80\x44 ]><doc/>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  expect_failure(text, XML_ERROR_SYNTAX,
                 "Invalid bytes in DOCTYPE not faulted");
}
END_TEST

START_TEST(test_bad_doctype_utf8) {
  const char *text = "<!DOCTYPE \xDB\x25"
                     "doc><doc/>"; // [1101 1011] [<0>010 0101]
  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Invalid UTF-8 in DOCTYPE not faulted");
}
END_TEST

START_TEST(test_bad_doctype_utf16) {
  const char text[] =
      /* <!DOCTYPE doc [ \x06f2 ]><doc/>
       *
       * U+06F2 = EXTENDED ARABIC-INDIC DIGIT TWO, a valid number
       * (name character) but not a valid letter (name start character)
       */
      "\0<\0!\0D\0O\0C\0T\0Y\0P\0E\0 \0d\0o\0c\0 \0[\0 "
      "\x06\xf2"
      "\0 \0]\0>\0<\0d\0o\0c\0/\0>";

  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      != XML_STATUS_ERROR)
    fail("Invalid bytes in DOCTYPE not faulted");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_SYNTAX)
    xml_failure(g_parser);
}
END_TEST

START_TEST(test_bad_doctype_plus) {
  const char *text = "<!DOCTYPE 1+ [ <!ENTITY foo 'bar'> ]>\n"
                     "<1+>&foo;</1+>";

  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "'+' in document name not faulted");
}
END_TEST

START_TEST(test_bad_doctype_star) {
  const char *text = "<!DOCTYPE 1* [ <!ENTITY foo 'bar'> ]>\n"
                     "<1*>&foo;</1*>";

  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "'*' in document name not faulted");
}
END_TEST

START_TEST(test_bad_doctype_query) {
  const char *text = "<!DOCTYPE 1? [ <!ENTITY foo 'bar'> ]>\n"
                     "<1?>&foo;</1?>";

  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "'?' in document name not faulted");
}
END_TEST

START_TEST(test_unknown_encoding_bad_ignore) {
  const char *text = "<?xml version='1.0' encoding='prefix-conv'?>"
                     "<!DOCTYPE doc SYSTEM 'foo'>"
                     "<doc><e>&entity;</e></doc>";
  ExtFaults fault = {"<![IGNORE[<!ELEMENT \xffG (#PCDATA)*>]]>",
                     "Invalid character not faulted", XCS("prefix-conv"),
                     XML_ERROR_INVALID_TOKEN};

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_faulter);
  XML_SetUserData(g_parser, &fault);
  expect_failure(text, XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                 "Bad IGNORE section with unknown encoding not failed");
}
END_TEST

START_TEST(test_entity_in_utf16_be_attr) {
  const char text[] =
      /* <e a='&#228; &#x00E4;'></e> */
      "\0<\0e\0 \0a\0=\0'\0&\0#\0\x32\0\x32\0\x38\0;\0 "
      "\0&\0#\0x\0\x30\0\x30\0E\0\x34\0;\0'\0>\0<\0/\0e\0>";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x00e4 \x00e4");
#else
  const XML_Char *expected = XCS("\xc3\xa4 \xc3\xa4");
#endif
  CharData storage;

  CharData_Init(&storage);
  XML_SetUserData(g_parser, &storage);
  XML_SetStartElementHandler(g_parser, accumulate_attribute);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_entity_in_utf16_le_attr) {
  const char text[] =
      /* <e a='&#228; &#x00E4;'></e> */
      "<\0e\0 \0a\0=\0'\0&\0#\0\x32\0\x32\0\x38\0;\0 \0"
      "&\0#\0x\0\x30\0\x30\0E\0\x34\0;\0'\0>\0<\0/\0e\0>\0";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x00e4 \x00e4");
#else
  const XML_Char *expected = XCS("\xc3\xa4 \xc3\xa4");
#endif
  CharData storage;

  CharData_Init(&storage);
  XML_SetUserData(g_parser, &storage);
  XML_SetStartElementHandler(g_parser, accumulate_attribute);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_entity_public_utf16_be) {
  const char text[] =
      /* <!DOCTYPE d [ */
      "\0<\0!\0D\0O\0C\0T\0Y\0P\0E\0 \0d\0 \0[\0\n"
      /* <!ENTITY % e PUBLIC 'foo' 'bar.ent'> */
      "\0<\0!\0E\0N\0T\0I\0T\0Y\0 \0%\0 \0e\0 \0P\0U\0B\0L\0I\0C\0 "
      "\0'\0f\0o\0o\0'\0 \0'\0b\0a\0r\0.\0e\0n\0t\0'\0>\0\n"
      /* %e; */
      "\0%\0e\0;\0\n"
      /* ]> */
      "\0]\0>\0\n"
      /* <d>&j;</d> */
      "\0<\0d\0>\0&\0j\0;\0<\0/\0d\0>";
  ExtTest2 test_data = {/* <!ENTITY j 'baz'> */
                        "\0<\0!\0E\0N\0T\0I\0T\0Y\0 \0j\0 \0'\0b\0a\0z\0'\0>",
                        34, NULL, NULL, EE_PARSE_NONE};
  const XML_Char *expected = XCS("baz");
  CharData storage;

  CharData_Init(&storage);
  test_data.storage = &storage;
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader2);
  XML_SetUserData(g_parser, &test_data);
  XML_SetCharacterDataHandler(g_parser, ext2_accumulate_characters);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_entity_public_utf16_le) {
  const char text[] =
      /* <!DOCTYPE d [ */
      "<\0!\0D\0O\0C\0T\0Y\0P\0E\0 \0d\0 \0[\0\n\0"
      /* <!ENTITY % e PUBLIC 'foo' 'bar.ent'> */
      "<\0!\0E\0N\0T\0I\0T\0Y\0 \0%\0 \0e\0 \0P\0U\0B\0L\0I\0C\0 \0"
      "'\0f\0o\0o\0'\0 \0'\0b\0a\0r\0.\0e\0n\0t\0'\0>\0\n\0"
      /* %e; */
      "%\0e\0;\0\n\0"
      /* ]> */
      "]\0>\0\n\0"
      /* <d>&j;</d> */
      "<\0d\0>\0&\0j\0;\0<\0/\0d\0>\0";
  ExtTest2 test_data = {/* <!ENTITY j 'baz'> */
                        "<\0!\0E\0N\0T\0I\0T\0Y\0 \0j\0 \0'\0b\0a\0z\0'\0>\0",
                        34, NULL, NULL, EE_PARSE_NONE};
  const XML_Char *expected = XCS("baz");
  CharData storage;

  CharData_Init(&storage);
  test_data.storage = &storage;
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader2);
  XML_SetUserData(g_parser, &test_data);
  XML_SetCharacterDataHandler(g_parser, ext2_accumulate_characters);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/* Test that a doctype with neither an internal nor external subset is
 * faulted
 */
START_TEST(test_short_doctype) {
  const char *text = "<!DOCTYPE doc></doc>";
  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "DOCTYPE without subset not rejected");
}
END_TEST

START_TEST(test_short_doctype_2) {
  const char *text = "<!DOCTYPE doc PUBLIC></doc>";
  expect_failure(text, XML_ERROR_SYNTAX,
                 "DOCTYPE without Public ID not rejected");
}
END_TEST

START_TEST(test_short_doctype_3) {
  const char *text = "<!DOCTYPE doc SYSTEM></doc>";
  expect_failure(text, XML_ERROR_SYNTAX,
                 "DOCTYPE without System ID not rejected");
}
END_TEST

START_TEST(test_long_doctype) {
  const char *text = "<!DOCTYPE doc PUBLIC 'foo' 'bar' 'baz'></doc>";
  expect_failure(text, XML_ERROR_SYNTAX, "DOCTYPE with extra ID not rejected");
}
END_TEST

START_TEST(test_bad_entity) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY foo PUBLIC>\n"
                     "]>\n"
                     "<doc/>";
  expect_failure(text, XML_ERROR_SYNTAX,
                 "ENTITY without Public ID is not rejected");
}
END_TEST

/* Test unquoted value is faulted */
START_TEST(test_bad_entity_2) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY % foo bar>\n"
                     "]>\n"
                     "<doc/>";
  expect_failure(text, XML_ERROR_SYNTAX,
                 "ENTITY without Public ID is not rejected");
}
END_TEST

START_TEST(test_bad_entity_3) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY % foo PUBLIC>\n"
                     "]>\n"
                     "<doc/>";
  expect_failure(text, XML_ERROR_SYNTAX,
                 "Parameter ENTITY without Public ID is not rejected");
}
END_TEST

START_TEST(test_bad_entity_4) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY % foo SYSTEM>\n"
                     "]>\n"
                     "<doc/>";
  expect_failure(text, XML_ERROR_SYNTAX,
                 "Parameter ENTITY without Public ID is not rejected");
}
END_TEST

START_TEST(test_bad_notation) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!NOTATION n SYSTEM>\n"
                     "]>\n"
                     "<doc/>";
  expect_failure(text, XML_ERROR_SYNTAX,
                 "Notation without System ID is not rejected");
}
END_TEST

/* Test for issue #11, wrongly suppressed default handler */
typedef struct default_check {
  const XML_Char *expected;
  const int expectedLen;
  XML_Bool seen;
} DefaultCheck;

static void XMLCALL
checking_default_handler(void *userData, const XML_Char *s, int len) {
  DefaultCheck *data = (DefaultCheck *)userData;
  int i;

  for (i = 0; data[i].expected != NULL; i++) {
    if (data[i].expectedLen == len
        && ! memcmp(data[i].expected, s, len * sizeof(XML_Char))) {
      data[i].seen = XML_TRUE;
      break;
    }
  }
}

START_TEST(test_default_doctype_handler) {
  const char *text = "<!DOCTYPE doc PUBLIC 'pubname' 'test.dtd' [\n"
                     "  <!ENTITY foo 'bar'>\n"
                     "]>\n"
                     "<doc>&foo;</doc>";
  DefaultCheck test_data[] = {{XCS("'pubname'"), 9, XML_FALSE},
                              {XCS("'test.dtd'"), 10, XML_FALSE},
                              {NULL, 0, XML_FALSE}};
  int i;

  XML_SetUserData(g_parser, &test_data);
  XML_SetDefaultHandler(g_parser, checking_default_handler);
  XML_SetEntityDeclHandler(g_parser, dummy_entity_decl_handler);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  for (i = 0; test_data[i].expected != NULL; i++)
    if (! test_data[i].seen)
      fail("Default handler not run for public !DOCTYPE");
}
END_TEST

START_TEST(test_empty_element_abort) {
  const char *text = "<abort/>";

  XML_SetStartElementHandler(g_parser, start_element_suspender);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      != XML_STATUS_ERROR)
    fail("Expected to error on abort");
}
END_TEST

/* Regression test for GH issue #612: unfinished m_declAttributeType
 * allocation in ->m_tempPool can corrupt following allocation.
 */
static int XMLCALL
external_entity_unfinished_attlist(XML_Parser parser, const XML_Char *context,
                                   const XML_Char *base,
                                   const XML_Char *systemId,
                                   const XML_Char *publicId) {
  const char *text = "<!ELEMENT barf ANY>\n"
                     "<!ATTLIST barf my_attr (blah|%blah;a|foo) #REQUIRED>\n"
                     "<!--COMMENT-->\n";
  XML_Parser ext_parser;

  UNUSED_P(base);
  UNUSED_P(publicId);
  if (systemId == NULL)
    return XML_STATUS_OK;

  ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
  if (ext_parser == NULL)
    fail("Could not create external entity parser");

  if (_XML_Parse_SINGLE_BYTES(ext_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(ext_parser);

  XML_ParserFree(ext_parser);
  return XML_STATUS_OK;
}

START_TEST(test_pool_integrity_with_unfinished_attr) {
  const char *text = "<?xml version='1.0' encoding='UTF-8'?>\n"
                     "<!DOCTYPE foo [\n"
                     "<!ELEMENT foo ANY>\n"
                     "<!ENTITY % entp SYSTEM \"external.dtd\">\n"
                     "%entp;\n"
                     "]>\n"
                     "<a></a>\n";
  const XML_Char *expected = XCS("COMMENT");
  CharData storage;

  CharData_Init(&storage);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_unfinished_attlist);
  XML_SetAttlistDeclHandler(g_parser, dummy_attlist_decl_handler);
  XML_SetCommentHandler(g_parser, accumulate_comment);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

typedef struct {
  XML_Parser parser;
  CharData *storage;
} ParserPlusStorage;

static void XMLCALL
accumulate_and_suspend_comment_handler(void *userData, const XML_Char *data) {
  ParserPlusStorage *const parserPlusStorage = (ParserPlusStorage *)userData;
  accumulate_comment(parserPlusStorage->storage, data);
  XML_StopParser(parserPlusStorage->parser, XML_TRUE);
}

START_TEST(test_nested_entity_suspend) {
  const char *const text = "<!DOCTYPE a [\n"
                           "  <!ENTITY e1 '<!--e1-->'>\n"
                           "  <!ENTITY e2 '<!--e2 head-->&e1;<!--e2 tail-->'>\n"
                           "  <!ENTITY e3 '<!--e3 head-->&e2;<!--e3 tail-->'>\n"
                           "]>\n"
                           "<a><!--start-->&e3;<!--end--></a>";
  const XML_Char *const expected = XCS("start") XCS("e3 head") XCS("e2 head")
      XCS("e1") XCS("e2 tail") XCS("e3 tail") XCS("end");
  CharData storage;
  XML_Parser parser = XML_ParserCreate(NULL);
  ParserPlusStorage parserPlusStorage = {parser, &storage};

  CharData_Init(&storage);
  XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetCommentHandler(parser, accumulate_and_suspend_comment_handler);
  XML_SetUserData(parser, &parserPlusStorage);

  enum XML_Status status = XML_Parse(parser, text, (int)strlen(text), XML_TRUE);
  while (status == XML_STATUS_SUSPENDED) {
    status = XML_ResumeParser(parser);
  }
  if (status != XML_STATUS_OK)
    xml_failure(parser);

  CharData_CheckXMLChars(&storage, expected);
  XML_ParserFree(parser);
}
END_TEST

/*
 * Namespaces tests.
 */

static void
namespace_setup(void) {
  g_parser = XML_ParserCreateNS(NULL, XCS(' '));
  if (g_parser == NULL)
    fail("Parser not created.");
}

static void
namespace_teardown(void) {
  basic_teardown();
}

/* Check that an element name and attribute name match the expected values.
   The expected values are passed as an array reference of string pointers
   provided as the userData argument; the first is the expected
   element name, and the second is the expected attribute name.
*/
static int triplet_start_flag = XML_FALSE;
static int triplet_end_flag = XML_FALSE;

static void XMLCALL
triplet_start_checker(void *userData, const XML_Char *name,
                      const XML_Char **atts) {
  XML_Char **elemstr = (XML_Char **)userData;
  char buffer[1024];
  if (xcstrcmp(elemstr[0], name) != 0) {
    snprintf(buffer, sizeof(buffer),
             "unexpected start string: '%" XML_FMT_STR "'", name);
    fail(buffer);
  }
  if (xcstrcmp(elemstr[1], atts[0]) != 0) {
    snprintf(buffer, sizeof(buffer),
             "unexpected attribute string: '%" XML_FMT_STR "'", atts[0]);
    fail(buffer);
  }
  triplet_start_flag = XML_TRUE;
}

/* Check that the element name passed to the end-element handler matches
   the expected value.  The expected value is passed as the first element
   in an array of strings passed as the userData argument.
*/
static void XMLCALL
triplet_end_checker(void *userData, const XML_Char *name) {
  XML_Char **elemstr = (XML_Char **)userData;
  if (xcstrcmp(elemstr[0], name) != 0) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer),
             "unexpected end string: '%" XML_FMT_STR "'", name);
    fail(buffer);
  }
  triplet_end_flag = XML_TRUE;
}

START_TEST(test_return_ns_triplet) {
  const char *text = "<foo:e xmlns:foo='http://example.org/' bar:a='12'\n"
                     "       xmlns:bar='http://example.org/'>";
  const char *epilog = "</foo:e>";
  const XML_Char *elemstr[]
      = {XCS("http://example.org/ e foo"), XCS("http://example.org/ a bar")};
  XML_SetReturnNSTriplet(g_parser, XML_TRUE);
  XML_SetUserData(g_parser, (void *)elemstr);
  XML_SetElementHandler(g_parser, triplet_start_checker, triplet_end_checker);
  XML_SetNamespaceDeclHandler(g_parser, dummy_start_namespace_decl_handler,
                              dummy_end_namespace_decl_handler);
  triplet_start_flag = XML_FALSE;
  triplet_end_flag = XML_FALSE;
  init_dummy_handlers();
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_FALSE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  if (! triplet_start_flag)
    fail("triplet_start_checker not invoked");
  /* Check that unsetting "return triplets" fails while still parsing */
  XML_SetReturnNSTriplet(g_parser, XML_FALSE);
  if (_XML_Parse_SINGLE_BYTES(g_parser, epilog, (int)strlen(epilog), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  if (! triplet_end_flag)
    fail("triplet_end_checker not invoked");
  if (get_dummy_handler_flags()
      != (DUMMY_START_NS_DECL_HANDLER_FLAG | DUMMY_END_NS_DECL_HANDLER_FLAG))
    fail("Namespace handlers not called");
}
END_TEST

static void XMLCALL
overwrite_start_checker(void *userData, const XML_Char *name,
                        const XML_Char **atts) {
  CharData *storage = (CharData *)userData;
  CharData_AppendXMLChars(storage, XCS("start "), 6);
  CharData_AppendXMLChars(storage, name, -1);
  while (*atts != NULL) {
    CharData_AppendXMLChars(storage, XCS("\nattribute "), 11);
    CharData_AppendXMLChars(storage, *atts, -1);
    atts += 2;
  }
  CharData_AppendXMLChars(storage, XCS("\n"), 1);
}

static void XMLCALL
overwrite_end_checker(void *userData, const XML_Char *name) {
  CharData *storage = (CharData *)userData;
  CharData_AppendXMLChars(storage, XCS("end "), 4);
  CharData_AppendXMLChars(storage, name, -1);
  CharData_AppendXMLChars(storage, XCS("\n"), 1);
}

static void
run_ns_tagname_overwrite_test(const char *text, const XML_Char *result) {
  CharData storage;
  CharData_Init(&storage);
  XML_SetUserData(g_parser, &storage);
  XML_SetElementHandler(g_parser, overwrite_start_checker,
                        overwrite_end_checker);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, result);
}

/* Regression test for SF bug #566334. */
START_TEST(test_ns_tagname_overwrite) {
  const char *text = "<n:e xmlns:n='http://example.org/'>\n"
                     "  <n:f n:attr='foo'/>\n"
                     "  <n:g n:attr2='bar'/>\n"
                     "</n:e>";
  const XML_Char *result = XCS("start http://example.org/ e\n")
      XCS("start http://example.org/ f\n")
          XCS("attribute http://example.org/ attr\n")
              XCS("end http://example.org/ f\n")
                  XCS("start http://example.org/ g\n")
                      XCS("attribute http://example.org/ attr2\n")
                          XCS("end http://example.org/ g\n")
                              XCS("end http://example.org/ e\n");
  run_ns_tagname_overwrite_test(text, result);
}
END_TEST

/* Regression test for SF bug #566334. */
START_TEST(test_ns_tagname_overwrite_triplet) {
  const char *text = "<n:e xmlns:n='http://example.org/'>\n"
                     "  <n:f n:attr='foo'/>\n"
                     "  <n:g n:attr2='bar'/>\n"
                     "</n:e>";
  const XML_Char *result = XCS("start http://example.org/ e n\n")
      XCS("start http://example.org/ f n\n")
          XCS("attribute http://example.org/ attr n\n")
              XCS("end http://example.org/ f n\n")
                  XCS("start http://example.org/ g n\n")
                      XCS("attribute http://example.org/ attr2 n\n")
                          XCS("end http://example.org/ g n\n")
                              XCS("end http://example.org/ e n\n");
  XML_SetReturnNSTriplet(g_parser, XML_TRUE);
  run_ns_tagname_overwrite_test(text, result);
}
END_TEST

/* Regression test for SF bug #620343. */
static void XMLCALL
start_element_fail(void *userData, const XML_Char *name,
                   const XML_Char **atts) {
  UNUSED_P(userData);
  UNUSED_P(name);
  UNUSED_P(atts);

  /* We should never get here. */
  fail("should never reach start_element_fail()");
}

static void XMLCALL
start_ns_clearing_start_element(void *userData, const XML_Char *prefix,
                                const XML_Char *uri) {
  UNUSED_P(prefix);
  UNUSED_P(uri);
  XML_SetStartElementHandler((XML_Parser)userData, NULL);
}

START_TEST(test_start_ns_clears_start_element) {
  /* This needs to use separate start/end tags; using the empty tag
     syntax doesn't cause the problematic path through Expat to be
     taken.
  */
  const char *text = "<e xmlns='http://example.org/'></e>";

  XML_SetStartElementHandler(g_parser, start_element_fail);
  XML_SetStartNamespaceDeclHandler(g_parser, start_ns_clearing_start_element);
  XML_SetEndNamespaceDeclHandler(g_parser, dummy_end_namespace_decl_handler);
  XML_UseParserAsHandlerArg(g_parser);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Regression test for SF bug #616863. */
static int XMLCALL
external_entity_handler(XML_Parser parser, const XML_Char *context,
                        const XML_Char *base, const XML_Char *systemId,
                        const XML_Char *publicId) {
  intptr_t callno = 1 + (intptr_t)XML_GetUserData(parser);
  const char *text;
  XML_Parser p2;

  UNUSED_P(base);
  UNUSED_P(systemId);
  UNUSED_P(publicId);
  if (callno == 1)
    text = ("<!ELEMENT doc (e+)>\n"
            "<!ATTLIST doc xmlns CDATA #IMPLIED>\n"
            "<!ELEMENT e EMPTY>\n");
  else
    text = ("<?xml version='1.0' encoding='us-ascii'?>"
            "<e/>");

  XML_SetUserData(parser, (void *)callno);
  p2 = XML_ExternalEntityParserCreate(parser, context, NULL);
  if (_XML_Parse_SINGLE_BYTES(p2, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR) {
    xml_failure(p2);
    return XML_STATUS_ERROR;
  }
  XML_ParserFree(p2);
  return XML_STATUS_OK;
}

START_TEST(test_default_ns_from_ext_subset_and_ext_ge) {
  const char *text = "<?xml version='1.0'?>\n"
                     "<!DOCTYPE doc SYSTEM 'http://example.org/doc.dtd' [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/entity.ent'>\n"
                     "]>\n"
                     "<doc xmlns='http://example.org/ns1'>\n"
                     "&en;\n"
                     "</doc>";

  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_handler);
  /* We actually need to set this handler to tickle this bug. */
  XML_SetStartElementHandler(g_parser, dummy_start_element);
  XML_SetUserData(g_parser, NULL);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Regression test #1 for SF bug #673791. */
START_TEST(test_ns_prefix_with_empty_uri_1) {
  const char *text = "<doc xmlns:prefix='http://example.org/'>\n"
                     "  <e xmlns:prefix=''/>\n"
                     "</doc>";

  expect_failure(text, XML_ERROR_UNDECLARING_PREFIX,
                 "Did not report re-setting namespace"
                 " URI with prefix to ''.");
}
END_TEST

/* Regression test #2 for SF bug #673791. */
START_TEST(test_ns_prefix_with_empty_uri_2) {
  const char *text = "<?xml version='1.0'?>\n"
                     "<docelem xmlns:pre=''/>";

  expect_failure(text, XML_ERROR_UNDECLARING_PREFIX,
                 "Did not report setting namespace URI with prefix to ''.");
}
END_TEST

/* Regression test #3 for SF bug #673791. */
START_TEST(test_ns_prefix_with_empty_uri_3) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ELEMENT doc EMPTY>\n"
                     "  <!ATTLIST doc\n"
                     "    xmlns:prefix CDATA ''>\n"
                     "]>\n"
                     "<doc/>";

  expect_failure(text, XML_ERROR_UNDECLARING_PREFIX,
                 "Didn't report attr default setting NS w/ prefix to ''.");
}
END_TEST

/* Regression test #4 for SF bug #673791. */
START_TEST(test_ns_prefix_with_empty_uri_4) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ELEMENT prefix:doc EMPTY>\n"
                     "  <!ATTLIST prefix:doc\n"
                     "    xmlns:prefix CDATA 'http://example.org/'>\n"
                     "]>\n"
                     "<prefix:doc/>";
  /* Packaged info expected by the end element handler;
     the weird structuring lets us re-use the triplet_end_checker()
     function also used for another test. */
  const XML_Char *elemstr[] = {XCS("http://example.org/ doc prefix")};
  XML_SetReturnNSTriplet(g_parser, XML_TRUE);
  XML_SetUserData(g_parser, (void *)elemstr);
  XML_SetEndElementHandler(g_parser, triplet_end_checker);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Test with non-xmlns prefix */
START_TEST(test_ns_unbound_prefix) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ELEMENT prefix:doc EMPTY>\n"
                     "  <!ATTLIST prefix:doc\n"
                     "    notxmlns:prefix CDATA 'http://example.org/'>\n"
                     "]>\n"
                     "<prefix:doc/>";

  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      != XML_STATUS_ERROR)
    fail("Unbound prefix incorrectly passed");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_UNBOUND_PREFIX)
    xml_failure(g_parser);
}
END_TEST

START_TEST(test_ns_default_with_empty_uri) {
  const char *text = "<doc xmlns='http://example.org/'>\n"
                     "  <e xmlns=''/>\n"
                     "</doc>";
  /* Add some handlers to exercise extra code paths */
  XML_SetStartNamespaceDeclHandler(g_parser,
                                   dummy_start_namespace_decl_handler);
  XML_SetEndNamespaceDeclHandler(g_parser, dummy_end_namespace_decl_handler);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Regression test for SF bug #692964: two prefixes for one namespace. */
START_TEST(test_ns_duplicate_attrs_diff_prefixes) {
  const char *text = "<doc xmlns:a='http://example.org/a'\n"
                     "     xmlns:b='http://example.org/a'\n"
                     "     a:a='v' b:a='v' />";
  expect_failure(text, XML_ERROR_DUPLICATE_ATTRIBUTE,
                 "did not report multiple attributes with same URI+name");
}
END_TEST

START_TEST(test_ns_duplicate_hashes) {
  /* The hash of an attribute is calculated as the hash of its URI
   * concatenated with a space followed by its name (after the
   * colon).  We wish to generate attributes with the same hash
   * value modulo the attribute table size so that we can check that
   * the attribute hash table works correctly.  The attribute hash
   * table size will be the smallest power of two greater than the
   * number of attributes, but at least eight.  There is
   * unfortunately no programmatic way of getting the hash or the
   * table size at user level, but the test code coverage percentage
   * will drop if the hashes cease to point to the same row.
   *
   * The cunning plan is to have few enough attributes to have a
   * reliable table size of 8, and have the single letter attribute
   * names be 8 characters apart, producing a hash which will be the
   * same modulo 8.
   */
  const char *text = "<doc xmlns:a='http://example.org/a'\n"
                     "     a:a='v' a:i='w' />";
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Regression test for SF bug #695401: unbound prefix. */
START_TEST(test_ns_unbound_prefix_on_attribute) {
  const char *text = "<doc a:attr=''/>";
  expect_failure(text, XML_ERROR_UNBOUND_PREFIX,
                 "did not report unbound prefix on attribute");
}
END_TEST

/* Regression test for SF bug #695401: unbound prefix. */
START_TEST(test_ns_unbound_prefix_on_element) {
  const char *text = "<a:doc/>";
  expect_failure(text, XML_ERROR_UNBOUND_PREFIX,
                 "did not report unbound prefix on element");
}
END_TEST

/* Test that the parsing status is correctly reset by XML_ParserReset().
 * We usE test_return_ns_triplet() for our example parse to improve
 * coverage of tidying up code executed.
 */
START_TEST(test_ns_parser_reset) {
  XML_ParsingStatus status;

  XML_GetParsingStatus(g_parser, &status);
  if (status.parsing != XML_INITIALIZED)
    fail("parsing status doesn't start INITIALIZED");
  test_return_ns_triplet();
  XML_GetParsingStatus(g_parser, &status);
  if (status.parsing != XML_FINISHED)
    fail("parsing status doesn't end FINISHED");
  XML_ParserReset(g_parser, NULL);
  XML_GetParsingStatus(g_parser, &status);
  if (status.parsing != XML_INITIALIZED)
    fail("parsing status doesn't reset to INITIALIZED");
}
END_TEST

/* Test that long element names with namespaces are handled correctly */
START_TEST(test_ns_long_element) {
  const char *text
      = "<foo:thisisalongenoughelementnametotriggerareallocation\n"
        " xmlns:foo='http://example.org/' bar:a='12'\n"
        " xmlns:bar='http://example.org/'>"
        "</foo:thisisalongenoughelementnametotriggerareallocation>";
  const XML_Char *elemstr[]
      = {XCS("http://example.org/")
             XCS(" thisisalongenoughelementnametotriggerareallocation foo"),
         XCS("http://example.org/ a bar")};

  XML_SetReturnNSTriplet(g_parser, XML_TRUE);
  XML_SetUserData(g_parser, (void *)elemstr);
  XML_SetElementHandler(g_parser, triplet_start_checker, triplet_end_checker);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Test mixed population of prefixed and unprefixed attributes */
START_TEST(test_ns_mixed_prefix_atts) {
  const char *text = "<e a='12' bar:b='13'\n"
                     " xmlns:bar='http://example.org/'>"
                     "</e>";

  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Test having a long namespaced element name inside a short one.
 * This exercises some internal buffer reallocation that is shared
 * across elements with the same namespace URI.
 */
START_TEST(test_ns_extend_uri_buffer) {
  const char *text = "<foo:e xmlns:foo='http://example.org/'>"
                     " <foo:thisisalongenoughnametotriggerallocationaction"
                     "   foo:a='12' />"
                     "</foo:e>";
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Test that xmlns is correctly rejected as an attribute in the xmlns
 * namespace, but not in other namespaces
 */
START_TEST(test_ns_reserved_attributes) {
  const char *text1
      = "<foo:e xmlns:foo='http://example.org/' xmlns:xmlns='12' />";
  const char *text2
      = "<foo:e xmlns:foo='http://example.org/' foo:xmlns='12' />";
  expect_failure(text1, XML_ERROR_RESERVED_PREFIX_XMLNS,
                 "xmlns not rejected as an attribute");
  XML_ParserReset(g_parser, NULL);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text2, (int)strlen(text2), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Test more reserved attributes */
START_TEST(test_ns_reserved_attributes_2) {
  const char *text1 = "<foo:e xmlns:foo='http://example.org/'"
                      "  xmlns:xml='http://example.org/' />";
  const char *text2
      = "<foo:e xmlns:foo='http://www.w3.org/XML/1998/namespace' />";
  const char *text3 = "<foo:e xmlns:foo='http://www.w3.org/2000/xmlns/' />";

  expect_failure(text1, XML_ERROR_RESERVED_PREFIX_XML,
                 "xml not rejected as an attribute");
  XML_ParserReset(g_parser, NULL);
  expect_failure(text2, XML_ERROR_RESERVED_NAMESPACE_URI,
                 "Use of w3.org URL not faulted");
  XML_ParserReset(g_parser, NULL);
  expect_failure(text3, XML_ERROR_RESERVED_NAMESPACE_URI,
                 "Use of w3.org xmlns URL not faulted");
}
END_TEST

/* Test string pool handling of namespace names of 2048 characters */
/* Exercises a particular string pool growth path */
START_TEST(test_ns_extremely_long_prefix) {
  /* C99 compilers are only required to support 4095-character
   * strings, so the following needs to be split in two to be safe
   * for all compilers.
   */
  const char *text1
      = "<doc "
        /* 64 character on each line */
        /* ...gives a total length of 2048 */
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        ":a='12'";
  const char *text2
      = " xmlns:"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "='foo'\n>"
        "</doc>";

  if (_XML_Parse_SINGLE_BYTES(g_parser, text1, (int)strlen(text1), XML_FALSE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text2, (int)strlen(text2), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Test unknown encoding handlers in namespace setup */
START_TEST(test_ns_unknown_encoding_success) {
  const char *text = "<?xml version='1.0' encoding='prefix-conv'?>\n"
                     "<foo:e xmlns:foo='http://example.org/'>Hi</foo:e>";

  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  run_character_check(text, XCS("Hi"));
}
END_TEST

/* Test that too many colons are rejected */
START_TEST(test_ns_double_colon) {
  const char *text = "<foo:e xmlns:foo='http://example.org/' foo:a:b='bar' />";
  const enum XML_Status status
      = _XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE);
#ifdef XML_NS
  if ((status == XML_STATUS_OK)
      || (XML_GetErrorCode(g_parser) != XML_ERROR_INVALID_TOKEN)) {
    fail("Double colon in attribute name not faulted"
         " (despite active namespace support)");
  }
#else
  if (status != XML_STATUS_OK) {
    fail("Double colon in attribute name faulted"
         " (despite inactive namespace support");
  }
#endif
}
END_TEST

START_TEST(test_ns_double_colon_element) {
  const char *text = "<foo:bar:e xmlns:foo='http://example.org/' />";
  const enum XML_Status status
      = _XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE);
#ifdef XML_NS
  if ((status == XML_STATUS_OK)
      || (XML_GetErrorCode(g_parser) != XML_ERROR_INVALID_TOKEN)) {
    fail("Double colon in element name not faulted"
         " (despite active namespace support)");
  }
#else
  if (status != XML_STATUS_OK) {
    fail("Double colon in element name faulted"
         " (despite inactive namespace support");
  }
#endif
}
END_TEST

/* Test that non-name characters after a colon are rejected */
START_TEST(test_ns_bad_attr_leafname) {
  const char *text = "<foo:e xmlns:foo='http://example.org/' foo:?ar='baz' />";

  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Invalid character in leafname not faulted");
}
END_TEST

START_TEST(test_ns_bad_element_leafname) {
  const char *text = "<foo:?oc xmlns:foo='http://example.org/' />";

  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Invalid character in element leafname not faulted");
}
END_TEST

/* Test high-byte-set UTF-16 characters are valid in a leafname */
START_TEST(test_ns_utf16_leafname) {
  const char text[] =
      /* <n:e xmlns:n='URI' n:{KHO KHWAI}='a' />
       * where {KHO KHWAI} = U+0E04 = 0xe0 0xb8 0x84 in UTF-8
       */
      "<\0n\0:\0e\0 \0x\0m\0l\0n\0s\0:\0n\0=\0'\0U\0R\0I\0'\0 \0"
      "n\0:\0\x04\x0e=\0'\0a\0'\0 \0/\0>\0";
  const XML_Char *expected = XCS("a");
  CharData storage;

  CharData_Init(&storage);
  XML_SetStartElementHandler(g_parser, accumulate_attribute);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_ns_utf16_element_leafname) {
  const char text[] =
      /* <n:{KHO KHWAI} xmlns:n='URI'/>
       * where {KHO KHWAI} = U+0E04 = 0xe0 0xb8 0x84 in UTF-8
       */
      "\0<\0n\0:\x0e\x04\0 \0x\0m\0l\0n\0s\0:\0n\0=\0'\0U\0R\0I\0'\0/\0>";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("URI \x0e04");
#else
  const XML_Char *expected = XCS("URI \xe0\xb8\x84");
#endif
  CharData storage;

  CharData_Init(&storage);
  XML_SetStartElementHandler(g_parser, start_element_event_handler);
  XML_SetUserData(g_parser, &storage);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_ns_utf16_doctype) {
  const char text[] =
      /* <!DOCTYPE foo:{KHO KHWAI} [ <!ENTITY bar 'baz'> ]>\n
       * where {KHO KHWAI} = U+0E04 = 0xe0 0xb8 0x84 in UTF-8
       */
      "\0<\0!\0D\0O\0C\0T\0Y\0P\0E\0 \0f\0o\0o\0:\x0e\x04\0 "
      "\0[\0 \0<\0!\0E\0N\0T\0I\0T\0Y\0 \0b\0a\0r\0 \0'\0b\0a\0z\0'\0>\0 "
      "\0]\0>\0\n"
      /* <foo:{KHO KHWAI} xmlns:foo='URI'>&bar;</foo:{KHO KHWAI}> */
      "\0<\0f\0o\0o\0:\x0e\x04\0 "
      "\0x\0m\0l\0n\0s\0:\0f\0o\0o\0=\0'\0U\0R\0I\0'\0>"
      "\0&\0b\0a\0r\0;"
      "\0<\0/\0f\0o\0o\0:\x0e\x04\0>";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("URI \x0e04");
#else
  const XML_Char *expected = XCS("URI \xe0\xb8\x84");
#endif
  CharData storage;

  CharData_Init(&storage);
  XML_SetUserData(g_parser, &storage);
  XML_SetStartElementHandler(g_parser, start_element_event_handler);
  XML_SetUnknownEncodingHandler(g_parser, MiscEncodingHandler, NULL);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_ns_invalid_doctype) {
  const char *text = "<!DOCTYPE foo:!bad [ <!ENTITY bar 'baz' ]>\n"
                     "<foo:!bad>&bar;</foo:!bad>";

  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "Invalid character in document local name not faulted");
}
END_TEST

START_TEST(test_ns_double_colon_doctype) {
  const char *text = "<!DOCTYPE foo:a:doc [ <!ENTITY bar 'baz' ]>\n"
                     "<foo:a:doc>&bar;</foo:a:doc>";

  expect_failure(text, XML_ERROR_SYNTAX,
                 "Double colon in document name not faulted");
}
END_TEST

START_TEST(test_ns_separator_in_uri) {
  struct test_case {
    enum XML_Status expectedStatus;
    const char *doc;
    XML_Char namesep;
  };
  struct test_case cases[] = {
      {XML_STATUS_OK, "<doc xmlns='one_two' />", XCS('\n')},
      {XML_STATUS_ERROR, "<doc xmlns='one&#x0A;two' />", XCS('\n')},
      {XML_STATUS_OK, "<doc xmlns='one:two' />", XCS(':')},
  };

  size_t i = 0;
  size_t failCount = 0;
  for (; i < sizeof(cases) / sizeof(cases[0]); i++) {
    XML_Parser parser = XML_ParserCreateNS(NULL, cases[i].namesep);
    XML_SetElementHandler(parser, dummy_start_element, dummy_end_element);
    if (XML_Parse(parser, cases[i].doc, (int)strlen(cases[i].doc),
                  /*isFinal*/ XML_TRUE)
        != cases[i].expectedStatus) {
      failCount++;
    }
    XML_ParserFree(parser);
  }

  if (failCount) {
    fail("Namespace separator handling is broken");
  }
}
END_TEST

/* Control variable; the number of times duff_allocator() will successfully
 * allocate */
#define ALLOC_ALWAYS_SUCCEED (-1)
#define REALLOC_ALWAYS_SUCCEED (-1)

static intptr_t allocation_count = ALLOC_ALWAYS_SUCCEED;
static intptr_t reallocation_count = REALLOC_ALWAYS_SUCCEED;

/* Crocked allocator for allocation failure tests */
static void *
duff_allocator(size_t size) {
  if (allocation_count == 0)
    return NULL;
  if (allocation_count != ALLOC_ALWAYS_SUCCEED)
    allocation_count--;
  return malloc(size);
}

/* Crocked reallocator for allocation failure tests */
static void *
duff_reallocator(void *ptr, size_t size) {
  if (reallocation_count == 0)
    return NULL;
  if (reallocation_count != REALLOC_ALWAYS_SUCCEED)
    reallocation_count--;
  return realloc(ptr, size);
}

/* Test that a failure to allocate the parser structure fails gracefully */
START_TEST(test_misc_alloc_create_parser) {
  XML_Memory_Handling_Suite memsuite = {duff_allocator, realloc, free};
  unsigned int i;
  const unsigned int max_alloc_count = 10;

  /* Something this simple shouldn't need more than 10 allocations */
  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    g_parser = XML_ParserCreate_MM(NULL, &memsuite, NULL);
    if (g_parser != NULL)
      break;
  }
  if (i == 0)
    fail("Parser unexpectedly ignored failing allocator");
  else if (i == max_alloc_count)
    fail("Parser not created with max allocation count");
}
END_TEST

/* Test memory allocation failures for a parser with an encoding */
START_TEST(test_misc_alloc_create_parser_with_encoding) {
  XML_Memory_Handling_Suite memsuite = {duff_allocator, realloc, free};
  unsigned int i;
  const unsigned int max_alloc_count = 10;

  /* Try several levels of allocation */
  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    g_parser = XML_ParserCreate_MM(XCS("us-ascii"), &memsuite, NULL);
    if (g_parser != NULL)
      break;
  }
  if (i == 0)
    fail("Parser ignored failing allocator");
  else if (i == max_alloc_count)
    fail("Parser not created with max allocation count");
}
END_TEST

/* Test that freeing a NULL parser doesn't cause an explosion.
 * (Not actually tested anywhere else)
 */
START_TEST(test_misc_null_parser) {
  XML_ParserFree(NULL);
}
END_TEST

/* Test that XML_ErrorString rejects out-of-range codes */
START_TEST(test_misc_error_string) {
  if (XML_ErrorString((enum XML_Error) - 1) != NULL)
    fail("Negative error code not rejected");
  if (XML_ErrorString((enum XML_Error)100) != NULL)
    fail("Large error code not rejected");
}
END_TEST

/* Test the version information is consistent */

/* Since we are working in XML_LChars (potentially 16-bits), we
 * can't use the standard C library functions for character
 * manipulation and have to roll our own.
 */
static int
parse_version(const XML_LChar *version_text,
              XML_Expat_Version *version_struct) {
  if (! version_text)
    return XML_FALSE;

  while (*version_text != 0x00) {
    if (*version_text >= ASCII_0 && *version_text <= ASCII_9)
      break;
    version_text++;
  }
  if (*version_text == 0x00)
    return XML_FALSE;

  /* version_struct->major = strtoul(version_text, 10, &version_text) */
  version_struct->major = 0;
  while (*version_text >= ASCII_0 && *version_text <= ASCII_9) {
    version_struct->major
        = 10 * version_struct->major + (*version_text++ - ASCII_0);
  }
  if (*version_text++ != ASCII_PERIOD)
    return XML_FALSE;

  /* Now for the minor version number */
  version_struct->minor = 0;
  while (*version_text >= ASCII_0 && *version_text <= ASCII_9) {
    version_struct->minor
        = 10 * version_struct->minor + (*version_text++ - ASCII_0);
  }
  if (*version_text++ != ASCII_PERIOD)
    return XML_FALSE;

  /* Finally the micro version number */
  version_struct->micro = 0;
  while (*version_text >= ASCII_0 && *version_text <= ASCII_9) {
    version_struct->micro
        = 10 * version_struct->micro + (*version_text++ - ASCII_0);
  }
  if (*version_text != 0x00)
    return XML_FALSE;
  return XML_TRUE;
}

static int
versions_equal(const XML_Expat_Version *first,
               const XML_Expat_Version *second) {
  return (first->major == second->major && first->minor == second->minor
          && first->micro == second->micro);
}

START_TEST(test_misc_version) {
  XML_Expat_Version read_version = XML_ExpatVersionInfo();
  /* Silence compiler warning with the following assignment */
  XML_Expat_Version parsed_version = {0, 0, 0};
  const XML_LChar *version_text = XML_ExpatVersion();

  if (version_text == NULL)
    fail("Could not obtain version text");
  assert(version_text != NULL);
  if (! parse_version(version_text, &parsed_version))
    fail("Unable to parse version text");
  if (! versions_equal(&read_version, &parsed_version))
    fail("Version mismatch");

#if ! defined(XML_UNICODE) || defined(XML_UNICODE_WCHAR_T)
  if (xcstrcmp(version_text, XCS("expat_2.5.0"))) /* needs bump on releases */
    fail("XML_*_VERSION in expat.h out of sync?\n");
#else
  /* If we have XML_UNICODE defined but not XML_UNICODE_WCHAR_T
   * then XML_LChar is defined as char, for some reason.
   */
  if (strcmp(version_text, "expat_2.2.5")) /* needs bump on releases */
    fail("XML_*_VERSION in expat.h out of sync?\n");
#endif /* ! defined(XML_UNICODE) || defined(XML_UNICODE_WCHAR_T) */
}
END_TEST

/* Test feature information */
START_TEST(test_misc_features) {
  const XML_Feature *features = XML_GetFeatureList();

  /* Prevent problems with double-freeing parsers */
  g_parser = NULL;
  if (features == NULL) {
    fail("Failed to get feature information");
  } else {
    /* Loop through the features checking what we can */
    while (features->feature != XML_FEATURE_END) {
      switch (features->feature) {
      case XML_FEATURE_SIZEOF_XML_CHAR:
        if (features->value != sizeof(XML_Char))
          fail("Incorrect size of XML_Char");
        break;
      case XML_FEATURE_SIZEOF_XML_LCHAR:
        if (features->value != sizeof(XML_LChar))
          fail("Incorrect size of XML_LChar");
        break;
      default:
        break;
      }
      features++;
    }
  }
}
END_TEST

/* Regression test for GitHub Issue #17: memory leak parsing attribute
 * values with mixed bound and unbound namespaces.
 */
START_TEST(test_misc_attribute_leak) {
  const char *text = "<D xmlns:L=\"D\" l:a='' L:a=''/>";
  XML_Memory_Handling_Suite memsuite
      = {tracking_malloc, tracking_realloc, tracking_free};

  g_parser = XML_ParserCreate_MM(XCS("UTF-8"), &memsuite, XCS("\n"));
  expect_failure(text, XML_ERROR_UNBOUND_PREFIX, "Unbound prefixes not found");
  XML_ParserFree(g_parser);
  /* Prevent the teardown trying to double free */
  g_parser = NULL;

  if (! tracking_report())
    fail("Memory leak found");
}
END_TEST

/* Test parser created for UTF-16LE is successful */
START_TEST(test_misc_utf16le) {
  const char text[] =
      /* <?xml version='1.0'?><q>Hi</q> */
      "<\0?\0x\0m\0l\0 \0"
      "v\0e\0r\0s\0i\0o\0n\0=\0'\0\x31\0.\0\x30\0'\0?\0>\0"
      "<\0q\0>\0H\0i\0<\0/\0q\0>\0";
  const XML_Char *expected = XCS("Hi");
  CharData storage;

  g_parser = XML_ParserCreate(XCS("UTF-16LE"));
  if (g_parser == NULL)
    fail("Parser not created");

  CharData_Init(&storage);
  XML_SetUserData(g_parser, &storage);
  XML_SetCharacterDataHandler(g_parser, accumulate_characters);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

typedef struct {
  XML_Parser parser;
  int deep;
} DataIssue240;

static void
start_element_issue_240(void *userData, const XML_Char *name,
                        const XML_Char **atts) {
  DataIssue240 *mydata = (DataIssue240 *)userData;
  UNUSED_P(name);
  UNUSED_P(atts);
  mydata->deep++;
}

static void
end_element_issue_240(void *userData, const XML_Char *name) {
  DataIssue240 *mydata = (DataIssue240 *)userData;

  UNUSED_P(name);
  mydata->deep--;
  if (mydata->deep == 0) {
    XML_StopParser(mydata->parser, 0);
  }
}

START_TEST(test_misc_stop_during_end_handler_issue_240_1) {
  XML_Parser parser;
  DataIssue240 *mydata;
  enum XML_Status result;
  const char *const doc1 = "<doc><e1/><e><foo/></e></doc>";

  parser = XML_ParserCreate(NULL);
  XML_SetElementHandler(parser, start_element_issue_240, end_element_issue_240);
  mydata = (DataIssue240 *)malloc(sizeof(DataIssue240));
  mydata->parser = parser;
  mydata->deep = 0;
  XML_SetUserData(parser, mydata);

  result = XML_Parse(parser, doc1, (int)strlen(doc1), 1);
  XML_ParserFree(parser);
  free(mydata);
  if (result != XML_STATUS_ERROR)
    fail("Stopping the parser did not work as expected");
}
END_TEST

START_TEST(test_misc_stop_during_end_handler_issue_240_2) {
  XML_Parser parser;
  DataIssue240 *mydata;
  enum XML_Status result;
  const char *const doc2 = "<doc><elem/></doc>";

  parser = XML_ParserCreate(NULL);
  XML_SetElementHandler(parser, start_element_issue_240, end_element_issue_240);
  mydata = (DataIssue240 *)malloc(sizeof(DataIssue240));
  mydata->parser = parser;
  mydata->deep = 0;
  XML_SetUserData(parser, mydata);

  result = XML_Parse(parser, doc2, (int)strlen(doc2), 1);
  XML_ParserFree(parser);
  free(mydata);
  if (result != XML_STATUS_ERROR)
    fail("Stopping the parser did not work as expected");
}
END_TEST

START_TEST(test_misc_deny_internal_entity_closing_doctype_issue_317) {
  const char *const inputOne = "<!DOCTYPE d [\n"
                               "<!ENTITY % e ']><d/>'>\n"
                               "\n"
                               "%e;";
  const char *const inputTwo = "<!DOCTYPE d [\n"
                               "<!ENTITY % e1 ']><d/>'><!ENTITY % e2 '&e1;'>\n"
                               "\n"
                               "%e2;";
  const char *const inputThree = "<!DOCTYPE d [\n"
                                 "<!ENTITY % e ']><d'>\n"
                                 "\n"
                                 "%e;";
  const char *const inputIssue317 = "<!DOCTYPE doc [\n"
                                    "<!ENTITY % foo ']>\n"
                                    "<doc>Hell<oc (#PCDATA)*>'>\n"
                                    "%foo;\n"
                                    "]>\n"
                                    "<doc>Hello, world</dVc>";

  const char *const inputs[] = {inputOne, inputTwo, inputThree, inputIssue317};
  size_t inputIndex = 0;

  for (; inputIndex < sizeof(inputs) / sizeof(inputs[0]); inputIndex++) {
    XML_Parser parser;
    enum XML_Status parseResult;
    int setParamEntityResult;
    XML_Size lineNumber;
    XML_Size columnNumber;
    const char *const input = inputs[inputIndex];

    parser = XML_ParserCreate(NULL);
    setParamEntityResult
        = XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    if (setParamEntityResult != 1)
      fail("Failed to set XML_PARAM_ENTITY_PARSING_ALWAYS.");

    parseResult = XML_Parse(parser, input, (int)strlen(input), 0);
    if (parseResult != XML_STATUS_ERROR) {
      parseResult = XML_Parse(parser, "", 0, 1);
      if (parseResult != XML_STATUS_ERROR) {
        fail("Parsing was expected to fail but succeeded.");
      }
    }

    if (XML_GetErrorCode(parser) != XML_ERROR_INVALID_TOKEN)
      fail("Error code does not match XML_ERROR_INVALID_TOKEN");

    lineNumber = XML_GetCurrentLineNumber(parser);
    if (lineNumber != 4)
      fail("XML_GetCurrentLineNumber does not work as expected.");

    columnNumber = XML_GetCurrentColumnNumber(parser);
    if (columnNumber != 0)
      fail("XML_GetCurrentColumnNumber does not work as expected.");

    XML_ParserFree(parser);
  }
}
END_TEST

START_TEST(test_misc_tag_mismatch_reset_leak) {
#ifdef XML_NS
  const char *const text = "<open xmlns='https://namespace1.test'></close>";
  XML_Parser parser = XML_ParserCreateNS(NULL, XCS('\n'));

  if (XML_Parse(parser, text, (int)strlen(text), XML_TRUE) != XML_STATUS_ERROR)
    fail("Call to parse was expected to fail");
  if (XML_GetErrorCode(parser) != XML_ERROR_TAG_MISMATCH)
    fail("Call to parse was expected to fail from a closing tag mismatch");

  XML_ParserReset(parser, NULL);

  if (XML_Parse(parser, text, (int)strlen(text), XML_TRUE) != XML_STATUS_ERROR)
    fail("Call to parse was expected to fail");
  if (XML_GetErrorCode(parser) != XML_ERROR_TAG_MISMATCH)
    fail("Call to parse was expected to fail from a closing tag mismatch");

  XML_ParserFree(parser);
#endif
}
END_TEST

static void
alloc_setup(void) {
  XML_Memory_Handling_Suite memsuite = {duff_allocator, duff_reallocator, free};

  /* Ensure the parser creation will go through */
  allocation_count = ALLOC_ALWAYS_SUCCEED;
  reallocation_count = REALLOC_ALWAYS_SUCCEED;
  g_parser = XML_ParserCreate_MM(NULL, &memsuite, NULL);
  if (g_parser == NULL)
    fail("Parser not created");
}

static void
alloc_teardown(void) {
  basic_teardown();
}

/* Test the effects of allocation failures on xml declaration processing */
START_TEST(test_alloc_parse_xdecl) {
  const char *text = "<?xml version='1.0' encoding='utf-8'?>\n"
                     "<doc>Hello, world</doc>";
  int i;
  const int max_alloc_count = 15;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetXmlDeclHandler(g_parser, dummy_xdecl_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* Resetting the parser is insufficient, because some memory
     * allocations are cached within the parser.  Instead we use
     * the teardown and setup routines to ensure that we have the
     * right sort of parser back in our hands.
     */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed with max allocations");
}
END_TEST

/* As above, but with an encoding big enough to cause storing the
 * version information to expand the string pool being used.
 */
static int XMLCALL
long_encoding_handler(void *userData, const XML_Char *encoding,
                      XML_Encoding *info) {
  int i;

  UNUSED_P(userData);
  UNUSED_P(encoding);
  for (i = 0; i < 256; i++)
    info->map[i] = i;
  info->data = NULL;
  info->convert = NULL;
  info->release = NULL;
  return XML_STATUS_OK;
}

START_TEST(test_alloc_parse_xdecl_2) {
  const char *text
      = "<?xml version='1.0' encoding='"
        /* Each line is 64 characters */
        "ThisIsAStupidlyLongEncodingNameIntendedToTriggerPoolGrowth123456"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMN"
        "'?>"
        "<doc>Hello, world</doc>";
  int i;
  const int max_alloc_count = 20;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetXmlDeclHandler(g_parser, dummy_xdecl_handler);
    XML_SetUnknownEncodingHandler(g_parser, long_encoding_handler, NULL);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed with max allocations");
}
END_TEST

/* Test the effects of allocation failures on a straightforward parse */
START_TEST(test_alloc_parse_pi) {
  const char *text = "<?xml version='1.0' encoding='utf-8'?>\n"
                     "<?pi unknown?>\n"
                     "<doc>"
                     "Hello, world"
                     "</doc>";
  int i;
  const int max_alloc_count = 15;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetProcessingInstructionHandler(g_parser, dummy_pi_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed with max allocations");
}
END_TEST

START_TEST(test_alloc_parse_pi_2) {
  const char *text = "<?xml version='1.0' encoding='utf-8'?>\n"
                     "<doc>"
                     "Hello, world"
                     "<?pi unknown?>\n"
                     "</doc>";
  int i;
  const int max_alloc_count = 15;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetProcessingInstructionHandler(g_parser, dummy_pi_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed with max allocations");
}
END_TEST

START_TEST(test_alloc_parse_pi_3) {
  const char *text
      = "<?"
        /* 64 characters per line */
        "This processing instruction should be long enough to ensure that"
        "it triggers the growth of an internal string pool when the      "
        "allocator fails at a cruicial moment FGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "Q?><doc/>";
  int i;
  const int max_alloc_count = 20;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetProcessingInstructionHandler(g_parser, dummy_pi_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed with max allocations");
}
END_TEST

START_TEST(test_alloc_parse_comment) {
  const char *text = "<?xml version='1.0' encoding='utf-8'?>\n"
                     "<!-- Test parsing this comment -->"
                     "<doc>Hi</doc>";
  int i;
  const int max_alloc_count = 15;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetCommentHandler(g_parser, dummy_comment_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed with max allocations");
}
END_TEST

START_TEST(test_alloc_parse_comment_2) {
  const char *text = "<?xml version='1.0' encoding='utf-8'?>\n"
                     "<doc>"
                     "Hello, world"
                     "<!-- Parse this comment too -->"
                     "</doc>";
  int i;
  const int max_alloc_count = 15;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetCommentHandler(g_parser, dummy_comment_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed with max allocations");
}
END_TEST

static int XMLCALL
external_entity_duff_loader(XML_Parser parser, const XML_Char *context,
                            const XML_Char *base, const XML_Char *systemId,
                            const XML_Char *publicId) {
  XML_Parser new_parser;
  unsigned int i;
  const unsigned int max_alloc_count = 10;

  UNUSED_P(base);
  UNUSED_P(systemId);
  UNUSED_P(publicId);
  /* Try a few different allocation levels */
  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    new_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (new_parser != NULL) {
      XML_ParserFree(new_parser);
      break;
    }
  }
  if (i == 0)
    fail("External parser creation ignored failing allocator");
  else if (i == max_alloc_count)
    fail("Extern parser not created with max allocation count");

  /* Make sure other random allocation doesn't now fail */
  allocation_count = ALLOC_ALWAYS_SUCCEED;

  /* Make sure the failure code path is executed too */
  return XML_STATUS_ERROR;
}

/* Test that external parser creation running out of memory is
 * correctly reported.  Based on the external entity test cases.
 */
START_TEST(test_alloc_create_external_parser) {
  const char *text = "<?xml version='1.0' encoding='us-ascii'?>\n"
                     "<!DOCTYPE doc SYSTEM 'foo'>\n"
                     "<doc>&entity;</doc>";
  char foo_text[] = "<!ELEMENT doc (#PCDATA)*>";

  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetUserData(g_parser, foo_text);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_duff_loader);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      != XML_STATUS_ERROR) {
    fail("External parser allocator returned success incorrectly");
  }
}
END_TEST

/* More external parser memory allocation testing */
START_TEST(test_alloc_run_external_parser) {
  const char *text = "<?xml version='1.0' encoding='us-ascii'?>\n"
                     "<!DOCTYPE doc SYSTEM 'foo'>\n"
                     "<doc>&entity;</doc>";
  char foo_text[] = "<!ELEMENT doc (#PCDATA)*>";
  unsigned int i;
  const unsigned int max_alloc_count = 15;

  for (i = 0; i < max_alloc_count; i++) {
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetUserData(g_parser, foo_text);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_null_loader);
    allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parsing ignored failing allocator");
  else if (i == max_alloc_count)
    fail("Parsing failed with allocation count 10");
}
END_TEST

static int XMLCALL
external_entity_dbl_handler(XML_Parser parser, const XML_Char *context,
                            const XML_Char *base, const XML_Char *systemId,
                            const XML_Char *publicId) {
  intptr_t callno = (intptr_t)XML_GetUserData(parser);
  const char *text;
  XML_Parser new_parser = NULL;
  int i;
  const int max_alloc_count = 20;

  UNUSED_P(base);
  UNUSED_P(systemId);
  UNUSED_P(publicId);
  if (callno == 0) {
    /* First time through, check how many calls to malloc occur */
    text = ("<!ELEMENT doc (e+)>\n"
            "<!ATTLIST doc xmlns CDATA #IMPLIED>\n"
            "<!ELEMENT e EMPTY>\n");
    allocation_count = 10000;
    new_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (new_parser == NULL) {
      fail("Unable to allocate first external parser");
      return XML_STATUS_ERROR;
    }
    /* Stash the number of calls in the user data */
    XML_SetUserData(parser, (void *)(intptr_t)(10000 - allocation_count));
  } else {
    text = ("<?xml version='1.0' encoding='us-ascii'?>"
            "<e/>");
    /* Try at varying levels to exercise more code paths */
    for (i = 0; i < max_alloc_count; i++) {
      allocation_count = callno + i;
      new_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
      if (new_parser != NULL)
        break;
    }
    if (i == 0) {
      fail("Second external parser unexpectedly created");
      XML_ParserFree(new_parser);
      return XML_STATUS_ERROR;
    } else if (i == max_alloc_count) {
      fail("Second external parser not created");
      return XML_STATUS_ERROR;
    }
  }

  allocation_count = ALLOC_ALWAYS_SUCCEED;
  if (_XML_Parse_SINGLE_BYTES(new_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR) {
    xml_failure(new_parser);
    return XML_STATUS_ERROR;
  }
  XML_ParserFree(new_parser);
  return XML_STATUS_OK;
}

/* Test that running out of memory in dtdCopy is correctly reported.
 * Based on test_default_ns_from_ext_subset_and_ext_ge()
 */
START_TEST(test_alloc_dtd_copy_default_atts) {
  const char *text = "<?xml version='1.0'?>\n"
                     "<!DOCTYPE doc SYSTEM 'http://example.org/doc.dtd' [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/entity.ent'>\n"
                     "]>\n"
                     "<doc xmlns='http://example.org/ns1'>\n"
                     "&en;\n"
                     "</doc>";

  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_dbl_handler);
  XML_SetUserData(g_parser, NULL);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

static int XMLCALL
external_entity_dbl_handler_2(XML_Parser parser, const XML_Char *context,
                              const XML_Char *base, const XML_Char *systemId,
                              const XML_Char *publicId) {
  intptr_t callno = (intptr_t)XML_GetUserData(parser);
  const char *text;
  XML_Parser new_parser;
  enum XML_Status rv;

  UNUSED_P(base);
  UNUSED_P(systemId);
  UNUSED_P(publicId);
  if (callno == 0) {
    /* Try different allocation levels for whole exercise */
    text = ("<!ELEMENT doc (e+)>\n"
            "<!ATTLIST doc xmlns CDATA #IMPLIED>\n"
            "<!ELEMENT e EMPTY>\n");
    XML_SetUserData(parser, (void *)(intptr_t)1);
    new_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (new_parser == NULL)
      return XML_STATUS_ERROR;
    rv = _XML_Parse_SINGLE_BYTES(new_parser, text, (int)strlen(text), XML_TRUE);
  } else {
    /* Just run through once */
    text = ("<?xml version='1.0' encoding='us-ascii'?>"
            "<e/>");
    new_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (new_parser == NULL)
      return XML_STATUS_ERROR;
    rv = _XML_Parse_SINGLE_BYTES(new_parser, text, (int)strlen(text), XML_TRUE);
  }
  XML_ParserFree(new_parser);
  if (rv == XML_STATUS_ERROR)
    return XML_STATUS_ERROR;
  return XML_STATUS_OK;
}

/* Test more external entity allocation failure paths */
START_TEST(test_alloc_external_entity) {
  const char *text = "<?xml version='1.0'?>\n"
                     "<!DOCTYPE doc SYSTEM 'http://example.org/doc.dtd' [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/entity.ent'>\n"
                     "]>\n"
                     "<doc xmlns='http://example.org/ns1'>\n"
                     "&en;\n"
                     "</doc>";
  int i;
  const int alloc_test_max_repeats = 50;

  for (i = 0; i < alloc_test_max_repeats; i++) {
    allocation_count = -1;
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_dbl_handler_2);
    XML_SetUserData(g_parser, NULL);
    allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        == XML_STATUS_OK)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  allocation_count = -1;
  if (i == 0)
    fail("External entity parsed despite duff allocator");
  if (i == alloc_test_max_repeats)
    fail("External entity not parsed at max allocation count");
}
END_TEST

/* Test more allocation failure paths */
static int XMLCALL
external_entity_alloc_set_encoding(XML_Parser parser, const XML_Char *context,
                                   const XML_Char *base,
                                   const XML_Char *systemId,
                                   const XML_Char *publicId) {
  /* As for external_entity_loader() */
  const char *text = "<?xml encoding='iso-8859-3'?>"
                     "\xC3\xA9";
  XML_Parser ext_parser;
  enum XML_Status status;

  UNUSED_P(base);
  UNUSED_P(systemId);
  UNUSED_P(publicId);
  ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
  if (ext_parser == NULL)
    return XML_STATUS_ERROR;
  if (! XML_SetEncoding(ext_parser, XCS("utf-8"))) {
    XML_ParserFree(ext_parser);
    return XML_STATUS_ERROR;
  }
  status
      = _XML_Parse_SINGLE_BYTES(ext_parser, text, (int)strlen(text), XML_TRUE);
  XML_ParserFree(ext_parser);
  if (status == XML_STATUS_ERROR)
    return XML_STATUS_ERROR;
  return XML_STATUS_OK;
}

START_TEST(test_alloc_ext_entity_set_encoding) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";
  int i;
  const int max_allocation_count = 30;

  for (i = 0; i < max_allocation_count; i++) {
    XML_SetExternalEntityRefHandler(g_parser,
                                    external_entity_alloc_set_encoding);
    allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        == XML_STATUS_OK)
      break;
    allocation_count = -1;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Encoding check succeeded despite failing allocator");
  if (i == max_allocation_count)
    fail("Encoding failed at max allocation count");
}
END_TEST

/* Test the effects of allocation failure in internal entities.
 * Based on test_unknown_encoding_internal_entity
 */
START_TEST(test_alloc_internal_entity) {
  const char *text = "<?xml version='1.0' encoding='unsupported-encoding'?>\n"
                     "<!DOCTYPE test [<!ENTITY foo 'bar'>]>\n"
                     "<test a='&foo;'/>";
  unsigned int i;
  const unsigned int max_alloc_count = 20;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetUnknownEncodingHandler(g_parser, unknown_released_encoding_handler,
                                  NULL);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Internal entity worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Internal entity failed at max allocation count");
}
END_TEST

/* Test the robustness against allocation failure of element handling
 * Based on test_dtd_default_handling().
 */
START_TEST(test_alloc_dtd_default_handling) {
  const char *text = "<!DOCTYPE doc [\n"
                     "<!ENTITY e SYSTEM 'http://example.org/e'>\n"
                     "<!NOTATION n SYSTEM 'http://example.org/n'>\n"
                     "<!ENTITY e1 SYSTEM 'http://example.org/e' NDATA n>\n"
                     "<!ELEMENT doc (#PCDATA)>\n"
                     "<!ATTLIST doc a CDATA #IMPLIED>\n"
                     "<?pi in dtd?>\n"
                     "<!--comment in dtd-->\n"
                     "]>\n"
                     "<doc><![CDATA[text in doc]]></doc>";
  const XML_Char *expected = XCS("\n\n\n\n\n\n\n\n\n<doc>text in doc</doc>");
  CharData storage;
  int i;
  const int max_alloc_count = 25;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    init_dummy_handlers();
    XML_SetDefaultHandler(g_parser, accumulate_characters);
    XML_SetDoctypeDeclHandler(g_parser, dummy_start_doctype_handler,
                              dummy_end_doctype_handler);
    XML_SetEntityDeclHandler(g_parser, dummy_entity_decl_handler);
    XML_SetNotationDeclHandler(g_parser, dummy_notation_decl_handler);
    XML_SetElementDeclHandler(g_parser, dummy_element_decl_handler);
    XML_SetAttlistDeclHandler(g_parser, dummy_attlist_decl_handler);
    XML_SetProcessingInstructionHandler(g_parser, dummy_pi_handler);
    XML_SetCommentHandler(g_parser, dummy_comment_handler);
    XML_SetCdataSectionHandler(g_parser, dummy_start_cdata_handler,
                               dummy_end_cdata_handler);
    XML_SetUnparsedEntityDeclHandler(g_parser,
                                     dummy_unparsed_entity_decl_handler);
    CharData_Init(&storage);
    XML_SetUserData(g_parser, &storage);
    XML_SetCharacterDataHandler(g_parser, accumulate_characters);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Default DTD parsed despite allocation failures");
  if (i == max_alloc_count)
    fail("Default DTD not parsed with maximum alloc count");
  CharData_CheckXMLChars(&storage, expected);
  if (get_dummy_handler_flags()
      != (DUMMY_START_DOCTYPE_HANDLER_FLAG | DUMMY_END_DOCTYPE_HANDLER_FLAG
          | DUMMY_ENTITY_DECL_HANDLER_FLAG | DUMMY_NOTATION_DECL_HANDLER_FLAG
          | DUMMY_ELEMENT_DECL_HANDLER_FLAG | DUMMY_ATTLIST_DECL_HANDLER_FLAG
          | DUMMY_COMMENT_HANDLER_FLAG | DUMMY_PI_HANDLER_FLAG
          | DUMMY_START_CDATA_HANDLER_FLAG | DUMMY_END_CDATA_HANDLER_FLAG
          | DUMMY_UNPARSED_ENTITY_DECL_HANDLER_FLAG))
    fail("Not all handlers were called");
}
END_TEST

/* Test robustness of XML_SetEncoding() with a failing allocator */
START_TEST(test_alloc_explicit_encoding) {
  int i;
  const int max_alloc_count = 5;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    if (XML_SetEncoding(g_parser, XCS("us-ascii")) == XML_STATUS_OK)
      break;
  }
  if (i == 0)
    fail("Encoding set despite failing allocator");
  else if (i == max_alloc_count)
    fail("Encoding not set at max allocation count");
}
END_TEST

/* Test robustness of XML_SetBase against a failing allocator */
START_TEST(test_alloc_set_base) {
  const XML_Char *new_base = XCS("/local/file/name.xml");
  int i;
  const int max_alloc_count = 5;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    if (XML_SetBase(g_parser, new_base) == XML_STATUS_OK)
      break;
  }
  if (i == 0)
    fail("Base set despite failing allocator");
  else if (i == max_alloc_count)
    fail("Base not set with max allocation count");
}
END_TEST

/* Test buffer extension in the face of a duff reallocator */
START_TEST(test_alloc_realloc_buffer) {
  const char *text = get_buffer_test_text;
  void *buffer;
  int i;
  const int max_realloc_count = 10;

  /* Get a smallish buffer */
  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    buffer = XML_GetBuffer(g_parser, 1536);
    if (buffer == NULL)
      fail("1.5K buffer reallocation failed");
    assert(buffer != NULL);
    memcpy(buffer, text, strlen(text));
    if (XML_ParseBuffer(g_parser, (int)strlen(text), XML_FALSE)
        == XML_STATUS_OK)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  reallocation_count = -1;
  if (i == 0)
    fail("Parse succeeded with no reallocation");
  else if (i == max_realloc_count)
    fail("Parse failed with max reallocation count");
}
END_TEST

/* Same test for external entity parsers */
static int XMLCALL
external_entity_reallocator(XML_Parser parser, const XML_Char *context,
                            const XML_Char *base, const XML_Char *systemId,
                            const XML_Char *publicId) {
  const char *text = get_buffer_test_text;
  XML_Parser ext_parser;
  void *buffer;
  enum XML_Status status;

  UNUSED_P(base);
  UNUSED_P(systemId);
  UNUSED_P(publicId);
  ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
  if (ext_parser == NULL)
    fail("Could not create external entity parser");

  reallocation_count = (intptr_t)XML_GetUserData(parser);
  buffer = XML_GetBuffer(ext_parser, 1536);
  if (buffer == NULL)
    fail("Buffer allocation failed");
  assert(buffer != NULL);
  memcpy(buffer, text, strlen(text));
  status = XML_ParseBuffer(ext_parser, (int)strlen(text), XML_FALSE);
  reallocation_count = -1;
  XML_ParserFree(ext_parser);
  return (status == XML_STATUS_OK) ? XML_STATUS_OK : XML_STATUS_ERROR;
}

START_TEST(test_alloc_ext_entity_realloc_buffer) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";
  int i;
  const int max_realloc_count = 10;

  for (i = 0; i < max_realloc_count; i++) {
    XML_SetExternalEntityRefHandler(g_parser, external_entity_reallocator);
    XML_SetUserData(g_parser, (void *)(intptr_t)i);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        == XML_STATUS_OK)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Succeeded with no reallocations");
  if (i == max_realloc_count)
    fail("Failed with max reallocations");
}
END_TEST

/* Test elements with many attributes are handled correctly */
START_TEST(test_alloc_realloc_many_attributes) {
  const char *text = "<!DOCTYPE doc [\n"
                     "<!ATTLIST doc za CDATA 'default'>\n"
                     "<!ATTLIST doc zb CDATA 'def2'>\n"
                     "<!ATTLIST doc zc CDATA 'def3'>\n"
                     "]>\n"
                     "<doc a='1'"
                     "     b='2'"
                     "     c='3'"
                     "     d='4'"
                     "     e='5'"
                     "     f='6'"
                     "     g='7'"
                     "     h='8'"
                     "     i='9'"
                     "     j='10'"
                     "     k='11'"
                     "     l='12'"
                     "     m='13'"
                     "     n='14'"
                     "     p='15'"
                     "     q='16'"
                     "     r='17'"
                     "     s='18'>"
                     "</doc>";
  int i;
  const int max_realloc_count = 10;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite no reallocations");
  if (i == max_realloc_count)
    fail("Parse failed at max reallocations");
}
END_TEST

/* Test handling of a public entity with failing allocator */
START_TEST(test_alloc_public_entity_value) {
  const char *text = "<!DOCTYPE doc SYSTEM 'http://example.org/'>\n"
                     "<doc></doc>\n";
  char dtd_text[]
      = "<!ELEMENT doc EMPTY>\n"
        "<!ENTITY % e1 PUBLIC 'foo' 'bar.ent'>\n"
        "<!ENTITY % "
        /* Each line is 64 characters */
        "ThisIsAStupidlyLongParameterNameIntendedToTriggerPoolGrowth12345"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        " '%e1;'>\n"
        "%e1;\n";
  int i;
  const int max_alloc_count = 50;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    init_dummy_handlers();
    XML_SetUserData(g_parser, dtd_text);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_public);
    /* Provoke a particular code path */
    XML_SetEntityDeclHandler(g_parser, dummy_entity_decl_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocation");
  if (i == max_alloc_count)
    fail("Parsing failed at max allocation count");
  if (get_dummy_handler_flags() != DUMMY_ENTITY_DECL_HANDLER_FLAG)
    fail("Entity declaration handler not called");
}
END_TEST

START_TEST(test_alloc_realloc_subst_public_entity_value) {
  const char *text = "<!DOCTYPE doc SYSTEM 'http://example.org/'>\n"
                     "<doc></doc>\n";
  char dtd_text[]
      = "<!ELEMENT doc EMPTY>\n"
        "<!ENTITY % "
        /* Each line is 64 characters */
        "ThisIsAStupidlyLongParameterNameIntendedToTriggerPoolGrowth12345"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        " PUBLIC 'foo' 'bar.ent'>\n"
        "%ThisIsAStupidlyLongParameterNameIntendedToTriggerPoolGrowth12345"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP;";
  int i;
  const int max_realloc_count = 10;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    XML_SetUserData(g_parser, dtd_text);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_public);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing reallocation");
  if (i == max_realloc_count)
    fail("Parsing failed at max reallocation count");
}
END_TEST

START_TEST(test_alloc_parse_public_doctype) {
  const char *text
      = "<?xml version='1.0' encoding='utf-8'?>\n"
        "<!DOCTYPE doc PUBLIC '"
        /* 64 characters per line */
        "http://example.com/a/long/enough/name/to/trigger/pool/growth/zz/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "' 'test'>\n"
        "<doc></doc>";
  int i;
  const int max_alloc_count = 25;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    init_dummy_handlers();
    XML_SetDoctypeDeclHandler(g_parser, dummy_start_doctype_decl_handler,
                              dummy_end_doctype_decl_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
  if (get_dummy_handler_flags()
      != (DUMMY_START_DOCTYPE_DECL_HANDLER_FLAG
          | DUMMY_END_DOCTYPE_DECL_HANDLER_FLAG))
    fail("Doctype handler functions not called");
}
END_TEST

START_TEST(test_alloc_parse_public_doctype_long_name) {
  const char *text
      = "<?xml version='1.0' encoding='utf-8'?>\n"
        "<!DOCTYPE doc PUBLIC 'http://example.com/foo' '"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "'>\n"
        "<doc></doc>";
  int i;
  const int max_alloc_count = 25;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetDoctypeDeclHandler(g_parser, dummy_start_doctype_decl_handler,
                              dummy_end_doctype_decl_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
}
END_TEST

static int XMLCALL
external_entity_alloc(XML_Parser parser, const XML_Char *context,
                      const XML_Char *base, const XML_Char *systemId,
                      const XML_Char *publicId) {
  const char *text = (const char *)XML_GetUserData(parser);
  XML_Parser ext_parser;
  int parse_res;

  UNUSED_P(base);
  UNUSED_P(systemId);
  UNUSED_P(publicId);
  ext_parser = XML_ExternalEntityParserCreate(parser, context, NULL);
  if (ext_parser == NULL)
    return XML_STATUS_ERROR;
  parse_res
      = _XML_Parse_SINGLE_BYTES(ext_parser, text, (int)strlen(text), XML_TRUE);
  XML_ParserFree(ext_parser);
  return parse_res;
}

/* Test foreign DTD handling */
START_TEST(test_alloc_set_foreign_dtd) {
  const char *text1 = "<?xml version='1.0' encoding='us-ascii'?>\n"
                      "<doc>&entity;</doc>";
  char text2[] = "<!ELEMENT doc (#PCDATA)*>";
  int i;
  const int max_alloc_count = 25;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetUserData(g_parser, &text2);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_alloc);
    if (XML_UseForeignDTD(g_parser, XML_TRUE) != XML_ERROR_NONE)
      fail("Could not set foreign DTD");
    if (_XML_Parse_SINGLE_BYTES(g_parser, text1, (int)strlen(text1), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
}
END_TEST

/* Test based on ibm/valid/P32/ibm32v04.xml */
START_TEST(test_alloc_attribute_enum_value) {
  const char *text = "<?xml version='1.0' standalone='no'?>\n"
                     "<!DOCTYPE animal SYSTEM 'test.dtd'>\n"
                     "<animal>This is a \n    <a/>  \n\nyellow tiger</animal>";
  char dtd_text[] = "<!ELEMENT animal (#PCDATA|a)*>\n"
                    "<!ELEMENT a EMPTY>\n"
                    "<!ATTLIST animal xml:space (default|preserve) 'preserve'>";
  int i;
  const int max_alloc_count = 30;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetExternalEntityRefHandler(g_parser, external_entity_alloc);
    XML_SetUserData(g_parser, dtd_text);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    /* An attribute list handler provokes a different code path */
    XML_SetAttlistDeclHandler(g_parser, dummy_attlist_decl_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
}
END_TEST

/* Test attribute enums sufficient to overflow the string pool */
START_TEST(test_alloc_realloc_attribute_enum_value) {
  const char *text = "<?xml version='1.0' standalone='no'?>\n"
                     "<!DOCTYPE animal SYSTEM 'test.dtd'>\n"
                     "<animal>This is a yellow tiger</animal>";
  /* We wish to define a collection of attribute enums that will
   * cause the string pool storing them to have to expand.  This
   * means more than 1024 bytes, including the parentheses and
   * separator bars.
   */
  char dtd_text[]
      = "<!ELEMENT animal (#PCDATA)*>\n"
        "<!ATTLIST animal thing "
        "(default"
        /* Each line is 64 characters */
        "|ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|BBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|CBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|DBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|EBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|FBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|GBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|HBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|IBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|JBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|KBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|LBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|MBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|NBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|OBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|PBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO)"
        " 'default'>";
  int i;
  const int max_realloc_count = 10;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    XML_SetExternalEntityRefHandler(g_parser, external_entity_alloc);
    XML_SetUserData(g_parser, dtd_text);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    /* An attribute list handler provokes a different code path */
    XML_SetAttlistDeclHandler(g_parser, dummy_attlist_decl_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing reallocator");
  if (i == max_realloc_count)
    fail("Parse failed at maximum reallocation count");
}
END_TEST

/* Test attribute enums in a #IMPLIED attribute forcing pool growth */
START_TEST(test_alloc_realloc_implied_attribute) {
  /* Forcing this particular code path is a balancing act.  The
   * addition of the closing parenthesis and terminal NUL must be
   * what pushes the string of enums over the 1024-byte limit,
   * otherwise a different code path will pick up the realloc.
   */
  const char *text
      = "<!DOCTYPE doc [\n"
        "<!ELEMENT doc EMPTY>\n"
        "<!ATTLIST doc a "
        /* Each line is 64 characters */
        "(ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|BBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|CBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|DBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|EBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|FBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|GBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|HBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|IBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|JBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|KBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|LBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|MBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|NBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|OBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|PBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMN)"
        " #IMPLIED>\n"
        "]><doc/>";
  int i;
  const int max_realloc_count = 10;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    XML_SetAttlistDeclHandler(g_parser, dummy_attlist_decl_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing reallocator");
  if (i == max_realloc_count)
    fail("Parse failed at maximum reallocation count");
}
END_TEST

/* Test attribute enums in a defaulted attribute forcing pool growth */
START_TEST(test_alloc_realloc_default_attribute) {
  /* Forcing this particular code path is a balancing act.  The
   * addition of the closing parenthesis and terminal NUL must be
   * what pushes the string of enums over the 1024-byte limit,
   * otherwise a different code path will pick up the realloc.
   */
  const char *text
      = "<!DOCTYPE doc [\n"
        "<!ELEMENT doc EMPTY>\n"
        "<!ATTLIST doc a "
        /* Each line is 64 characters */
        "(ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|BBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|CBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|DBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|EBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|FBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|GBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|HBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|IBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|JBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|KBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|LBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|MBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|NBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|OBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        "|PBCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMN)"
        " 'ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO'"
        ">\n]><doc/>";
  int i;
  const int max_realloc_count = 10;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    XML_SetAttlistDeclHandler(g_parser, dummy_attlist_decl_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing reallocator");
  if (i == max_realloc_count)
    fail("Parse failed at maximum reallocation count");
}
END_TEST

/* Test long notation name with dodgy allocator */
START_TEST(test_alloc_notation) {
  const char *text
      = "<!DOCTYPE doc [\n"
        "<!NOTATION "
        /* Each line is 64 characters */
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        " SYSTEM 'http://example.org/n'>\n"
        "<!ENTITY e SYSTEM 'http://example.org/e' NDATA "
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        ">\n"
        "<!ELEMENT doc EMPTY>\n"
        "]>\n<doc/>";
  int i;
  const int max_alloc_count = 20;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    init_dummy_handlers();
    XML_SetNotationDeclHandler(g_parser, dummy_notation_decl_handler);
    XML_SetEntityDeclHandler(g_parser, dummy_entity_decl_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite allocation failures");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
  if (get_dummy_handler_flags()
      != (DUMMY_ENTITY_DECL_HANDLER_FLAG | DUMMY_NOTATION_DECL_HANDLER_FLAG))
    fail("Entity declaration handler not called");
}
END_TEST

/* Test public notation with dodgy allocator */
START_TEST(test_alloc_public_notation) {
  const char *text
      = "<!DOCTYPE doc [\n"
        "<!NOTATION note PUBLIC '"
        /* 64 characters per line */
        "http://example.com/a/long/enough/name/to/trigger/pool/growth/zz/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "' 'foo'>\n"
        "<!ENTITY e SYSTEM 'http://example.com/e' NDATA note>\n"
        "<!ELEMENT doc EMPTY>\n"
        "]>\n<doc/>";
  int i;
  const int max_alloc_count = 20;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    init_dummy_handlers();
    XML_SetNotationDeclHandler(g_parser, dummy_notation_decl_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite allocation failures");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
  if (get_dummy_handler_flags() != DUMMY_NOTATION_DECL_HANDLER_FLAG)
    fail("Notation handler not called");
}
END_TEST

/* Test public notation with dodgy allocator */
START_TEST(test_alloc_system_notation) {
  const char *text
      = "<!DOCTYPE doc [\n"
        "<!NOTATION note SYSTEM '"
        /* 64 characters per line */
        "http://example.com/a/long/enough/name/to/trigger/pool/growth/zz/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "'>\n"
        "<!ENTITY e SYSTEM 'http://example.com/e' NDATA note>\n"
        "<!ELEMENT doc EMPTY>\n"
        "]>\n<doc/>";
  int i;
  const int max_alloc_count = 20;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    init_dummy_handlers();
    XML_SetNotationDeclHandler(g_parser, dummy_notation_decl_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite allocation failures");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
  if (get_dummy_handler_flags() != DUMMY_NOTATION_DECL_HANDLER_FLAG)
    fail("Notation handler not called");
}
END_TEST

START_TEST(test_alloc_nested_groups) {
  const char *text
      = "<!DOCTYPE doc [\n"
        "<!ELEMENT doc "
        /* Sixteen elements per line */
        "(e,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,"
        "(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?"
        "))))))))))))))))))))))))))))))))>\n"
        "<!ELEMENT e EMPTY>"
        "]>\n"
        "<doc><e/></doc>";
  CharData storage;
  int i;
  const int max_alloc_count = 20;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    CharData_Init(&storage);
    XML_SetElementDeclHandler(g_parser, dummy_element_decl_handler);
    XML_SetStartElementHandler(g_parser, record_element_start_handler);
    XML_SetUserData(g_parser, &storage);
    init_dummy_handlers();
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }

  if (i == 0)
    fail("Parse succeeded despite failing reallocator");
  if (i == max_alloc_count)
    fail("Parse failed at maximum reallocation count");
  CharData_CheckXMLChars(&storage, XCS("doce"));
  if (get_dummy_handler_flags() != DUMMY_ELEMENT_DECL_HANDLER_FLAG)
    fail("Element handler not fired");
}
END_TEST

START_TEST(test_alloc_realloc_nested_groups) {
  const char *text
      = "<!DOCTYPE doc [\n"
        "<!ELEMENT doc "
        /* Sixteen elements per line */
        "(e,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,"
        "(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?,(e?"
        "))))))))))))))))))))))))))))))))>\n"
        "<!ELEMENT e EMPTY>"
        "]>\n"
        "<doc><e/></doc>";
  CharData storage;
  int i;
  const int max_realloc_count = 10;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    CharData_Init(&storage);
    XML_SetElementDeclHandler(g_parser, dummy_element_decl_handler);
    XML_SetStartElementHandler(g_parser, record_element_start_handler);
    XML_SetUserData(g_parser, &storage);
    init_dummy_handlers();
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }

  if (i == 0)
    fail("Parse succeeded despite failing reallocator");
  if (i == max_realloc_count)
    fail("Parse failed at maximum reallocation count");
  CharData_CheckXMLChars(&storage, XCS("doce"));
  if (get_dummy_handler_flags() != DUMMY_ELEMENT_DECL_HANDLER_FLAG)
    fail("Element handler not fired");
}
END_TEST

START_TEST(test_alloc_large_group) {
  const char *text = "<!DOCTYPE doc [\n"
                     "<!ELEMENT doc ("
                     "a1|a2|a3|a4|a5|a6|a7|a8|"
                     "b1|b2|b3|b4|b5|b6|b7|b8|"
                     "c1|c2|c3|c4|c5|c6|c7|c8|"
                     "d1|d2|d3|d4|d5|d6|d7|d8|"
                     "e1"
                     ")+>\n"
                     "]>\n"
                     "<doc>\n"
                     "<a1/>\n"
                     "</doc>\n";
  int i;
  const int max_alloc_count = 50;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetElementDeclHandler(g_parser, dummy_element_decl_handler);
    init_dummy_handlers();
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
  if (get_dummy_handler_flags() != DUMMY_ELEMENT_DECL_HANDLER_FLAG)
    fail("Element handler flag not raised");
}
END_TEST

START_TEST(test_alloc_realloc_group_choice) {
  const char *text = "<!DOCTYPE doc [\n"
                     "<!ELEMENT doc ("
                     "a1|a2|a3|a4|a5|a6|a7|a8|"
                     "b1|b2|b3|b4|b5|b6|b7|b8|"
                     "c1|c2|c3|c4|c5|c6|c7|c8|"
                     "d1|d2|d3|d4|d5|d6|d7|d8|"
                     "e1"
                     ")+>\n"
                     "]>\n"
                     "<doc>\n"
                     "<a1/>\n"
                     "<b2 attr='foo'>This is a foo</b2>\n"
                     "<c3></c3>\n"
                     "</doc>\n";
  int i;
  const int max_realloc_count = 10;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    XML_SetElementDeclHandler(g_parser, dummy_element_decl_handler);
    init_dummy_handlers();
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing reallocator");
  if (i == max_realloc_count)
    fail("Parse failed at maximum reallocation count");
  if (get_dummy_handler_flags() != DUMMY_ELEMENT_DECL_HANDLER_FLAG)
    fail("Element handler flag not raised");
}
END_TEST

START_TEST(test_alloc_pi_in_epilog) {
  const char *text = "<doc></doc>\n"
                     "<?pi in epilog?>";
  int i;
  const int max_alloc_count = 15;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetProcessingInstructionHandler(g_parser, dummy_pi_handler);
    init_dummy_handlers();
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse completed despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
  if (get_dummy_handler_flags() != DUMMY_PI_HANDLER_FLAG)
    fail("Processing instruction handler not invoked");
}
END_TEST

START_TEST(test_alloc_comment_in_epilog) {
  const char *text = "<doc></doc>\n"
                     "<!-- comment in epilog -->";
  int i;
  const int max_alloc_count = 15;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetCommentHandler(g_parser, dummy_comment_handler);
    init_dummy_handlers();
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse completed despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
  if (get_dummy_handler_flags() != DUMMY_COMMENT_HANDLER_FLAG)
    fail("Processing instruction handler not invoked");
}
END_TEST

START_TEST(test_alloc_realloc_long_attribute_value) {
  const char *text
      = "<!DOCTYPE doc [<!ENTITY foo '"
        /* Each line is 64 characters */
        "This entity will be substituted as an attribute value, and is   "
        "calculated to be exactly long enough that the terminating NUL   "
        "that the library adds internally will trigger the string pool to"
        "grow. GHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "'>]>\n"
        "<doc a='&foo;'></doc>";
  int i;
  const int max_realloc_count = 10;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing reallocator");
  if (i == max_realloc_count)
    fail("Parse failed at maximum reallocation count");
}
END_TEST

START_TEST(test_alloc_attribute_whitespace) {
  const char *text = "<doc a=' '></doc>";
  int i;
  const int max_alloc_count = 15;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
}
END_TEST

START_TEST(test_alloc_attribute_predefined_entity) {
  const char *text = "<doc a='&amp;'></doc>";
  int i;
  const int max_alloc_count = 15;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
}
END_TEST

/* Test that a character reference at the end of a suitably long
 * default value for an attribute can trigger pool growth, and recovers
 * if the allocator fails on it.
 */
START_TEST(test_alloc_long_attr_default_with_char_ref) {
  const char *text
      = "<!DOCTYPE doc [<!ATTLIST doc a CDATA '"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHI"
        "&#x31;'>]>\n"
        "<doc/>";
  int i;
  const int max_alloc_count = 20;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
}
END_TEST

/* Test that a long character reference substitution triggers a pool
 * expansion correctly for an attribute value.
 */
START_TEST(test_alloc_long_attr_value) {
  const char *text
      = "<!DOCTYPE test [<!ENTITY foo '\n"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "'>]>\n"
        "<test a='&foo;'/>";
  int i;
  const int max_alloc_count = 25;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing allocator");
  if (i == max_alloc_count)
    fail("Parse failed at maximum allocation count");
}
END_TEST

/* Test that an error in a nested parameter entity substitution is
 * handled correctly.  It seems unlikely that the code path being
 * exercised can be reached purely by carefully crafted XML, but an
 * allocation error in the right place will definitely do it.
 */
START_TEST(test_alloc_nested_entities) {
  const char *text = "<!DOCTYPE doc SYSTEM 'http://example.org/one.ent'>\n"
                     "<doc />";
  ExtFaults test_data
      = {"<!ENTITY % pe1 '"
         /* 64 characters per line */
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
         "'>\n"
         "<!ENTITY % pe2 '%pe1;'>\n"
         "%pe2;",
         "Memory Fail not faulted", NULL, XML_ERROR_NO_MEMORY};

  /* Causes an allocation error in a nested storeEntityValue() */
  allocation_count = 12;
  XML_SetUserData(g_parser, &test_data);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_faulter);
  expect_failure(text, XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                 "Entity allocation failure not noted");
}
END_TEST

START_TEST(test_alloc_realloc_param_entity_newline) {
  const char *text = "<!DOCTYPE doc SYSTEM 'http://example.org/'>\n"
                     "<doc/>";
  char dtd_text[]
      = "<!ENTITY % pe '<!ATTLIST doc att CDATA \""
        /* 64 characters per line */
        "This default value is carefully crafted so that the carriage    "
        "return right at the end of the entity string causes an internal "
        "string pool to have to grow.  This allows us to test the alloc  "
        "failure path from that point. OPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDE"
        "\">\n'>"
        "%pe;\n";
  int i;
  const int max_realloc_count = 5;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    XML_SetUserData(g_parser, dtd_text);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_alloc);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing reallocator");
  if (i == max_realloc_count)
    fail("Parse failed at maximum reallocation count");
}
END_TEST

START_TEST(test_alloc_realloc_ce_extends_pe) {
  const char *text = "<!DOCTYPE doc SYSTEM 'http://example.org/'>\n"
                     "<doc/>";
  char dtd_text[]
      = "<!ENTITY % pe '<!ATTLIST doc att CDATA \""
        /* 64 characters per line */
        "This default value is carefully crafted so that the character   "
        "entity at the end causes an internal string pool to have to     "
        "grow.  This allows us to test the allocation failure path from  "
        "that point onwards. EFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFG&#x51;"
        "\">\n'>"
        "%pe;\n";
  int i;
  const int max_realloc_count = 5;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    XML_SetUserData(g_parser, dtd_text);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_alloc);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parse succeeded despite failing reallocator");
  if (i == max_realloc_count)
    fail("Parse failed at maximum reallocation count");
}
END_TEST

START_TEST(test_alloc_realloc_attributes) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ATTLIST doc\n"
                     "    a1  (a|b|c)   'a'\n"
                     "    a2  (foo|bar) #IMPLIED\n"
                     "    a3  NMTOKEN   #IMPLIED\n"
                     "    a4  NMTOKENS  #IMPLIED\n"
                     "    a5  ID        #IMPLIED\n"
                     "    a6  IDREF     #IMPLIED\n"
                     "    a7  IDREFS    #IMPLIED\n"
                     "    a8  ENTITY    #IMPLIED\n"
                     "    a9  ENTITIES  #IMPLIED\n"
                     "    a10 CDATA     #IMPLIED\n"
                     "  >]>\n"
                     "<doc>wombat</doc>\n";
  int i;
  const int max_realloc_count = 5;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }

  if (i == 0)
    fail("Parse succeeded despite failing reallocator");
  if (i == max_realloc_count)
    fail("Parse failed at maximum reallocation count");
}
END_TEST

START_TEST(test_alloc_long_doc_name) {
  const char *text =
      /* 64 characters per line */
      "<LongRootElementNameThatWillCauseTheNextAllocationToExpandTheStr"
      "ingPoolForTheDTDQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
      " a='1'/>";
  int i;
  const int max_alloc_count = 20;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing reallocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max reallocation count");
}
END_TEST

START_TEST(test_alloc_long_base) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY e SYSTEM 'foo'>\n"
                     "]>\n"
                     "<doc>&e;</doc>";
  char entity_text[] = "Hello world";
  const XML_Char *base =
      /* 64 characters per line */
      /* clang-format off */
        XCS("LongBaseURI/that/will/overflow/an/internal/buffer/and/cause/it/t")
        XCS("o/have/to/grow/PQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/");
  /* clang-format on */
  int i;
  const int max_alloc_count = 25;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetUserData(g_parser, entity_text);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_alloc);
    if (XML_SetBase(g_parser, base) == XML_STATUS_ERROR) {
      XML_ParserReset(g_parser, NULL);
      continue;
    }
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max allocation count");
}
END_TEST

START_TEST(test_alloc_long_public_id) {
  const char *text
      = "<!DOCTYPE doc [\n"
        "  <!ENTITY e PUBLIC '"
        /* 64 characters per line */
        "LongPublicIDThatShouldResultInAnInternalStringPoolGrowingAtASpec"
        "ificMomentKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "' 'bar'>\n"
        "]>\n"
        "<doc>&e;</doc>";
  char entity_text[] = "Hello world";
  int i;
  const int max_alloc_count = 40;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetUserData(g_parser, entity_text);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_alloc);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max allocation count");
}
END_TEST

START_TEST(test_alloc_long_entity_value) {
  const char *text
      = "<!DOCTYPE doc [\n"
        "  <!ENTITY e1 '"
        /* 64 characters per line */
        "Long entity value that should provoke a string pool to grow whil"
        "e setting up to parse the external entity below. xyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "'>\n"
        "  <!ENTITY e2 SYSTEM 'bar'>\n"
        "]>\n"
        "<doc>&e2;</doc>";
  char entity_text[] = "Hello world";
  int i;
  const int max_alloc_count = 40;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetUserData(g_parser, entity_text);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_alloc);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max allocation count");
}
END_TEST

START_TEST(test_alloc_long_notation) {
  const char *text
      = "<!DOCTYPE doc [\n"
        "  <!NOTATION note SYSTEM '"
        /* 64 characters per line */
        "ALongNotationNameThatShouldProvokeStringPoolGrowthWhileCallingAn"
        "ExternalEntityParserUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "'>\n"
        "  <!ENTITY e1 SYSTEM 'foo' NDATA "
        /* 64 characters per line */
        "ALongNotationNameThatShouldProvokeStringPoolGrowthWhileCallingAn"
        "ExternalEntityParserUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB"
        ">\n"
        "  <!ENTITY e2 SYSTEM 'bar'>\n"
        "]>\n"
        "<doc>&e2;</doc>";
  ExtOption options[]
      = {{XCS("foo"), "Entity Foo"}, {XCS("bar"), "Entity Bar"}, {NULL, NULL}};
  int i;
  const int max_alloc_count = 40;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetUserData(g_parser, options);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_optioner);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;

    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max allocation count");
}
END_TEST

static int XMLCALL
external_entity_parser_create_alloc_fail_handler(XML_Parser parser,
                                                 const XML_Char *context,
                                                 const XML_Char *base,
                                                 const XML_Char *systemId,
                                                 const XML_Char *publicId) {
  UNUSED_P(base);
  UNUSED_P(systemId);
  UNUSED_P(publicId);

  if (context != NULL)
    fail("Unexpected non-NULL context");

  // The following number intends to fail the upcoming allocation in line
  // "parser->m_protocolEncodingName = copyString(encodingName,
  // &(parser->m_mem));" in function parserInit.
  allocation_count = 3;

  const XML_Char *const encodingName = XCS("UTF-8"); // needs something non-NULL
  const XML_Parser ext_parser
      = XML_ExternalEntityParserCreate(parser, context, encodingName);
  if (ext_parser != NULL)
    fail(
        "Call to XML_ExternalEntityParserCreate was expected to fail out-of-memory");

  allocation_count = ALLOC_ALWAYS_SUCCEED;
  return XML_STATUS_ERROR;
}

START_TEST(test_alloc_reset_after_external_entity_parser_create_fail) {
  const char *const text = "<!DOCTYPE doc SYSTEM 'foo'><doc/>";

  XML_SetExternalEntityRefHandler(
      g_parser, external_entity_parser_create_alloc_fail_handler);
  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);

  if (XML_Parse(g_parser, text, (int)strlen(text), XML_TRUE)
      != XML_STATUS_ERROR)
    fail("Call to parse was expected to fail");

  if (XML_GetErrorCode(g_parser) != XML_ERROR_EXTERNAL_ENTITY_HANDLING)
    fail("Call to parse was expected to fail from the external entity handler");

  XML_ParserReset(g_parser, NULL);
}
END_TEST

static void
nsalloc_setup(void) {
  XML_Memory_Handling_Suite memsuite = {duff_allocator, duff_reallocator, free};
  XML_Char ns_sep[2] = {' ', '\0'};

  /* Ensure the parser creation will go through */
  allocation_count = ALLOC_ALWAYS_SUCCEED;
  reallocation_count = REALLOC_ALWAYS_SUCCEED;
  g_parser = XML_ParserCreate_MM(NULL, &memsuite, ns_sep);
  if (g_parser == NULL)
    fail("Parser not created");
}

static void
nsalloc_teardown(void) {
  basic_teardown();
}

/* Test the effects of allocation failure in simple namespace parsing.
 * Based on test_ns_default_with_empty_uri()
 */
START_TEST(test_nsalloc_xmlns) {
  const char *text = "<doc xmlns='http://example.org/'>\n"
                     "  <e xmlns=''/>\n"
                     "</doc>";
  unsigned int i;
  const unsigned int max_alloc_count = 30;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    /* Exercise more code paths with a default handler */
    XML_SetDefaultHandler(g_parser, dummy_default_handler);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* Resetting the parser is insufficient, because some memory
     * allocations are cached within the parser.  Instead we use
     * the teardown and setup routines to ensure that we have the
     * right sort of parser back in our hands.
     */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at maximum allocation count");
}
END_TEST

/* Test XML_ParseBuffer interface with namespace and a dicky allocator */
START_TEST(test_nsalloc_parse_buffer) {
  const char *text = "<doc>Hello</doc>";
  void *buffer;

  /* Try a parse before the start of the world */
  /* (Exercises new code path) */
  if (XML_ParseBuffer(g_parser, 0, XML_FALSE) != XML_STATUS_ERROR)
    fail("Pre-init XML_ParseBuffer not faulted");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_NO_BUFFER)
    fail("Pre-init XML_ParseBuffer faulted for wrong reason");

  buffer = XML_GetBuffer(g_parser, 1 /* any small number greater than 0 */);
  if (buffer == NULL)
    fail("Could not acquire parse buffer");

  allocation_count = 0;
  if (XML_ParseBuffer(g_parser, 0, XML_FALSE) != XML_STATUS_ERROR)
    fail("Pre-init XML_ParseBuffer not faulted");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_NO_MEMORY)
    fail("Pre-init XML_ParseBuffer faulted for wrong reason");

  /* Now with actual memory allocation */
  allocation_count = ALLOC_ALWAYS_SUCCEED;
  if (XML_ParseBuffer(g_parser, 0, XML_FALSE) != XML_STATUS_OK)
    xml_failure(g_parser);

  /* Check that resuming an unsuspended parser is faulted */
  if (XML_ResumeParser(g_parser) != XML_STATUS_ERROR)
    fail("Resuming unsuspended parser not faulted");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_NOT_SUSPENDED)
    xml_failure(g_parser);

  /* Get the parser into suspended state */
  XML_SetCharacterDataHandler(g_parser, clearing_aborting_character_handler);
  g_resumable = XML_TRUE;
  buffer = XML_GetBuffer(g_parser, (int)strlen(text));
  if (buffer == NULL)
    fail("Could not acquire parse buffer");
  assert(buffer != NULL);
  memcpy(buffer, text, strlen(text));
  if (XML_ParseBuffer(g_parser, (int)strlen(text), XML_TRUE)
      != XML_STATUS_SUSPENDED)
    xml_failure(g_parser);
  if (XML_GetErrorCode(g_parser) != XML_ERROR_NONE)
    xml_failure(g_parser);
  if (XML_ParseBuffer(g_parser, (int)strlen(text), XML_TRUE)
      != XML_STATUS_ERROR)
    fail("Suspended XML_ParseBuffer not faulted");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_SUSPENDED)
    xml_failure(g_parser);
  if (XML_GetBuffer(g_parser, (int)strlen(text)) != NULL)
    fail("Suspended XML_GetBuffer not faulted");

  /* Get it going again and complete the world */
  XML_SetCharacterDataHandler(g_parser, NULL);
  if (XML_ResumeParser(g_parser) != XML_STATUS_OK)
    xml_failure(g_parser);
  if (XML_ParseBuffer(g_parser, (int)strlen(text), XML_TRUE)
      != XML_STATUS_ERROR)
    fail("Post-finishing XML_ParseBuffer not faulted");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_FINISHED)
    xml_failure(g_parser);
  if (XML_GetBuffer(g_parser, (int)strlen(text)) != NULL)
    fail("Post-finishing XML_GetBuffer not faulted");
}
END_TEST

/* Check handling of long prefix names (pool growth) */
START_TEST(test_nsalloc_long_prefix) {
  const char *text
      = "<"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        ":foo xmlns:"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "='http://example.org/'>"
        "</"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        ":foo>";
  int i;
  const int max_alloc_count = 40;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max allocation count");
}
END_TEST

/* Check handling of long uri names (pool growth) */
START_TEST(test_nsalloc_long_uri) {
  const char *text
      = "<foo:e xmlns:foo='http://example.org/"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "' bar:a='12'\n"
        "xmlns:bar='http://example.org/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789A/"
        "'>"
        "</foo:e>";
  int i;
  const int max_alloc_count = 40;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max allocation count");
}
END_TEST

/* Test handling of long attribute names with prefixes */
START_TEST(test_nsalloc_long_attr) {
  const char *text
      = "<foo:e xmlns:foo='http://example.org/' bar:"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "='12'\n"
        "xmlns:bar='http://example.org/'>"
        "</foo:e>";
  int i;
  const int max_alloc_count = 40;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max allocation count");
}
END_TEST

/* Test handling of an attribute name with a long namespace prefix */
START_TEST(test_nsalloc_long_attr_prefix) {
  const char *text
      = "<foo:e xmlns:foo='http://example.org/' "
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        ":a='12'\n"
        "xmlns:"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "='http://example.org/'>"
        "</foo:e>";
  const XML_Char *elemstr[] = {
      /* clang-format off */
        XCS("http://example.org/ e foo"),
        XCS("http://example.org/ a ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
        XCS("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ")
      /* clang-format on */
  };
  int i;
  const int max_alloc_count = 40;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetReturnNSTriplet(g_parser, XML_TRUE);
    XML_SetUserData(g_parser, (void *)elemstr);
    XML_SetElementHandler(g_parser, triplet_start_checker, triplet_end_checker);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max allocation count");
}
END_TEST

/* Test attribute handling in the face of a dodgy reallocator */
START_TEST(test_nsalloc_realloc_attributes) {
  const char *text = "<foo:e xmlns:foo='http://example.org/' bar:a='12'\n"
                     "       xmlns:bar='http://example.org/'>"
                     "</foo:e>";
  int i;
  const int max_realloc_count = 10;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing reallocations");
  else if (i == max_realloc_count)
    fail("Parsing failed at max reallocation count");
}
END_TEST

/* Test long element names with namespaces under a failing allocator */
START_TEST(test_nsalloc_long_element) {
  const char *text
      = "<foo:thisisalongenoughelementnametotriggerareallocation\n"
        " xmlns:foo='http://example.org/' bar:a='12'\n"
        " xmlns:bar='http://example.org/'>"
        "</foo:thisisalongenoughelementnametotriggerareallocation>";
  const XML_Char *elemstr[]
      = {XCS("http://example.org/")
             XCS(" thisisalongenoughelementnametotriggerareallocation foo"),
         XCS("http://example.org/ a bar")};
  int i;
  const int max_alloc_count = 30;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetReturnNSTriplet(g_parser, XML_TRUE);
    XML_SetUserData(g_parser, (void *)elemstr);
    XML_SetElementHandler(g_parser, triplet_start_checker, triplet_end_checker);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing reallocations");
  else if (i == max_alloc_count)
    fail("Parsing failed at max reallocation count");
}
END_TEST

/* Test the effects of reallocation failure when reassigning a
 * binding.
 *
 * XML_ParserReset does not free the BINDING structures used by a
 * parser, but instead adds them to an internal free list to be reused
 * as necessary.  Likewise the URI buffers allocated for the binding
 * aren't freed, but kept attached to their existing binding.  If the
 * new binding has a longer URI, it will need reallocation.  This test
 * provokes that reallocation, and tests the control path if it fails.
 */
START_TEST(test_nsalloc_realloc_binding_uri) {
  const char *first = "<doc xmlns='http://example.org/'>\n"
                      "  <e xmlns='' />\n"
                      "</doc>";
  const char *second
      = "<doc xmlns='http://example.org/long/enough/URI/to/reallocate/'>\n"
        "  <e xmlns='' />\n"
        "</doc>";
  unsigned i;
  const unsigned max_realloc_count = 10;

  /* First, do a full parse that will leave bindings around */
  if (_XML_Parse_SINGLE_BYTES(g_parser, first, (int)strlen(first), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);

  /* Now repeat with a longer URI and a duff reallocator */
  for (i = 0; i < max_realloc_count; i++) {
    XML_ParserReset(g_parser, NULL);
    reallocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, second, (int)strlen(second), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
  }
  if (i == 0)
    fail("Parsing worked despite failing reallocation");
  else if (i == max_realloc_count)
    fail("Parsing failed at max reallocation count");
}
END_TEST

/* Check handling of long prefix names (pool growth) */
START_TEST(test_nsalloc_realloc_long_prefix) {
  const char *text
      = "<"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        ":foo xmlns:"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "='http://example.org/'>"
        "</"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        ":foo>";
  int i;
  const int max_realloc_count = 12;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing reallocations");
  else if (i == max_realloc_count)
    fail("Parsing failed even at max reallocation count");
}
END_TEST

/* Check handling of even long prefix names (different code path) */
START_TEST(test_nsalloc_realloc_longer_prefix) {
  const char *text
      = "<"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "Q:foo xmlns:"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "Q='http://example.org/'>"
        "</"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "Q:foo>";
  int i;
  const int max_realloc_count = 12;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing reallocations");
  else if (i == max_realloc_count)
    fail("Parsing failed even at max reallocation count");
}
END_TEST

START_TEST(test_nsalloc_long_namespace) {
  const char *text1
      = "<"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        ":e xmlns:"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "='http://example.org/'>\n";
  const char *text2
      = "<"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        ":f "
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        ":attr='foo'/>\n"
        "</"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        ":e>";
  int i;
  const int max_alloc_count = 40;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text1, (int)strlen(text1), XML_FALSE)
            != XML_STATUS_ERROR
        && _XML_Parse_SINGLE_BYTES(g_parser, text2, (int)strlen(text2),
                                   XML_TRUE)
               != XML_STATUS_ERROR)
      break;
    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max allocation count");
}
END_TEST

/* Using a slightly shorter namespace name provokes allocations in
 * slightly different places in the code.
 */
START_TEST(test_nsalloc_less_long_namespace) {
  const char *text
      = "<"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345678"
        ":e xmlns:"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345678"
        "='http://example.org/'>\n"
        "<"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345678"
        ":f "
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345678"
        ":att='foo'/>\n"
        "</"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AZ"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345678"
        ":e>";
  int i;
  const int max_alloc_count = 40;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max allocation count");
}
END_TEST

START_TEST(test_nsalloc_long_context) {
  const char *text
      = "<!DOCTYPE doc SYSTEM 'foo' [\n"
        "  <!ATTLIST doc baz ID #REQUIRED>\n"
        "  <!ENTITY en SYSTEM 'bar'>\n"
        "]>\n"
        "<doc xmlns='http://example.org/"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKL"
        "' baz='2'>\n"
        "&en;"
        "</doc>";
  ExtOption options[] = {
      {XCS("foo"), "<!ELEMENT e EMPTY>"}, {XCS("bar"), "<e/>"}, {NULL, NULL}};
  int i;
  const int max_alloc_count = 70;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetUserData(g_parser, options);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_optioner);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;

    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max allocation count");
}
END_TEST

/* This function is void; it will throw a fail() on error, so if it
 * returns normally it must have succeeded.
 */
static void
context_realloc_test(const char *text) {
  ExtOption options[] = {
      {XCS("foo"), "<!ELEMENT e EMPTY>"}, {XCS("bar"), "<e/>"}, {NULL, NULL}};
  int i;
  const int max_realloc_count = 6;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    XML_SetUserData(g_parser, options);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_optioner);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing reallocations");
  else if (i == max_realloc_count)
    fail("Parsing failed even at max reallocation count");
}

START_TEST(test_nsalloc_realloc_long_context) {
  const char *text
      = "<!DOCTYPE doc SYSTEM 'foo' [\n"
        "  <!ENTITY en SYSTEM 'bar'>\n"
        "]>\n"
        "<doc xmlns='http://example.org/"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKL"
        "'>\n"
        "&en;"
        "</doc>";

  context_realloc_test(text);
}
END_TEST

START_TEST(test_nsalloc_realloc_long_context_2) {
  const char *text
      = "<!DOCTYPE doc SYSTEM 'foo' [\n"
        "  <!ENTITY en SYSTEM 'bar'>\n"
        "]>\n"
        "<doc xmlns='http://example.org/"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJK"
        "'>\n"
        "&en;"
        "</doc>";

  context_realloc_test(text);
}
END_TEST

START_TEST(test_nsalloc_realloc_long_context_3) {
  const char *text
      = "<!DOCTYPE doc SYSTEM 'foo' [\n"
        "  <!ENTITY en SYSTEM 'bar'>\n"
        "]>\n"
        "<doc xmlns='http://example.org/"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGH"
        "'>\n"
        "&en;"
        "</doc>";

  context_realloc_test(text);
}
END_TEST

START_TEST(test_nsalloc_realloc_long_context_4) {
  const char *text
      = "<!DOCTYPE doc SYSTEM 'foo' [\n"
        "  <!ENTITY en SYSTEM 'bar'>\n"
        "]>\n"
        "<doc xmlns='http://example.org/"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO"
        "'>\n"
        "&en;"
        "</doc>";

  context_realloc_test(text);
}
END_TEST

START_TEST(test_nsalloc_realloc_long_context_5) {
  const char *text
      = "<!DOCTYPE doc SYSTEM 'foo' [\n"
        "  <!ENTITY en SYSTEM 'bar'>\n"
        "]>\n"
        "<doc xmlns='http://example.org/"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABC"
        "'>\n"
        "&en;"
        "</doc>";

  context_realloc_test(text);
}
END_TEST

START_TEST(test_nsalloc_realloc_long_context_6) {
  const char *text
      = "<!DOCTYPE doc SYSTEM 'foo' [\n"
        "  <!ENTITY en SYSTEM 'bar'>\n"
        "]>\n"
        "<doc xmlns='http://example.org/"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNOP"
        "'>\n"
        "&en;"
        "</doc>";

  context_realloc_test(text);
}
END_TEST

START_TEST(test_nsalloc_realloc_long_context_7) {
  const char *text
      = "<!DOCTYPE doc SYSTEM 'foo' [\n"
        "  <!ENTITY en SYSTEM 'bar'>\n"
        "]>\n"
        "<doc xmlns='http://example.org/"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLM"
        "'>\n"
        "&en;"
        "</doc>";

  context_realloc_test(text);
}
END_TEST

START_TEST(test_nsalloc_realloc_long_ge_name) {
  const char *text
      = "<!DOCTYPE doc SYSTEM 'foo' [\n"
        "  <!ENTITY "
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        " SYSTEM 'bar'>\n"
        "]>\n"
        "<doc xmlns='http://example.org/baz'>\n"
        "&"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        ";"
        "</doc>";
  ExtOption options[] = {
      {XCS("foo"), "<!ELEMENT el EMPTY>"}, {XCS("bar"), "<el/>"}, {NULL, NULL}};
  int i;
  const int max_realloc_count = 10;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    XML_SetUserData(g_parser, options);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_optioner);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;
    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing reallocations");
  else if (i == max_realloc_count)
    fail("Parsing failed even at max reallocation count");
}
END_TEST

/* Test that when a namespace is passed through the context mechanism
 * to an external entity parser, the parsers handle reallocation
 * failures correctly.  The prefix is exactly the right length to
 * provoke particular uncommon code paths.
 */
START_TEST(test_nsalloc_realloc_long_context_in_dtd) {
  const char *text1
      = "<!DOCTYPE "
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        ":doc [\n"
        "  <!ENTITY First SYSTEM 'foo/First'>\n"
        "]>\n"
        "<"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        ":doc xmlns:"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "='foo/Second'>&First;";
  const char *text2
      = "</"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        ":doc>";
  ExtOption options[] = {{XCS("foo/First"), "Hello world"}, {NULL, NULL}};
  int i;
  const int max_realloc_count = 20;

  for (i = 0; i < max_realloc_count; i++) {
    reallocation_count = i;
    XML_SetUserData(g_parser, options);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_optioner);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text1, (int)strlen(text1), XML_FALSE)
            != XML_STATUS_ERROR
        && _XML_Parse_SINGLE_BYTES(g_parser, text2, (int)strlen(text2),
                                   XML_TRUE)
               != XML_STATUS_ERROR)
      break;
    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing reallocations");
  else if (i == max_realloc_count)
    fail("Parsing failed even at max reallocation count");
}
END_TEST

START_TEST(test_nsalloc_long_default_in_ext) {
  const char *text
      = "<!DOCTYPE doc [\n"
        "  <!ATTLIST e a1 CDATA '"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
        "'>\n"
        "  <!ENTITY x SYSTEM 'foo'>\n"
        "]>\n"
        "<doc>&x;</doc>";
  ExtOption options[] = {{XCS("foo"), "<e/>"}, {NULL, NULL}};
  int i;
  const int max_alloc_count = 50;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetUserData(g_parser, options);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_optioner);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;

    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max allocation count");
}
END_TEST

START_TEST(test_nsalloc_long_systemid_in_ext) {
  const char *text
      = "<!DOCTYPE doc SYSTEM 'foo' [\n"
        "  <!ENTITY en SYSTEM '"
        /* 64 characters per line */
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"
        "'>\n"
        "]>\n"
        "<doc>&en;</doc>";
  ExtOption options[] = {
      {XCS("foo"), "<!ELEMENT e EMPTY>"},
      {/* clang-format off */
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/")
            XCS("ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/ABCDEFGHIJKLMNO/"),
       /* clang-format on */
       "<e/>"},
      {NULL, NULL}};
  int i;
  const int max_alloc_count = 55;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetUserData(g_parser, options);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_optioner);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;

    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Parsing worked despite failing allocations");
  else if (i == max_alloc_count)
    fail("Parsing failed even at max allocation count");
}
END_TEST

/* Test the effects of allocation failure on parsing an element in a
 * namespace.  Based on test_nsalloc_long_context.
 */
START_TEST(test_nsalloc_prefixed_element) {
  const char *text = "<!DOCTYPE pfx:element SYSTEM 'foo' [\n"
                     "  <!ATTLIST pfx:element baz ID #REQUIRED>\n"
                     "  <!ENTITY en SYSTEM 'bar'>\n"
                     "]>\n"
                     "<pfx:element xmlns:pfx='http://example.org/' baz='2'>\n"
                     "&en;"
                     "</pfx:element>";
  ExtOption options[] = {
      {XCS("foo"), "<!ELEMENT e EMPTY>"}, {XCS("bar"), "<e/>"}, {NULL, NULL}};
  int i;
  const int max_alloc_count = 70;

  for (i = 0; i < max_alloc_count; i++) {
    allocation_count = i;
    XML_SetUserData(g_parser, options);
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_optioner);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        != XML_STATUS_ERROR)
      break;

    /* See comment in test_nsalloc_xmlns() */
    nsalloc_teardown();
    nsalloc_setup();
  }
  if (i == 0)
    fail("Success despite failing allocator");
  else if (i == max_alloc_count)
    fail("Failed even at full allocation count");
}
END_TEST

#if defined(XML_DTD)
typedef enum XML_Status (*XmlParseFunction)(XML_Parser, const char *, int, int);

struct AccountingTestCase {
  const char *primaryText;
  const char *firstExternalText;  /* often NULL */
  const char *secondExternalText; /* often NULL */
  const unsigned long long expectedCountBytesIndirectExtra;
  XML_Bool singleBytesWanted;
};

static int
accounting_external_entity_ref_handler(XML_Parser parser,
                                       const XML_Char *context,
                                       const XML_Char *base,
                                       const XML_Char *systemId,
                                       const XML_Char *publicId) {
  UNUSED_P(context);
  UNUSED_P(base);
  UNUSED_P(publicId);

  const struct AccountingTestCase *const testCase
      = (const struct AccountingTestCase *)XML_GetUserData(parser);

  const char *externalText = NULL;
  if (xcstrcmp(systemId, XCS("first.ent")) == 0) {
    externalText = testCase->firstExternalText;
  } else if (xcstrcmp(systemId, XCS("second.ent")) == 0) {
    externalText = testCase->secondExternalText;
  } else {
    assert(! "systemId is neither \"first.ent\" nor \"second.ent\"");
  }
  assert(externalText);

  XML_Parser entParser = XML_ExternalEntityParserCreate(parser, context, 0);
  assert(entParser);

  const XmlParseFunction xmlParseFunction
      = testCase->singleBytesWanted ? _XML_Parse_SINGLE_BYTES : XML_Parse;

  const enum XML_Status status = xmlParseFunction(
      entParser, externalText, (int)strlen(externalText), XML_TRUE);

  XML_ParserFree(entParser);
  return status;
}

START_TEST(test_accounting_precision) {
  const XML_Bool filled_later = XML_TRUE; /* value is arbitrary */
  struct AccountingTestCase cases[] = {
      {"<e/>", NULL, NULL, 0, 0},
      {"<e></e>", NULL, NULL, 0, 0},

      /* Attributes */
      {"<e k1=\"v2\" k2=\"v2\"/>", NULL, NULL, 0, filled_later},
      {"<e k1=\"v2\" k2=\"v2\"></e>", NULL, NULL, 0, 0},
      {"<p:e xmlns:p=\"https://domain.invalid/\" />", NULL, NULL, 0,
       filled_later},
      {"<e k=\"&amp;&apos;&gt;&lt;&quot;\" />", NULL, NULL,
       sizeof(XML_Char) * 5 /* number of predefined entities */, filled_later},
      {"<e1 xmlns='https://example.org/'>\n"
       "  <e2 xmlns=''/>\n"
       "</e1>",
       NULL, NULL, 0, filled_later},

      /* Text */
      {"<e>text</e>", NULL, NULL, 0, filled_later},
      {"<e1><e2>text1<e3/>text2</e2></e1>", NULL, NULL, 0, filled_later},
      {"<e>&amp;&apos;&gt;&lt;&quot;</e>", NULL, NULL,
       sizeof(XML_Char) * 5 /* number of predefined entities */, filled_later},
      {"<e>&#65;&#41;</e>", NULL, NULL, 0, filled_later},

      /* Prolog */
      {"<?xml version=\"1.0\"?><root/>", NULL, NULL, 0, filled_later},

      /* Whitespace */
      {"  <e1>  <e2>  </e2>  </e1>  ", NULL, NULL, 0, filled_later},
      {"<e1  ><e2  /></e1  >", NULL, NULL, 0, filled_later},
      {"<e1><e2 k = \"v\"/><e3 k = 'v'/></e1>", NULL, NULL, 0, filled_later},

      /* Comments */
      {"<!-- Comment --><e><!-- Comment --></e>", NULL, NULL, 0, filled_later},

      /* Processing instructions */
      {"<?xml-stylesheet type=\"text/xsl\" href=\"https://domain.invalid/\" media=\"all\"?><e/>",
       NULL, NULL, 0, filled_later},
      {"<?pi0?><?pi1 ?><?pi2  ?><!DOCTYPE r SYSTEM 'first.ent'><r/>",
       "<?pi3?><!ENTITY % e1 SYSTEM 'second.ent'><?pi4?>%e1;<?pi5?>", "<?pi6?>",
       0, filled_later},

      /* CDATA */
      {"<e><![CDATA[one two three]]></e>", NULL, NULL, 0, filled_later},
      /* The following is the essence of this OSS-Fuzz finding:
         https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=34302
         https://oss-fuzz.com/testcase-detail/4860575394955264
      */
      {"<!DOCTYPE r [\n"
       "<!ENTITY e \"111<![CDATA[2 <= 2]]>333\">\n"
       "]>\n"
       "<r>&e;</r>\n",
       NULL, NULL, sizeof(XML_Char) * strlen("111<![CDATA[2 <= 2]]>333"),
       filled_later},

      /* Conditional sections */
      {"<!DOCTYPE r [\n"
       "<!ENTITY % draft 'INCLUDE'>\n"
       "<!ENTITY % final 'IGNORE'>\n"
       "<!ENTITY % import SYSTEM \"first.ent\">\n"
       "%import;\n"
       "]>\n"
       "<r/>\n",
       "<![%draft;[<!--1-->]]>\n"
       "<![%final;[<!--22-->]]>",
       NULL, sizeof(XML_Char) * (strlen("INCLUDE") + strlen("IGNORE")),
       filled_later},

      /* General entities */
      {"<!DOCTYPE root [\n"
       "<!ENTITY nine \"123456789\">\n"
       "]>\n"
       "<root>&nine;</root>",
       NULL, NULL, sizeof(XML_Char) * strlen("123456789"), filled_later},
      {"<!DOCTYPE root [\n"
       "<!ENTITY nine \"123456789\">\n"
       "]>\n"
       "<root k1=\"&nine;\"/>",
       NULL, NULL, sizeof(XML_Char) * strlen("123456789"), filled_later},
      {"<!DOCTYPE root [\n"
       "<!ENTITY nine \"123456789\">\n"
       "<!ENTITY nine2 \"&nine;&nine;\">\n"
       "]>\n"
       "<root>&nine2;&nine2;&nine2;</root>",
       NULL, NULL,
       sizeof(XML_Char) * 3 /* calls to &nine2; */ * 2 /* calls to &nine; */
           * (strlen("&nine;") + strlen("123456789")),
       filled_later},
      {"<!DOCTYPE r [\n"
       "  <!ENTITY five SYSTEM 'first.ent'>\n"
       "]>\n"
       "<r>&five;</r>",
       "12345", NULL, 0, filled_later},

      /* Parameter entities */
      {"<!DOCTYPE r [\n"
       "<!ENTITY % comment \"<!---->\">\n"
       "%comment;\n"
       "]>\n"
       "<r/>",
       NULL, NULL, sizeof(XML_Char) * strlen("<!---->"), filled_later},
      {"<!DOCTYPE r [\n"
       "<!ENTITY % ninedef \"&#60;!ENTITY nine &#34;123456789&#34;&#62;\">\n"
       "%ninedef;\n"
       "]>\n"
       "<r>&nine;</r>",
       NULL, NULL,
       sizeof(XML_Char)
           * (strlen("<!ENTITY nine \"123456789\">") + strlen("123456789")),
       filled_later},
      {"<!DOCTYPE r [\n"
       "<!ENTITY % comment \"<!--1-->\">\n"
       "<!ENTITY % comment2 \"&#37;comment;<!--22-->&#37;comment;\">\n"
       "%comment2;\n"
       "]>\n"
       "<r/>\n",
       NULL, NULL,
       sizeof(XML_Char)
           * (strlen("%comment;<!--22-->%comment;") + 2 * strlen("<!--1-->")),
       filled_later},
      {"<!DOCTYPE r [\n"
       "  <!ENTITY % five \"12345\">\n"
       "  <!ENTITY % five2def \"&#60;!ENTITY five2 &#34;[&#37;five;][&#37;five;]]]]&#34;&#62;\">\n"
       "  %five2def;\n"
       "]>\n"
       "<r>&five2;</r>",
       NULL, NULL, /* from "%five2def;": */
       sizeof(XML_Char)
           * (strlen("<!ENTITY five2 \"[%five;][%five;]]]]\">")
              + 2 /* calls to "%five;" */ * strlen("12345")
              + /* from "&five2;": */ strlen("[12345][12345]]]]")),
       filled_later},
      {"<!DOCTYPE r SYSTEM \"first.ent\">\n"
       "<r/>",
       "<!ENTITY % comment '<!--1-->'>\n"
       "<!ENTITY % comment2 '<!--22-->%comment;<!--22-->%comment;<!--22-->'>\n"
       "%comment2;",
       NULL,
       sizeof(XML_Char)
           * (strlen("<!--22-->%comment;<!--22-->%comment;<!--22-->")
              + 2 /* calls to "%comment;" */ * strlen("<!---->")),
       filled_later},
      {"<!DOCTYPE r SYSTEM 'first.ent'>\n"
       "<r/>",
       "<!ENTITY % e1 PUBLIC 'foo' 'second.ent'>\n"
       "<!ENTITY % e2 '<!--22-->%e1;<!--22-->'>\n"
       "%e2;\n",
       "<!--1-->", sizeof(XML_Char) * strlen("<!--22--><!--1--><!--22-->"),
       filled_later},
      {
          "<!DOCTYPE r SYSTEM 'first.ent'>\n"
          "<r/>",
          "<!ENTITY % e1 SYSTEM 'second.ent'>\n"
          "<!ENTITY % e2 '%e1;'>",
          "<?xml version='1.0' encoding='utf-8'?>\n"
          "hello\n"
          "xml" /* without trailing newline! */,
          0,
          filled_later,
      },
      {
          "<!DOCTYPE r SYSTEM 'first.ent'>\n"
          "<r/>",
          "<!ENTITY % e1 SYSTEM 'second.ent'>\n"
          "<!ENTITY % e2 '%e1;'>",
          "<?xml version='1.0' encoding='utf-8'?>\n"
          "hello\n"
          "xml\n" /* with trailing newline! */,
          0,
          filled_later,
      },
      {"<!DOCTYPE doc SYSTEM 'first.ent'>\n"
       "<doc></doc>\n",
       "<!ELEMENT doc EMPTY>\n"
       "<!ENTITY % e1 SYSTEM 'second.ent'>\n"
       "<!ENTITY % e2 '%e1;'>\n"
       "%e1;\n",
       "\xEF\xBB\xBF<!ATTLIST doc a1 CDATA 'value'>" /* UTF-8 BOM */,
       strlen("\xEF\xBB\xBF<!ATTLIST doc a1 CDATA 'value'>"), filled_later},
      {"<!DOCTYPE r [\n"
       "  <!ENTITY five SYSTEM 'first.ent'>\n"
       "]>\n"
       "<r>&five;</r>",
       "\xEF\xBB\xBF" /* UTF-8 BOM */, NULL, 0, filled_later},
  };

  const size_t countCases = sizeof(cases) / sizeof(cases[0]);
  size_t u = 0;
  for (; u < countCases; u++) {
    size_t v = 0;
    for (; v < 2; v++) {
      const XML_Bool singleBytesWanted = (v == 0) ? XML_FALSE : XML_TRUE;
      const unsigned long long expectedCountBytesDirect
          = strlen(cases[u].primaryText);
      const unsigned long long expectedCountBytesIndirect
          = (cases[u].firstExternalText ? strlen(cases[u].firstExternalText)
                                        : 0)
            + (cases[u].secondExternalText ? strlen(cases[u].secondExternalText)
                                           : 0)
            + cases[u].expectedCountBytesIndirectExtra;

      XML_Parser parser = XML_ParserCreate(NULL);
      XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
      if (cases[u].firstExternalText) {
        XML_SetExternalEntityRefHandler(parser,
                                        accounting_external_entity_ref_handler);
        XML_SetUserData(parser, (void *)&cases[u]);
        cases[u].singleBytesWanted = singleBytesWanted;
      }

      const XmlParseFunction xmlParseFunction
          = singleBytesWanted ? _XML_Parse_SINGLE_BYTES : XML_Parse;

      enum XML_Status status
          = xmlParseFunction(parser, cases[u].primaryText,
                             (int)strlen(cases[u].primaryText), XML_TRUE);
      if (status != XML_STATUS_OK) {
        _xml_failure(parser, __FILE__, __LINE__);
      }

      const unsigned long long actualCountBytesDirect
          = testingAccountingGetCountBytesDirect(parser);
      const unsigned long long actualCountBytesIndirect
          = testingAccountingGetCountBytesIndirect(parser);

      XML_ParserFree(parser);

      if (actualCountBytesDirect != expectedCountBytesDirect) {
        fprintf(
            stderr,
            "Document " EXPAT_FMT_SIZE_T("") " of " EXPAT_FMT_SIZE_T("") ", %s: Expected " EXPAT_FMT_ULL(
                "") " count direct bytes, got " EXPAT_FMT_ULL("") " instead.\n",
            u + 1, countCases, singleBytesWanted ? "single bytes" : "chunks",
            expectedCountBytesDirect, actualCountBytesDirect);
        fail("Count of direct bytes is off");
      }

      if (actualCountBytesIndirect != expectedCountBytesIndirect) {
        fprintf(
            stderr,
            "Document " EXPAT_FMT_SIZE_T("") " of " EXPAT_FMT_SIZE_T("") ", %s: Expected " EXPAT_FMT_ULL(
                "") " count indirect bytes, got " EXPAT_FMT_ULL("") " instead.\n",
            u + 1, countCases, singleBytesWanted ? "single bytes" : "chunks",
            expectedCountBytesIndirect, actualCountBytesIndirect);
        fail("Count of indirect bytes is off");
      }
    }
  }
}
END_TEST

static float
portableNAN(void) {
  return strtof("nan", NULL);
}

static float
portableINFINITY(void) {
  return strtof("infinity", NULL);
}

START_TEST(test_billion_laughs_attack_protection_api) {
  XML_Parser parserWithoutParent = XML_ParserCreate(NULL);
  XML_Parser parserWithParent
      = XML_ExternalEntityParserCreate(parserWithoutParent, NULL, NULL);
  if (parserWithoutParent == NULL)
    fail("parserWithoutParent is NULL");
  if (parserWithParent == NULL)
    fail("parserWithParent is NULL");

  // XML_SetBillionLaughsAttackProtectionMaximumAmplification, error cases
  if (XML_SetBillionLaughsAttackProtectionMaximumAmplification(NULL, 123.0f)
      == XML_TRUE)
    fail("Call with NULL parser is NOT supposed to succeed");
  if (XML_SetBillionLaughsAttackProtectionMaximumAmplification(parserWithParent,
                                                               123.0f)
      == XML_TRUE)
    fail("Call with non-root parser is NOT supposed to succeed");
  if (XML_SetBillionLaughsAttackProtectionMaximumAmplification(
          parserWithoutParent, portableNAN())
      == XML_TRUE)
    fail("Call with NaN limit is NOT supposed to succeed");
  if (XML_SetBillionLaughsAttackProtectionMaximumAmplification(
          parserWithoutParent, -1.0f)
      == XML_TRUE)
    fail("Call with negative limit is NOT supposed to succeed");
  if (XML_SetBillionLaughsAttackProtectionMaximumAmplification(
          parserWithoutParent, 0.9f)
      == XML_TRUE)
    fail("Call with positive limit <1.0 is NOT supposed to succeed");

  // XML_SetBillionLaughsAttackProtectionMaximumAmplification, success cases
  if (XML_SetBillionLaughsAttackProtectionMaximumAmplification(
          parserWithoutParent, 1.0f)
      == XML_FALSE)
    fail("Call with positive limit >=1.0 is supposed to succeed");
  if (XML_SetBillionLaughsAttackProtectionMaximumAmplification(
          parserWithoutParent, 123456.789f)
      == XML_FALSE)
    fail("Call with positive limit >=1.0 is supposed to succeed");
  if (XML_SetBillionLaughsAttackProtectionMaximumAmplification(
          parserWithoutParent, portableINFINITY())
      == XML_FALSE)
    fail("Call with positive limit >=1.0 is supposed to succeed");

  // XML_SetBillionLaughsAttackProtectionActivationThreshold, error cases
  if (XML_SetBillionLaughsAttackProtectionActivationThreshold(NULL, 123)
      == XML_TRUE)
    fail("Call with NULL parser is NOT supposed to succeed");
  if (XML_SetBillionLaughsAttackProtectionActivationThreshold(parserWithParent,
                                                              123)
      == XML_TRUE)
    fail("Call with non-root parser is NOT supposed to succeed");

  // XML_SetBillionLaughsAttackProtectionActivationThreshold, success cases
  if (XML_SetBillionLaughsAttackProtectionActivationThreshold(
          parserWithoutParent, 123)
      == XML_FALSE)
    fail("Call with non-NULL parentless parser is supposed to succeed");

  XML_ParserFree(parserWithParent);
  XML_ParserFree(parserWithoutParent);
}
END_TEST

START_TEST(test_helper_unsigned_char_to_printable) {
  // Smoke test
  unsigned char uc = 0;
  for (; uc < (unsigned char)-1; uc++) {
    const char *const printable = unsignedCharToPrintable(uc);
    if (printable == NULL)
      fail("unsignedCharToPrintable returned NULL");
    if (strlen(printable) < (size_t)1)
      fail("unsignedCharToPrintable returned empty string");
  }

  // Two concrete samples
  if (strcmp(unsignedCharToPrintable('A'), "A") != 0)
    fail("unsignedCharToPrintable result mistaken");
  if (strcmp(unsignedCharToPrintable('\\'), "\\\\") != 0)
    fail("unsignedCharToPrintable result mistaken");
}
END_TEST
#endif // defined(XML_DTD)

static Suite *
make_suite(void) {
  Suite *s = suite_create("basic");
  TCase *tc_basic = make_basic_test_case(s);
  TCase *tc_namespace = tcase_create("XML namespaces");
  TCase *tc_misc = tcase_create("miscellaneous tests");
  TCase *tc_alloc = tcase_create("allocation tests");
  TCase *tc_nsalloc = tcase_create("namespace allocation tests");
#if defined(XML_DTD)
  TCase *tc_accounting = tcase_create("accounting tests");
#endif

  tcase_add_test__ifdef_xml_dtd(tc_basic, test_ext_entity_not_standalone);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_ext_entity_value_abort);
  tcase_add_test(tc_basic, test_bad_public_doctype);
  tcase_add_test(tc_basic, test_attribute_enum_value);
  tcase_add_test(tc_basic, test_predefined_entity_redefinition);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_dtd_stop_processing);
  tcase_add_test(tc_basic, test_public_notation_no_sysid);
  tcase_add_test(tc_basic, test_nested_groups);
  tcase_add_test(tc_basic, test_group_choice);
  tcase_add_test(tc_basic, test_standalone_parameter_entity);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_skipped_parameter_entity);
  tcase_add_test__ifdef_xml_dtd(tc_basic,
                                test_recursive_external_parameter_entity);
  tcase_add_test(tc_basic, test_undefined_ext_entity_in_external_dtd);
  tcase_add_test(tc_basic, test_suspend_xdecl);
  tcase_add_test(tc_basic, test_abort_epilog);
  tcase_add_test(tc_basic, test_abort_epilog_2);
  tcase_add_test(tc_basic, test_suspend_epilog);
  tcase_add_test(tc_basic, test_suspend_in_sole_empty_tag);
  tcase_add_test(tc_basic, test_unfinished_epilog);
  tcase_add_test(tc_basic, test_partial_char_in_epilog);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_suspend_resume_internal_entity);
  tcase_add_test__ifdef_xml_dtd(tc_basic,
                                test_suspend_resume_internal_entity_issue_629);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_resume_entity_with_syntax_error);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_suspend_resume_parameter_entity);
  tcase_add_test(tc_basic, test_restart_on_error);
  tcase_add_test(tc_basic, test_reject_lt_in_attribute_value);
  tcase_add_test(tc_basic, test_reject_unfinished_param_in_att_value);
  tcase_add_test(tc_basic, test_trailing_cr_in_att_value);
  tcase_add_test(tc_basic, test_standalone_internal_entity);
  tcase_add_test(tc_basic, test_skipped_external_entity);
  tcase_add_test(tc_basic, test_skipped_null_loaded_ext_entity);
  tcase_add_test(tc_basic, test_skipped_unloaded_ext_entity);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_param_entity_with_trailing_cr);
  tcase_add_test(tc_basic, test_invalid_character_entity);
  tcase_add_test(tc_basic, test_invalid_character_entity_2);
  tcase_add_test(tc_basic, test_invalid_character_entity_3);
  tcase_add_test(tc_basic, test_invalid_character_entity_4);
  tcase_add_test(tc_basic, test_pi_handled_in_default);
  tcase_add_test(tc_basic, test_comment_handled_in_default);
  tcase_add_test(tc_basic, test_pi_yml);
  tcase_add_test(tc_basic, test_pi_xnl);
  tcase_add_test(tc_basic, test_pi_xmm);
  tcase_add_test(tc_basic, test_utf16_pi);
  tcase_add_test(tc_basic, test_utf16_be_pi);
  tcase_add_test(tc_basic, test_utf16_be_comment);
  tcase_add_test(tc_basic, test_utf16_le_comment);
  tcase_add_test(tc_basic, test_missing_encoding_conversion_fn);
  tcase_add_test(tc_basic, test_failing_encoding_conversion_fn);
  tcase_add_test(tc_basic, test_unknown_encoding_success);
  tcase_add_test(tc_basic, test_unknown_encoding_bad_name);
  tcase_add_test(tc_basic, test_unknown_encoding_bad_name_2);
  tcase_add_test(tc_basic, test_unknown_encoding_long_name_1);
  tcase_add_test(tc_basic, test_unknown_encoding_long_name_2);
  tcase_add_test(tc_basic, test_invalid_unknown_encoding);
  tcase_add_test(tc_basic, test_unknown_ascii_encoding_ok);
  tcase_add_test(tc_basic, test_unknown_ascii_encoding_fail);
  tcase_add_test(tc_basic, test_unknown_encoding_invalid_length);
  tcase_add_test(tc_basic, test_unknown_encoding_invalid_topbit);
  tcase_add_test(tc_basic, test_unknown_encoding_invalid_surrogate);
  tcase_add_test(tc_basic, test_unknown_encoding_invalid_high);
  tcase_add_test(tc_basic, test_unknown_encoding_invalid_attr_value);
  tcase_add_test(tc_basic, test_ext_entity_latin1_utf16le_bom);
  tcase_add_test(tc_basic, test_ext_entity_latin1_utf16be_bom);
  tcase_add_test(tc_basic, test_ext_entity_latin1_utf16le_bom2);
  tcase_add_test(tc_basic, test_ext_entity_latin1_utf16be_bom2);
  tcase_add_test(tc_basic, test_ext_entity_utf16_be);
  tcase_add_test(tc_basic, test_ext_entity_utf16_le);
  tcase_add_test(tc_basic, test_ext_entity_utf16_unknown);
  tcase_add_test(tc_basic, test_ext_entity_utf8_non_bom);
  tcase_add_test(tc_basic, test_utf8_in_cdata_section);
  tcase_add_test(tc_basic, test_utf8_in_cdata_section_2);
  tcase_add_test(tc_basic, test_utf8_in_start_tags);
  tcase_add_test(tc_basic, test_trailing_spaces_in_elements);
  tcase_add_test(tc_basic, test_utf16_attribute);
  tcase_add_test(tc_basic, test_utf16_second_attr);
  tcase_add_test(tc_basic, test_attr_after_solidus);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_utf16_pe);
  tcase_add_test(tc_basic, test_bad_attr_desc_keyword);
  tcase_add_test(tc_basic, test_bad_attr_desc_keyword_utf16);
  tcase_add_test(tc_basic, test_bad_doctype);
  tcase_add_test(tc_basic, test_bad_doctype_utf8);
  tcase_add_test(tc_basic, test_bad_doctype_utf16);
  tcase_add_test(tc_basic, test_bad_doctype_plus);
  tcase_add_test(tc_basic, test_bad_doctype_star);
  tcase_add_test(tc_basic, test_bad_doctype_query);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_unknown_encoding_bad_ignore);
  tcase_add_test(tc_basic, test_entity_in_utf16_be_attr);
  tcase_add_test(tc_basic, test_entity_in_utf16_le_attr);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_entity_public_utf16_be);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_entity_public_utf16_le);
  tcase_add_test(tc_basic, test_short_doctype);
  tcase_add_test(tc_basic, test_short_doctype_2);
  tcase_add_test(tc_basic, test_short_doctype_3);
  tcase_add_test(tc_basic, test_long_doctype);
  tcase_add_test(tc_basic, test_bad_entity);
  tcase_add_test(tc_basic, test_bad_entity_2);
  tcase_add_test(tc_basic, test_bad_entity_3);
  tcase_add_test(tc_basic, test_bad_entity_4);
  tcase_add_test(tc_basic, test_bad_notation);
  tcase_add_test(tc_basic, test_default_doctype_handler);
  tcase_add_test(tc_basic, test_empty_element_abort);
  tcase_add_test__ifdef_xml_dtd(tc_basic,
                                test_pool_integrity_with_unfinished_attr);
  tcase_add_test(tc_basic, test_nested_entity_suspend);

  suite_add_tcase(s, tc_namespace);
  tcase_add_checked_fixture(tc_namespace, namespace_setup, namespace_teardown);
  tcase_add_test(tc_namespace, test_return_ns_triplet);
  tcase_add_test(tc_namespace, test_ns_tagname_overwrite);
  tcase_add_test(tc_namespace, test_ns_tagname_overwrite_triplet);
  tcase_add_test(tc_namespace, test_start_ns_clears_start_element);
  tcase_add_test__ifdef_xml_dtd(tc_namespace,
                                test_default_ns_from_ext_subset_and_ext_ge);
  tcase_add_test(tc_namespace, test_ns_prefix_with_empty_uri_1);
  tcase_add_test(tc_namespace, test_ns_prefix_with_empty_uri_2);
  tcase_add_test(tc_namespace, test_ns_prefix_with_empty_uri_3);
  tcase_add_test(tc_namespace, test_ns_prefix_with_empty_uri_4);
  tcase_add_test(tc_namespace, test_ns_unbound_prefix);
  tcase_add_test(tc_namespace, test_ns_default_with_empty_uri);
  tcase_add_test(tc_namespace, test_ns_duplicate_attrs_diff_prefixes);
  tcase_add_test(tc_namespace, test_ns_duplicate_hashes);
  tcase_add_test(tc_namespace, test_ns_unbound_prefix_on_attribute);
  tcase_add_test(tc_namespace, test_ns_unbound_prefix_on_element);
  tcase_add_test(tc_namespace, test_ns_parser_reset);
  tcase_add_test(tc_namespace, test_ns_long_element);
  tcase_add_test(tc_namespace, test_ns_mixed_prefix_atts);
  tcase_add_test(tc_namespace, test_ns_extend_uri_buffer);
  tcase_add_test(tc_namespace, test_ns_reserved_attributes);
  tcase_add_test(tc_namespace, test_ns_reserved_attributes_2);
  tcase_add_test(tc_namespace, test_ns_extremely_long_prefix);
  tcase_add_test(tc_namespace, test_ns_unknown_encoding_success);
  tcase_add_test(tc_namespace, test_ns_double_colon);
  tcase_add_test(tc_namespace, test_ns_double_colon_element);
  tcase_add_test(tc_namespace, test_ns_bad_attr_leafname);
  tcase_add_test(tc_namespace, test_ns_bad_element_leafname);
  tcase_add_test(tc_namespace, test_ns_utf16_leafname);
  tcase_add_test(tc_namespace, test_ns_utf16_element_leafname);
  tcase_add_test(tc_namespace, test_ns_utf16_doctype);
  tcase_add_test(tc_namespace, test_ns_invalid_doctype);
  tcase_add_test(tc_namespace, test_ns_double_colon_doctype);
  tcase_add_test(tc_namespace, test_ns_separator_in_uri);

  suite_add_tcase(s, tc_misc);
  tcase_add_checked_fixture(tc_misc, NULL, basic_teardown);
  tcase_add_test(tc_misc, test_misc_alloc_create_parser);
  tcase_add_test(tc_misc, test_misc_alloc_create_parser_with_encoding);
  tcase_add_test(tc_misc, test_misc_null_parser);
  tcase_add_test(tc_misc, test_misc_error_string);
  tcase_add_test(tc_misc, test_misc_version);
  tcase_add_test(tc_misc, test_misc_features);
  tcase_add_test(tc_misc, test_misc_attribute_leak);
  tcase_add_test(tc_misc, test_misc_utf16le);
  tcase_add_test(tc_misc, test_misc_stop_during_end_handler_issue_240_1);
  tcase_add_test(tc_misc, test_misc_stop_during_end_handler_issue_240_2);
  tcase_add_test__ifdef_xml_dtd(
      tc_misc, test_misc_deny_internal_entity_closing_doctype_issue_317);
  tcase_add_test(tc_misc, test_misc_tag_mismatch_reset_leak);

  suite_add_tcase(s, tc_alloc);
  tcase_add_checked_fixture(tc_alloc, alloc_setup, alloc_teardown);
  tcase_add_test(tc_alloc, test_alloc_parse_xdecl);
  tcase_add_test(tc_alloc, test_alloc_parse_xdecl_2);
  tcase_add_test(tc_alloc, test_alloc_parse_pi);
  tcase_add_test(tc_alloc, test_alloc_parse_pi_2);
  tcase_add_test(tc_alloc, test_alloc_parse_pi_3);
  tcase_add_test(tc_alloc, test_alloc_parse_comment);
  tcase_add_test(tc_alloc, test_alloc_parse_comment_2);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_create_external_parser);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_run_external_parser);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_dtd_copy_default_atts);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_external_entity);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_ext_entity_set_encoding);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_internal_entity);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_dtd_default_handling);
  tcase_add_test(tc_alloc, test_alloc_explicit_encoding);
  tcase_add_test(tc_alloc, test_alloc_set_base);
  tcase_add_test(tc_alloc, test_alloc_realloc_buffer);
  tcase_add_test(tc_alloc, test_alloc_ext_entity_realloc_buffer);
  tcase_add_test(tc_alloc, test_alloc_realloc_many_attributes);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_public_entity_value);
  tcase_add_test__ifdef_xml_dtd(tc_alloc,
                                test_alloc_realloc_subst_public_entity_value);
  tcase_add_test(tc_alloc, test_alloc_parse_public_doctype);
  tcase_add_test(tc_alloc, test_alloc_parse_public_doctype_long_name);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_set_foreign_dtd);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_attribute_enum_value);
  tcase_add_test__ifdef_xml_dtd(tc_alloc,
                                test_alloc_realloc_attribute_enum_value);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_realloc_implied_attribute);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_realloc_default_attribute);
  tcase_add_test(tc_alloc, test_alloc_notation);
  tcase_add_test(tc_alloc, test_alloc_public_notation);
  tcase_add_test(tc_alloc, test_alloc_system_notation);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_nested_groups);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_realloc_nested_groups);
  tcase_add_test(tc_alloc, test_alloc_large_group);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_realloc_group_choice);
  tcase_add_test(tc_alloc, test_alloc_pi_in_epilog);
  tcase_add_test(tc_alloc, test_alloc_comment_in_epilog);
  tcase_add_test__ifdef_xml_dtd(tc_alloc,
                                test_alloc_realloc_long_attribute_value);
  tcase_add_test(tc_alloc, test_alloc_attribute_whitespace);
  tcase_add_test(tc_alloc, test_alloc_attribute_predefined_entity);
  tcase_add_test(tc_alloc, test_alloc_long_attr_default_with_char_ref);
  tcase_add_test(tc_alloc, test_alloc_long_attr_value);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_nested_entities);
  tcase_add_test__ifdef_xml_dtd(tc_alloc,
                                test_alloc_realloc_param_entity_newline);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_realloc_ce_extends_pe);
  tcase_add_test__ifdef_xml_dtd(tc_alloc, test_alloc_realloc_attributes);
  tcase_add_test(tc_alloc, test_alloc_long_doc_name);
  tcase_add_test(tc_alloc, test_alloc_long_base);
  tcase_add_test(tc_alloc, test_alloc_long_public_id);
  tcase_add_test(tc_alloc, test_alloc_long_entity_value);
  tcase_add_test(tc_alloc, test_alloc_long_notation);
  tcase_add_test__ifdef_xml_dtd(
      tc_alloc, test_alloc_reset_after_external_entity_parser_create_fail);

  suite_add_tcase(s, tc_nsalloc);
  tcase_add_checked_fixture(tc_nsalloc, nsalloc_setup, nsalloc_teardown);
  tcase_add_test(tc_nsalloc, test_nsalloc_xmlns);
  tcase_add_test(tc_nsalloc, test_nsalloc_parse_buffer);
  tcase_add_test(tc_nsalloc, test_nsalloc_long_prefix);
  tcase_add_test(tc_nsalloc, test_nsalloc_long_uri);
  tcase_add_test(tc_nsalloc, test_nsalloc_long_attr);
  tcase_add_test(tc_nsalloc, test_nsalloc_long_attr_prefix);
  tcase_add_test(tc_nsalloc, test_nsalloc_realloc_attributes);
  tcase_add_test(tc_nsalloc, test_nsalloc_long_element);
  tcase_add_test(tc_nsalloc, test_nsalloc_realloc_binding_uri);
  tcase_add_test(tc_nsalloc, test_nsalloc_realloc_long_prefix);
  tcase_add_test(tc_nsalloc, test_nsalloc_realloc_longer_prefix);
  tcase_add_test(tc_nsalloc, test_nsalloc_long_namespace);
  tcase_add_test(tc_nsalloc, test_nsalloc_less_long_namespace);
  tcase_add_test(tc_nsalloc, test_nsalloc_long_context);
  tcase_add_test(tc_nsalloc, test_nsalloc_realloc_long_context);
  tcase_add_test(tc_nsalloc, test_nsalloc_realloc_long_context_2);
  tcase_add_test(tc_nsalloc, test_nsalloc_realloc_long_context_3);
  tcase_add_test(tc_nsalloc, test_nsalloc_realloc_long_context_4);
  tcase_add_test(tc_nsalloc, test_nsalloc_realloc_long_context_5);
  tcase_add_test(tc_nsalloc, test_nsalloc_realloc_long_context_6);
  tcase_add_test(tc_nsalloc, test_nsalloc_realloc_long_context_7);
  tcase_add_test(tc_nsalloc, test_nsalloc_realloc_long_ge_name);
  tcase_add_test(tc_nsalloc, test_nsalloc_realloc_long_context_in_dtd);
  tcase_add_test(tc_nsalloc, test_nsalloc_long_default_in_ext);
  tcase_add_test(tc_nsalloc, test_nsalloc_long_systemid_in_ext);
  tcase_add_test(tc_nsalloc, test_nsalloc_prefixed_element);

#if defined(XML_DTD)
  suite_add_tcase(s, tc_accounting);
  tcase_add_test(tc_accounting, test_accounting_precision);
  tcase_add_test(tc_accounting, test_billion_laughs_attack_protection_api);
  tcase_add_test(tc_accounting, test_helper_unsigned_char_to_printable);
#endif

  return s;
}

int
main(int argc, char *argv[]) {
  int i, nf;
  int verbosity = CK_NORMAL;
  Suite *s = make_suite();
  SRunner *sr = srunner_create(s);

  for (i = 1; i < argc; ++i) {
    char *opt = argv[i];
    if (strcmp(opt, "-v") == 0 || strcmp(opt, "--verbose") == 0)
      verbosity = CK_VERBOSE;
    else if (strcmp(opt, "-q") == 0 || strcmp(opt, "--quiet") == 0)
      verbosity = CK_SILENT;
    else {
      fprintf(stderr, "runtests: unknown option '%s'\n", opt);
      return 2;
    }
  }
  if (verbosity != CK_SILENT)
    printf("Expat version: %" XML_FMT_STR "\n", XML_ExpatVersion());
  srunner_run_all(sr, verbosity);
  nf = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
