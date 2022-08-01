/* Tests in the "basic" test case of the Expat test suite
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


#include <string.h>

#include "expat.h"
#include "minicheck.h"
#include "common.h"
#include "siphash.h"
#include "basic_tests.h"


/*
 * Character & encoding tests.
 */


START_TEST(test_nul_byte) {
  char text[] = "<doc>\0</doc>";

  /* test that a NUL byte (in US-ASCII data) is an error */
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_OK)
    fail("Parser did not report error on NUL-byte.");
  if (XML_GetErrorCode(g_parser) != XML_ERROR_INVALID_TOKEN)
    xml_failure(g_parser);
}
END_TEST

START_TEST(test_u0000_char) {
  /* test that a NUL byte (in US-ASCII data) is an error */
  expect_failure("<doc>&#0;</doc>", XML_ERROR_BAD_CHAR_REF,
                 "Parser did not report error on NUL-byte.");
}
END_TEST

START_TEST(test_siphash_self) {
  if (! sip24_valid())
    fail("SipHash self-test failed");
}
END_TEST

START_TEST(test_siphash_spec) {
  /* https://131002.net/siphash/siphash.pdf (page 19, "Test values") */
  const char message[] = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09"
                         "\x0a\x0b\x0c\x0d\x0e";
  const size_t len = sizeof(message) - 1;
  const uint64_t expected = _SIP_ULL(0xa129ca61U, 0x49be45e5U);
  struct siphash state;
  struct sipkey key;

  sip_tokey(&key, "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09"
                  "\x0a\x0b\x0c\x0d\x0e\x0f");
  sip24_init(&state, &key);

  /* Cover spread across calls */
  sip24_update(&state, message, 4);
  sip24_update(&state, message + 4, len - 4);

  /* Cover null length */
  sip24_update(&state, message, 0);

  if (sip24_final(&state) != expected)
    fail("sip24_final failed spec test\n");

  /* Cover wrapper */
  if (siphash24(message, len, &key) != expected)
    fail("siphash24 failed spec test\n");
}
END_TEST

START_TEST(test_bom_utf8) {
  /* This test is really just making sure we don't core on a UTF-8 BOM. */
  const char *text = "\357\273\277<e/>";

  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

START_TEST(test_bom_utf16_be) {
  char text[] = "\376\377\0<\0e\0/\0>";

  if (_XML_Parse_SINGLE_BYTES(g_parser, text, sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

START_TEST(test_bom_utf16_le) {
  char text[] = "\377\376<\0e\0/\0>\0";

  if (_XML_Parse_SINGLE_BYTES(g_parser, text, sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Parse whole buffer at once to exercise a different code path */
START_TEST(test_nobom_utf16_le) {
  char text[] = " \0<\0e\0/\0>\0";

  if (XML_Parse(g_parser, text, sizeof(text) - 1, XML_TRUE) == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

TCase *
make_basic_test_case(Suite *s) {
  TCase *tc_basic = tcase_create("basic tests");

  suite_add_tcase(s, tc_basic);
  tcase_add_checked_fixture(tc_basic, basic_setup, basic_teardown);
  tcase_add_test(tc_basic, test_nul_byte);
  tcase_add_test(tc_basic, test_u0000_char);
  tcase_add_test(tc_basic, test_siphash_self);
  tcase_add_test(tc_basic, test_siphash_spec);
  tcase_add_test(tc_basic, test_bom_utf8);
  tcase_add_test(tc_basic, test_bom_utf16_be);
  tcase_add_test(tc_basic, test_bom_utf16_le);
  tcase_add_test(tc_basic, test_nobom_utf16_le);

  return tc_basic;
}

