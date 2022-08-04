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
#include "ns_tests.h"

XML_Parser g_parser = NULL;

/* Check the attribute whitespace checker: */
static void
testhelper_is_whitespace_normalized(void) {
  assert(is_whitespace_normalized(XCS("abc"), 0));
  assert(is_whitespace_normalized(XCS("abc"), 1));
  assert(is_whitespace_normalized(XCS("abc def ghi"), 0));
  assert(is_whitespace_normalized(XCS("abc def ghi"), 1));
  assert(! is_whitespace_normalized(XCS(" abc def ghi"), 0));
  assert(is_whitespace_normalized(XCS(" abc def ghi"), 1));
  assert(! is_whitespace_normalized(XCS("abc  def ghi"), 0));
  assert(is_whitespace_normalized(XCS("abc  def ghi"), 1));
  assert(! is_whitespace_normalized(XCS("abc def ghi "), 0));
  assert(is_whitespace_normalized(XCS("abc def ghi "), 1));
  assert(! is_whitespace_normalized(XCS(" "), 0));
  assert(is_whitespace_normalized(XCS(" "), 1));
  assert(! is_whitespace_normalized(XCS("\t"), 0));
  assert(! is_whitespace_normalized(XCS("\t"), 1));
  assert(! is_whitespace_normalized(XCS("\n"), 0));
  assert(! is_whitespace_normalized(XCS("\n"), 1));
  assert(! is_whitespace_normalized(XCS("\r"), 0));
  assert(! is_whitespace_normalized(XCS("\r"), 1));
  assert(! is_whitespace_normalized(XCS("abc\t def"), 1));
}

/*
 * Namespaces tests.
 */

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
  if (xcstrcmp(version_text, XCS("expat_2.4.8"))) /* needs bump on releases */
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
  XML_Parser new_parser;
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
portableNAN() {
  return strtof("nan", NULL);
}

static float
portableINFINITY() {
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
  TCase *tc_misc;
  TCase *tc_alloc;
  TCase *tc_nsalloc;
#if defined(XML_DTD)
  TCase *tc_accounting;
#endif

  make_basic_test_case(s);
  make_namespace_test_case(s);
  tc_misc = tcase_create("miscellaneous tests");
  tc_alloc = tcase_create("allocation tests");
  tc_nsalloc = tcase_create("namespace allocation tests");
#if defined(XML_DTD)
  tc_accounting = tcase_create("accounting tests");
#endif

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

  /* run the tests for internal helper functions */
  testhelper_is_whitespace_normalized();

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
