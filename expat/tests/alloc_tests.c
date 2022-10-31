/* Tests in the "allocation" test case for the Expat test suite
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

#if defined(NDEBUG)
#  undef NDEBUG /* because test suite relies on assert(...) at the moment */
#endif

#include <string.h>
#include <assert.h>

#include "expat.h"
#include "common.h"
#include "minicheck.h"
#include "dummy.h"
#include "handlers.h"
#include "alloc_tests.h"

void
alloc_setup(void) {
  XML_Memory_Handling_Suite memsuite = {duff_allocator, duff_reallocator, free};

  /* Ensure the parser creation will go through */
  g_allocation_count = ALLOC_ALWAYS_SUCCEED;
  g_reallocation_count = REALLOC_ALWAYS_SUCCEED;
  g_parser = XML_ParserCreate_MM(NULL, &memsuite, NULL);
  if (g_parser == NULL)
    fail("Parser not created");
}

void
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
    g_allocation_count = i;
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
    g_allocation_count = i;
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
    g_allocation_count = i;
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
    g_allocation_count = i;
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
    g_allocation_count = i;
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
    g_allocation_count = i;
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
    g_allocation_count = i;
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
    g_allocation_count = i;
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
  int callno = 0;

  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_dbl_handler);
  XML_SetUserData(g_parser, &callno);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

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
  int callno = 0;

  for (i = 0; i < alloc_test_max_repeats; i++) {
    g_allocation_count = -1;
    XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetExternalEntityRefHandler(g_parser, external_entity_dbl_handler_2);
    callno = 0;
    XML_SetUserData(g_parser, &callno);
    g_allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        == XML_STATUS_OK)
      break;
    /* See comment in test_alloc_parse_xdecl() */
    alloc_teardown();
    alloc_setup();
  }
  g_allocation_count = -1;
  if (i == 0)
    fail("External entity parsed despite duff allocator");
  if (i == alloc_test_max_repeats)
    fail("External entity not parsed at max allocation count");
}
END_TEST

/* Test more allocation failure paths */
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
    g_allocation_count = i;
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        == XML_STATUS_OK)
      break;
    g_allocation_count = -1;
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
    g_allocation_count = i;
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
    g_allocation_count = i;
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
    g_allocation_count = i;
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
    g_allocation_count = i;
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
    g_reallocation_count = i;
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
  g_reallocation_count = -1;
  if (i == 0)
    fail("Parse succeeded with no reallocation");
  else if (i == max_realloc_count)
    fail("Parse failed with max reallocation count");
}
END_TEST

TCase *
make_alloc_test_case(Suite *s) {
  TCase *tc_alloc = tcase_create("allocation tests");

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

  return tc_alloc; /* TEMPORARY: this will become a void function */
}
