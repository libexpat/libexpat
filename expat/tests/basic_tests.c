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


#include <stdio.h>
#include <string.h>
#include <assert.h>

#if ! defined(__cplusplus)
#  include <stdbool.h>
#endif

#include "expat.h"
#include "internal.h"  /* For UNUSED_P() */
#include "minicheck.h"
#include "chardata.h"
#include "structdata.h"
#include "common.h"
#include "dummy.h"
#include "handlers.h"
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

/* Regression test for SF bug #491986. */
START_TEST(test_danish_latin1) {
  const char *text = "<?xml version='1.0' encoding='iso-8859-1'?>\n"
                     "<e>J\xF8rgen \xE6\xF8\xE5\xC6\xD8\xC5</e>";
#ifdef XML_UNICODE
  const XML_Char *expected
      = XCS("J\x00f8rgen \x00e6\x00f8\x00e5\x00c6\x00d8\x00c5");
#else
  const XML_Char *expected
      = XCS("J\xC3\xB8rgen \xC3\xA6\xC3\xB8\xC3\xA5\xC3\x86\xC3\x98\xC3\x85");
#endif
  run_character_check(text, expected);
}
END_TEST

/* Regression test for SF bug #514281. */
START_TEST(test_french_charref_hexidecimal) {
  const char *text = "<?xml version='1.0' encoding='iso-8859-1'?>\n"
                     "<doc>&#xE9;&#xE8;&#xE0;&#xE7;&#xEA;&#xC8;</doc>";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x00e9\x00e8\x00e0\x00e7\x00ea\x00c8");
#else
  const XML_Char *expected
      = XCS("\xC3\xA9\xC3\xA8\xC3\xA0\xC3\xA7\xC3\xAA\xC3\x88");
#endif
  run_character_check(text, expected);
}
END_TEST

START_TEST(test_french_charref_decimal) {
  const char *text = "<?xml version='1.0' encoding='iso-8859-1'?>\n"
                     "<doc>&#233;&#232;&#224;&#231;&#234;&#200;</doc>";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x00e9\x00e8\x00e0\x00e7\x00ea\x00c8");
#else
  const XML_Char *expected
      = XCS("\xC3\xA9\xC3\xA8\xC3\xA0\xC3\xA7\xC3\xAA\xC3\x88");
#endif
  run_character_check(text, expected);
}
END_TEST

START_TEST(test_french_latin1) {
  const char *text = "<?xml version='1.0' encoding='iso-8859-1'?>\n"
                     "<doc>\xE9\xE8\xE0\xE7\xEa\xC8</doc>";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x00e9\x00e8\x00e0\x00e7\x00ea\x00c8");
#else
  const XML_Char *expected
      = XCS("\xC3\xA9\xC3\xA8\xC3\xA0\xC3\xA7\xC3\xAA\xC3\x88");
#endif
  run_character_check(text, expected);
}
END_TEST

START_TEST(test_french_utf8) {
  const char *text = "<?xml version='1.0' encoding='utf-8'?>\n"
                     "<doc>\xC3\xA9</doc>";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x00e9");
#else
  const XML_Char *expected = XCS("\xC3\xA9");
#endif
  run_character_check(text, expected);
}
END_TEST

/* Regression test for SF bug #600479.
   XXX There should be a test that exercises all legal XML Unicode
   characters as PCDATA and attribute value content, and XML Name
   characters as part of element and attribute names.
*/
START_TEST(test_utf8_false_rejection) {
  const char *text = "<doc>\xEF\xBA\xBF</doc>";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\xfebf");
#else
  const XML_Char *expected = XCS("\xEF\xBA\xBF");
#endif
  run_character_check(text, expected);
}
END_TEST

/* Regression test for SF bug #477667.
   This test assures that any 8-bit character followed by a 7-bit
   character will not be mistakenly interpreted as a valid UTF-8
   sequence.
*/
START_TEST(test_illegal_utf8) {
  char text[100];
  int i;

  for (i = 128; i <= 255; ++i) {
    sprintf(text, "<e>%ccd</e>", i);
    if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
        == XML_STATUS_OK) {
      sprintf(text, "expected token error for '%c' (ordinal %d) in UTF-8 text",
              i, i);
      fail(text);
    } else if (XML_GetErrorCode(g_parser) != XML_ERROR_INVALID_TOKEN)
      xml_failure(g_parser);
    /* Reset the parser since we use the same parser repeatedly. */
    XML_ParserReset(g_parser, NULL);
  }
}
END_TEST

/* Examples, not masks: */
#define UTF8_LEAD_1 "\x7f" /* 0b01111111 */
#define UTF8_LEAD_2 "\xdf" /* 0b11011111 */
#define UTF8_LEAD_3 "\xef" /* 0b11101111 */
#define UTF8_LEAD_4 "\xf7" /* 0b11110111 */
#define UTF8_FOLLOW "\xbf" /* 0b10111111 */

START_TEST(test_utf8_auto_align) {
  struct TestCase {
    ptrdiff_t expectedMovementInChars;
    const char *input;
  };

  struct TestCase cases[] = {
      {00, ""},

      {00, UTF8_LEAD_1},

      {-1, UTF8_LEAD_2},
      {00, UTF8_LEAD_2 UTF8_FOLLOW},

      {-1, UTF8_LEAD_3},
      {-2, UTF8_LEAD_3 UTF8_FOLLOW},
      {00, UTF8_LEAD_3 UTF8_FOLLOW UTF8_FOLLOW},

      {-1, UTF8_LEAD_4},
      {-2, UTF8_LEAD_4 UTF8_FOLLOW},
      {-3, UTF8_LEAD_4 UTF8_FOLLOW UTF8_FOLLOW},
      {00, UTF8_LEAD_4 UTF8_FOLLOW UTF8_FOLLOW UTF8_FOLLOW},
  };

  size_t i = 0;
  bool success = true;
  for (; i < sizeof(cases) / sizeof(*cases); i++) {
    const char *fromLim = cases[i].input + strlen(cases[i].input);
    const char *const fromLimInitially = fromLim;
    ptrdiff_t actualMovementInChars;

    _INTERNAL_trim_to_complete_utf8_characters(cases[i].input, &fromLim);

    actualMovementInChars = (fromLim - fromLimInitially);
    if (actualMovementInChars != cases[i].expectedMovementInChars) {
      size_t j = 0;
      success = false;
      printf("[-] UTF-8 case %2u: Expected movement by %2d chars"
             ", actually moved by %2d chars: \"",
             (unsigned)(i + 1), (int)cases[i].expectedMovementInChars,
             (int)actualMovementInChars);
      for (; j < strlen(cases[i].input); j++) {
        printf("\\x%02x", (unsigned char)cases[i].input[j]);
      }
      printf("\"\n");
    }
  }

  if (! success) {
    fail("UTF-8 auto-alignment is not bullet-proof\n");
  }
}
END_TEST

START_TEST(test_utf16) {
  /* <?xml version="1.0" encoding="UTF-16"?>
   *  <doc a='123'>some {A} text</doc>
   *
   * where {A} is U+FF21, FULLWIDTH LATIN CAPITAL LETTER A
   */
  char text[]
      = "\000<\000?\000x\000m\000\154\000 \000v\000e\000r\000s\000i\000o"
        "\000n\000=\000'\0001\000.\000\060\000'\000 \000e\000n\000c\000o"
        "\000d\000i\000n\000g\000=\000'\000U\000T\000F\000-\0001\000\066"
        "\000'\000?\000>\000\n"
        "\000<\000d\000o\000c\000 \000a\000=\000'\0001\0002\0003\000'\000>"
        "\000s\000o\000m\000e\000 \xff\x21\000 \000t\000e\000x\000t\000"
        "<\000/\000d\000o\000c\000>";
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("some \xff21 text");
#else
  const XML_Char *expected = XCS("some \357\274\241 text");
#endif
  CharData storage;

  CharData_Init(&storage);
  XML_SetUserData(g_parser, &storage);
  XML_SetCharacterDataHandler(g_parser, accumulate_characters);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, sizeof(text) - 1, XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

START_TEST(test_utf16_le_epilog_newline) {
  unsigned int first_chunk_bytes = 17;
  char text[] = "\xFF\xFE"                  /* BOM */
                "<\000e\000/\000>\000"      /* document element */
                "\r\000\n\000\r\000\n\000"; /* epilog */

  if (first_chunk_bytes >= sizeof(text) - 1)
    fail("bad value of first_chunk_bytes");
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, first_chunk_bytes, XML_FALSE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  else {
    enum XML_Status rc;
    rc = _XML_Parse_SINGLE_BYTES(g_parser, text + first_chunk_bytes,
                                 sizeof(text) - first_chunk_bytes - 1,
                                 XML_TRUE);
    if (rc == XML_STATUS_ERROR)
      xml_failure(g_parser);
  }
}
END_TEST

/* Test that an outright lie in the encoding is faulted */
START_TEST(test_not_utf16) {
  const char *text = "<?xml version='1.0' encoding='utf-16'?>"
                     "<doc>Hi</doc>";

  /* Use a handler to provoke the appropriate code paths */
  XML_SetXmlDeclHandler(g_parser, dummy_xdecl_handler);
  expect_failure(text, XML_ERROR_INCORRECT_ENCODING,
                 "UTF-16 declared in UTF-8 not faulted");
}
END_TEST

/* Test that an unknown encoding is rejected */
START_TEST(test_bad_encoding) {
  const char *text = "<doc>Hi</doc>";

  if (! XML_SetEncoding(g_parser, XCS("unknown-encoding")))
    fail("XML_SetEncoding failed");
  expect_failure(text, XML_ERROR_UNKNOWN_ENCODING,
                 "Unknown encoding not faulted");
}
END_TEST

/* Regression test for SF bug #481609, #774028. */
START_TEST(test_latin1_umlauts) {
  const char *text
      = "<?xml version='1.0' encoding='iso-8859-1'?>\n"
        "<e a='\xE4 \xF6 \xFC &#228; &#246; &#252; &#x00E4; &#x0F6; &#xFC; >'\n"
        "  >\xE4 \xF6 \xFC &#228; &#246; &#252; &#x00E4; &#x0F6; &#xFC; ></e>";
#ifdef XML_UNICODE
  /* Expected results in UTF-16 */
  const XML_Char *expected = XCS("\x00e4 \x00f6 \x00fc ")
      XCS("\x00e4 \x00f6 \x00fc ") XCS("\x00e4 \x00f6 \x00fc >");
#else
  /* Expected results in UTF-8 */
  const XML_Char *expected = XCS("\xC3\xA4 \xC3\xB6 \xC3\xBC ")
      XCS("\xC3\xA4 \xC3\xB6 \xC3\xBC ") XCS("\xC3\xA4 \xC3\xB6 \xC3\xBC >");
#endif

  run_character_check(text, expected);
  XML_ParserReset(g_parser, NULL);
  run_attribute_check(text, expected);
  /* Repeat with a default handler */
  XML_ParserReset(g_parser, NULL);
  XML_SetDefaultHandler(g_parser, dummy_default_handler);
  run_character_check(text, expected);
  XML_ParserReset(g_parser, NULL);
  XML_SetDefaultHandler(g_parser, dummy_default_handler);
  run_attribute_check(text, expected);
}
END_TEST

/* Test that an element name with a 4-byte UTF-8 character is rejected */
START_TEST(test_long_utf8_character) {
  const char *text
      = "<?xml version='1.0' encoding='utf-8'?>\n"
        /* 0xf0 0x90 0x80 0x80 = U+10000, the first Linear B character */
        "<do\xf0\x90\x80\x80/>";
  expect_failure(text, XML_ERROR_INVALID_TOKEN,
                 "4-byte UTF-8 character in element name not faulted");
}
END_TEST

/* Test that a long latin-1 attribute (too long to convert in one go)
 * is correctly converted
 */
START_TEST(test_long_latin1_attribute) {
  const char *text
      = "<?xml version='1.0' encoding='iso-8859-1'?>\n"
        "<doc att='"
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
        "ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO"
        /* Last character splits across a buffer boundary */
        "\xe4'>\n</doc>";

  const XML_Char *expected =
      /* 64 characters per line */
      /* clang-format off */
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNO")
  /* clang-format on */
#ifdef XML_UNICODE
                                                  XCS("\x00e4");
#else
                                                  XCS("\xc3\xa4");
#endif

  run_attribute_check(text, expected);
}
END_TEST

/* Test that a long ASCII attribute (too long to convert in one go)
 * is correctly converted
 */
START_TEST(test_long_ascii_attribute) {
  const char *text
      = "<?xml version='1.0' encoding='us-ascii'?>\n"
        "<doc att='"
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
        "01234'>\n</doc>";
  const XML_Char *expected =
      /* 64 characters per line */
      /* clang-format off */
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP")
        XCS("01234");
  /* clang-format on */

  run_attribute_check(text, expected);
}
END_TEST

/* Regression test #1 for SF bug #653180. */
START_TEST(test_line_number_after_parse) {
  const char *text = "<tag>\n"
                     "\n"
                     "\n</tag>";
  XML_Size lineno;

  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_FALSE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  lineno = XML_GetCurrentLineNumber(g_parser);
  if (lineno != 4) {
    char buffer[100];
    sprintf(buffer, "expected 4 lines, saw %" XML_FMT_INT_MOD "u", lineno);
    fail(buffer);
  }
}
END_TEST

/* Regression test #2 for SF bug #653180. */
START_TEST(test_column_number_after_parse) {
  const char *text = "<tag></tag>";
  XML_Size colno;

  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_FALSE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  colno = XML_GetCurrentColumnNumber(g_parser);
  if (colno != 11) {
    char buffer[100];
    sprintf(buffer, "expected 11 columns, saw %" XML_FMT_INT_MOD "u", colno);
    fail(buffer);
  }
}
END_TEST

/* Regression test #3 for SF bug #653180. */
START_TEST(test_line_and_column_numbers_inside_handlers) {
  const char *text = "<a>\n"      /* Unix end-of-line */
                     "  <b>\r\n"  /* Windows end-of-line */
                     "    <c/>\r" /* Mac OS end-of-line */
                     "  </b>\n"
                     "  <d>\n"
                     "    <f/>\n"
                     "  </d>\n"
                     "</a>";
  const StructDataEntry expected[]
      = {{XCS("a"), 0, 1, STRUCT_START_TAG}, {XCS("b"), 2, 2, STRUCT_START_TAG},
         {XCS("c"), 4, 3, STRUCT_START_TAG}, {XCS("c"), 8, 3, STRUCT_END_TAG},
         {XCS("b"), 2, 4, STRUCT_END_TAG},   {XCS("d"), 2, 5, STRUCT_START_TAG},
         {XCS("f"), 4, 6, STRUCT_START_TAG}, {XCS("f"), 8, 6, STRUCT_END_TAG},
         {XCS("d"), 2, 7, STRUCT_END_TAG},   {XCS("a"), 0, 8, STRUCT_END_TAG}};
  const int expected_count = sizeof(expected) / sizeof(StructDataEntry);
  StructData storage;

  StructData_Init(&storage);
  XML_SetUserData(g_parser, &storage);
  XML_SetStartElementHandler(g_parser, start_element_event_handler2);
  XML_SetEndElementHandler(g_parser, end_element_event_handler2);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);

  StructData_CheckItems(&storage, expected, expected_count);
  StructData_Dispose(&storage);
}
END_TEST

/* Regression test #4 for SF bug #653180. */
START_TEST(test_line_number_after_error) {
  const char *text = "<a>\n"
                     "  <b>\n"
                     "  </a>"; /* missing </b> */
  XML_Size lineno;
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_FALSE)
      != XML_STATUS_ERROR)
    fail("Expected a parse error");

  lineno = XML_GetCurrentLineNumber(g_parser);
  if (lineno != 3) {
    char buffer[100];
    sprintf(buffer, "expected 3 lines, saw %" XML_FMT_INT_MOD "u", lineno);
    fail(buffer);
  }
}
END_TEST

/* Regression test #5 for SF bug #653180. */
START_TEST(test_column_number_after_error) {
  const char *text = "<a>\n"
                     "  <b>\n"
                     "  </a>"; /* missing </b> */
  XML_Size colno;
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_FALSE)
      != XML_STATUS_ERROR)
    fail("Expected a parse error");

  colno = XML_GetCurrentColumnNumber(g_parser);
  if (colno != 4) {
    char buffer[100];
    sprintf(buffer, "expected 4 columns, saw %" XML_FMT_INT_MOD "u", colno);
    fail(buffer);
  }
}
END_TEST

/* Regression test for SF bug #478332. */
START_TEST(test_really_long_lines) {
  /* This parses an input line longer than INIT_DATA_BUF_SIZE
     characters long (defined to be 1024 in xmlparse.c).  We take a
     really cheesy approach to building the input buffer, because
     this avoids writing bugs in buffer-filling code.
  */
  const char *text
      = "<e>"
        /* 64 chars */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        /* until we have at least 1024 characters on the line: */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "</e>";
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Test cdata processing across a buffer boundary */
START_TEST(test_really_long_encoded_lines) {
  /* As above, except that we want to provoke an output buffer
   * overflow with a non-trivial encoding.  For this we need to pass
   * the whole cdata in one go, not byte-by-byte.
   */
  void *buffer;
  const char *text
      = "<?xml version='1.0' encoding='iso-8859-1'?>"
        "<e>"
        /* 64 chars */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        /* until we have at least 1024 characters on the line: */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+"
        "</e>";
  int parse_len = (int)strlen(text);

  /* Need a cdata handler to provoke the code path we want to test */
  XML_SetCharacterDataHandler(g_parser, dummy_cdata_handler);
  buffer = XML_GetBuffer(g_parser, parse_len);
  if (buffer == NULL)
    fail("Could not allocate parse buffer");
  assert(buffer != NULL);
  memcpy(buffer, text, parse_len);
  if (XML_ParseBuffer(g_parser, parse_len, XML_TRUE) == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/*
 * Element event tests.
 */

START_TEST(test_end_element_events) {
  const char *text = "<a><b><c/></b><d><f/></d></a>";
  const XML_Char *expected = XCS("/c/b/f/d/a");
  CharData storage;

  CharData_Init(&storage);
  XML_SetUserData(g_parser, &storage);
  XML_SetEndElementHandler(g_parser, end_element_event_handler);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, expected);
}
END_TEST

/*
 * Attribute tests.
 */

static void XMLCALL
check_attr_contains_normalized_whitespace(void *userData, const XML_Char *name,
                                          const XML_Char **atts) {
  int i;
  UNUSED_P(userData);
  UNUSED_P(name);
  for (i = 0; atts[i] != NULL; i += 2) {
    const XML_Char *attrname = atts[i];
    const XML_Char *value = atts[i + 1];
    if (xcstrcmp(XCS("attr"), attrname) == 0
        || xcstrcmp(XCS("ents"), attrname) == 0
        || xcstrcmp(XCS("refs"), attrname) == 0) {
      if (! is_whitespace_normalized(value, 0)) {
        char buffer[256];
        sprintf(buffer,
                "attribute value not normalized: %" XML_FMT_STR
                "='%" XML_FMT_STR "'",
                attrname, value);
        fail(buffer);
      }
    }
  }
}

START_TEST(test_attr_whitespace_normalization) {
  const char *text
      = "<!DOCTYPE doc [\n"
        "  <!ATTLIST doc\n"
        "            attr NMTOKENS #REQUIRED\n"
        "            ents ENTITIES #REQUIRED\n"
        "            refs IDREFS   #REQUIRED>\n"
        "]>\n"
        "<doc attr='    a  b c\t\td\te\t' refs=' id-1   \t  id-2\t\t'  \n"
        "     ents=' ent-1   \t\r\n"
        "            ent-2  ' >\n"
        "  <e id='id-1'/>\n"
        "  <e id='id-2'/>\n"
        "</doc>";

  XML_SetStartElementHandler(g_parser,
                             check_attr_contains_normalized_whitespace);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/*
 * XML declaration tests.
 */

START_TEST(test_xmldecl_misplaced) {
  expect_failure("\n"
                 "<?xml version='1.0'?>\n"
                 "<a/>",
                 XML_ERROR_MISPLACED_XML_PI,
                 "failed to report misplaced XML declaration");
}
END_TEST

START_TEST(test_xmldecl_invalid) {
  expect_failure("<?xml version='1.0' \xc3\xa7?>\n<doc/>", XML_ERROR_XML_DECL,
                 "Failed to report invalid XML declaration");
}
END_TEST

START_TEST(test_xmldecl_missing_attr) {
  expect_failure("<?xml ='1.0'?>\n<doc/>\n", XML_ERROR_XML_DECL,
                 "Failed to report missing XML declaration attribute");
}
END_TEST

START_TEST(test_xmldecl_missing_value) {
  expect_failure("<?xml version='1.0' encoding='us-ascii' standalone?>\n"
                 "<doc/>",
                 XML_ERROR_XML_DECL,
                 "Failed to report missing attribute value");
}
END_TEST

/* Regression test for SF bug #584832. */
START_TEST(test_unknown_encoding_internal_entity) {
  const char *text = "<?xml version='1.0' encoding='unsupported-encoding'?>\n"
                     "<!DOCTYPE test [<!ENTITY foo 'bar'>]>\n"
                     "<test a='&foo;'/>";

  XML_SetUnknownEncodingHandler(g_parser, UnknownEncodingHandler, NULL);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Test unrecognised encoding handler */
START_TEST(test_unrecognised_encoding_internal_entity) {
  const char *text = "<?xml version='1.0' encoding='unsupported-encoding'?>\n"
                     "<!DOCTYPE test [<!ENTITY foo 'bar'>]>\n"
                     "<test a='&foo;'/>";

  XML_SetUnknownEncodingHandler(g_parser, UnrecognisedEncodingHandler, NULL);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      != XML_STATUS_ERROR)
    fail("Unrecognised encoding not rejected");
}
END_TEST

/* Regression test for SF bug #620106. */
START_TEST(test_ext_entity_set_encoding) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";
  ExtTest test_data
      = {/* This text says it's an unsupported encoding, but it's really
            UTF-8, which we tell Expat using XML_SetEncoding().
         */
         "<?xml encoding='iso-8859-3'?>\xC3\xA9", XCS("utf-8"), NULL};
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x00e9");
#else
  const XML_Char *expected = XCS("\xc3\xa9");
#endif

  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader);
  run_ext_character_check(text, &test_data, expected);
}
END_TEST

/* Test external entities with no handler */
START_TEST(test_ext_entity_no_handler) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";

  XML_SetDefaultHandler(g_parser, dummy_default_handler);
  run_character_check(text, XCS(""));
}
END_TEST

/* Test UTF-8 BOM is accepted */
START_TEST(test_ext_entity_set_bom) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";
  ExtTest test_data = {"\xEF\xBB\xBF" /* BOM */
                       "<?xml encoding='iso-8859-3'?>"
                       "\xC3\xA9",
                       XCS("utf-8"), NULL};
#ifdef XML_UNICODE
  const XML_Char *expected = XCS("\x00e9");
#else
  const XML_Char *expected = XCS("\xc3\xa9");
#endif

  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader);
  run_ext_character_check(text, &test_data, expected);
}
END_TEST

/* Test that bad encodings are faulted */
START_TEST(test_ext_entity_bad_encoding) {
  const char *text = "<!DOCTYPE doc [\n"
                     "  <!ENTITY en SYSTEM 'http://example.org/dummy.ent'>\n"
                     "]>\n"
                     "<doc>&en;</doc>";
  ExtFaults fault
      = {"<?xml encoding='iso-8859-3'?>u", "Unsupported encoding not faulted",
         XCS("unknown"), XML_ERROR_UNKNOWN_ENCODING};

  XML_SetExternalEntityRefHandler(g_parser, external_entity_faulter);
  XML_SetUserData(g_parser, &fault);
  expect_failure(text, XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                 "Bad encoding should not have been accepted");
}
END_TEST

/* Try handing an invalid encoding to an external entity parser */
START_TEST(test_ext_entity_bad_encoding_2) {
  const char *text = "<?xml version='1.0' encoding='us-ascii'?>\n"
                     "<!DOCTYPE doc SYSTEM 'foo'>\n"
                     "<doc>&entity;</doc>";
  ExtFaults fault
      = {"<!ELEMENT doc (#PCDATA)*>", "Unknown encoding not faulted",
         XCS("unknown-encoding"), XML_ERROR_UNKNOWN_ENCODING};

  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_faulter);
  XML_SetUserData(g_parser, &fault);
  expect_failure(text, XML_ERROR_EXTERNAL_ENTITY_HANDLING,
                 "Bad encoding not faulted in external entity handler");
}
END_TEST

/* Test that no error is reported for unknown entities if we don't
   read an external subset.  This was fixed in Expat 1.95.5.
*/
START_TEST(test_wfc_undeclared_entity_unread_external_subset) {
  const char *text = "<!DOCTYPE doc SYSTEM 'foo'>\n"
                     "<doc>&entity;</doc>";

  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

/* Test that an error is reported for unknown entities if we don't
   have an external subset.
*/
START_TEST(test_wfc_undeclared_entity_no_external_subset) {
  expect_failure("<doc>&entity;</doc>", XML_ERROR_UNDEFINED_ENTITY,
                 "Parser did not report undefined entity w/out a DTD.");
}
END_TEST

/* Test that an error is reported for unknown entities if we don't
   read an external subset, but have been declared standalone.
*/
START_TEST(test_wfc_undeclared_entity_standalone) {
  const char *text
      = "<?xml version='1.0' encoding='us-ascii' standalone='yes'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";

  expect_failure(text, XML_ERROR_UNDEFINED_ENTITY,
                 "Parser did not report undefined entity (standalone).");
}
END_TEST

/* Test that an error is reported for unknown entities if we have read
   an external subset, and standalone is true.
*/
START_TEST(test_wfc_undeclared_entity_with_external_subset_standalone) {
  const char *text
      = "<?xml version='1.0' encoding='us-ascii' standalone='yes'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";
  ExtTest test_data = {"<!ELEMENT doc (#PCDATA)*>", NULL, NULL};

  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetUserData(g_parser, &test_data);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader);
  expect_failure(text, XML_ERROR_UNDEFINED_ENTITY,
                 "Parser did not report undefined entity (external DTD).");
}
END_TEST

/* Test that external entity handling is not done if the parsing flag
 * is set to UNLESS_STANDALONE
 */
START_TEST(test_entity_with_external_subset_unless_standalone) {
  const char *text
      = "<?xml version='1.0' encoding='us-ascii' standalone='yes'?>\n"
        "<!DOCTYPE doc SYSTEM 'foo'>\n"
        "<doc>&entity;</doc>";
  ExtTest test_data = {"<!ENTITY entity 'bar'>", NULL, NULL};

  XML_SetParamEntityParsing(g_parser,
                            XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE);
  XML_SetUserData(g_parser, &test_data);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader);
  expect_failure(text, XML_ERROR_UNDEFINED_ENTITY,
                 "Parser did not report undefined entity");
}
END_TEST

/* Test that no error is reported for unknown entities if we have read
   an external subset, and standalone is false.
*/
START_TEST(test_wfc_undeclared_entity_with_external_subset) {
  const char *text = "<?xml version='1.0' encoding='us-ascii'?>\n"
                     "<!DOCTYPE doc SYSTEM 'foo'>\n"
                     "<doc>&entity;</doc>";
  ExtTest test_data = {"<!ELEMENT doc (#PCDATA)*>", NULL, NULL};

  XML_SetParamEntityParsing(g_parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetExternalEntityRefHandler(g_parser, external_entity_loader);
  run_ext_character_check(text, &test_data, XCS(""));
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
  tcase_add_test(tc_basic, test_illegal_utf8);
  tcase_add_test(tc_basic, test_utf8_auto_align);
  tcase_add_test(tc_basic, test_utf16);
  tcase_add_test(tc_basic, test_utf16_le_epilog_newline);
  tcase_add_test(tc_basic, test_not_utf16);
  tcase_add_test(tc_basic, test_bad_encoding);
  tcase_add_test(tc_basic, test_latin1_umlauts);
  tcase_add_test(tc_basic, test_long_utf8_character);
  tcase_add_test(tc_basic, test_long_latin1_attribute);
  tcase_add_test(tc_basic, test_long_ascii_attribute);
  /* Regression test for SF bug #491986. */
  tcase_add_test(tc_basic, test_danish_latin1);
  /* Regression test for SF bug #514281. */
  tcase_add_test(tc_basic, test_french_charref_hexidecimal);
  tcase_add_test(tc_basic, test_french_charref_decimal);
  tcase_add_test(tc_basic, test_french_latin1);
  tcase_add_test(tc_basic, test_french_utf8);
  tcase_add_test(tc_basic, test_utf8_false_rejection);
  tcase_add_test(tc_basic, test_line_number_after_parse);
  tcase_add_test(tc_basic, test_column_number_after_parse);
  tcase_add_test(tc_basic, test_line_and_column_numbers_inside_handlers);
  tcase_add_test(tc_basic, test_line_number_after_error);
  tcase_add_test(tc_basic, test_column_number_after_error);
  tcase_add_test(tc_basic, test_really_long_lines);
  tcase_add_test(tc_basic, test_really_long_encoded_lines);
  tcase_add_test(tc_basic, test_end_element_events);
  tcase_add_test(tc_basic, test_attr_whitespace_normalization);
  tcase_add_test(tc_basic, test_xmldecl_misplaced);
  tcase_add_test(tc_basic, test_xmldecl_invalid);
  tcase_add_test(tc_basic, test_xmldecl_missing_attr);
  tcase_add_test(tc_basic, test_xmldecl_missing_value);
  tcase_add_test(tc_basic, test_unknown_encoding_internal_entity);
  tcase_add_test(tc_basic, test_unrecognised_encoding_internal_entity);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_ext_entity_set_encoding);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_ext_entity_no_handler);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_ext_entity_set_bom);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_ext_entity_bad_encoding);
  tcase_add_test__ifdef_xml_dtd(tc_basic, test_ext_entity_bad_encoding_2);
  tcase_add_test(tc_basic, test_wfc_undeclared_entity_unread_external_subset);
  tcase_add_test(tc_basic, test_wfc_undeclared_entity_no_external_subset);
  tcase_add_test(tc_basic, test_wfc_undeclared_entity_standalone);
  tcase_add_test(tc_basic,
                 test_wfc_undeclared_entity_with_external_subset_standalone);
  tcase_add_test(tc_basic, test_entity_with_external_subset_unless_standalone);
  tcase_add_test(tc_basic, test_wfc_undeclared_entity_with_external_subset);

  return tc_basic;
}

