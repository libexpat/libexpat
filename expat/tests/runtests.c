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

#include "basic_tests.h"
#include "ns_tests.h"
#include "misc_tests.h"
#include "alloc_tests.h"
#include "nsalloc_tests.h"

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
    g_reallocation_count = i;
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
    g_reallocation_count = i;
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
    g_reallocation_count = i;
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
    g_allocation_count = i;
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
    g_allocation_count = i;
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
    g_allocation_count = i;
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
  TCase *tc_nsalloc;
#if defined(XML_DTD)
  TCase *tc_accounting;
#endif

  make_basic_test_case(s);
  make_namespace_test_case(s);
  make_miscellaneous_test_case(s);
  make_allocation_test_case(s);
  tc_nsalloc = make_namespace_allocation_test_case(s);
#if defined(XML_DTD)
  tc_accounting = tcase_create("accounting tests");
#endif

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
