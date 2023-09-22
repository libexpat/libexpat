/* Tests in the "miscellaneous" test case for the Expat test suite
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
   Copyright (c) 2021      Donghee Na <donghee.na@python.org>
   Copyright (c) 2023      Sony Corporation / Snild Dolkow <snild@sony.com>
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

#include <assert.h>
#include <string.h>

#include "expat_config.h"

#include "expat.h"
#include "internal.h"
#include "minicheck.h"
#include "memcheck.h"
#include "common.h"
#include "ascii.h" /* for ASCII_xxx */
#include "handlers.h"
#include "misc_tests.h"

/* Test that a failure to allocate the parser structure fails gracefully */
START_TEST(test_misc_alloc_create_parser) {
  XML_Memory_Handling_Suite memsuite = {duff_allocator, realloc, free};
  unsigned int i;
  const unsigned int max_alloc_count = 10;

  /* Something this simple shouldn't need more than 10 allocations */
  for (i = 0; i < max_alloc_count; i++) {
    g_allocation_count = i;
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
    g_allocation_count = i;
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

  if (xcstrcmp(version_text, XCS("expat_2.5.0"))) /* needs bump on releases */
    fail("XML_*_VERSION in expat.h out of sync?\n");
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
    set_subtest("%s", inputs[inputIndex]);
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

void
make_miscellaneous_test_case(Suite *s) {
  TCase *tc_misc = tcase_create("miscellaneous tests");

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
}
