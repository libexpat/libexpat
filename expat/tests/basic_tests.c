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

#if ! defined(__cplusplus)
#  include <stdbool.h>
#endif

#include "expat.h"
#include "minicheck.h"
#include "chardata.h"
#include "common.h"
#include "dummy.h"
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

  return tc_basic;
}
