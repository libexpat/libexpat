/* Unit tests related to the fifth edition of XML 1.0 in particular
__  __            _
                         ___\ \/ /_ __   __ _| |_
                        / _ \\  /| '_ \ / _` | __|
                       |  __//  \| |_) | (_| | |_
                        \___/_/\_\ .__/ \__,_|\__|
                                 |_| XML parser

   Copyright (c) 2026 Sebastian Pipping <sebastian@pipping.org>
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

   SPDX-License-Identifier: MIT
*/

#include "fifth_edition_tests.h"

#include "common.h"   // for g_chunksize
#include "internal.h" // for g_reparseDeferralEnabledDefault

#include <stddef.h> // for size_t
#include <stdint.h>

#define EXPAT_MAX_CODEPOINT 0x10FFFF
#define EXPAT_PARTIAL_CHARACTER (EXPAT_MAX_CODEPOINT + 1)
#define EXPAT_MALFORMED_CHARACTER (EXPAT_MAX_CODEPOINT + 2)

static const uint8_t b_0000_0111 = 0x07;
static const uint8_t b_0000_1111 = 0x0F;
static const uint8_t b_0001_1111 = 0x1F;
static const uint8_t b_0011_1111 = 0x3F;
static const uint8_t b_1000_0000 = 0x80;
static const uint8_t b_1100_0000 = 0xC0;
static const uint8_t b_1110_0000 = 0xE0;
static const uint8_t b_1111_0000 = 0xF0;

static unsigned
encodeCharUtf8(unsigned codepoint, uint8_t *target) {
  assert_true(target != NULL);

  if (codepoint > EXPAT_MAX_CODEPOINT)
    return 0;

  if (codepoint <= 0x7F) { // 7 bits
    target[0] = codepoint;
    return 1;
  }

  if (codepoint <= 0x07FF) { // 11 bits
    target[0] = b_1100_0000 | (codepoint >> 6);
    target[1] = b_1000_0000 | (codepoint & b_0011_1111);
    return 2;
  }

  if (codepoint <= 0xFFFF) { // 16 bits
    target[0] = b_1110_0000 | (codepoint >> 12);
    target[1] = b_1000_0000 | ((codepoint >> 6) & b_0011_1111);
    target[2] = b_1000_0000 | (codepoint & b_0011_1111);
    return 3;
  }

  assert_true(codepoint <= 0x10FFFF); // 21 bits
  target[0] = b_1111_0000 + (codepoint >> 18);
  target[1] = b_1000_0000 + ((codepoint >> 12) & b_0011_1111);
  target[2] = b_1000_0000 + ((codepoint >> 6) & b_0011_1111);
  target[3] = b_1000_0000 + (codepoint & b_0011_1111);
  return 4;
}

static unsigned
utf8LeadLen(uint8_t leadByte) {
  if ((leadByte & b_1100_0000) == b_1000_0000)
    return 0;
  if ((leadByte & b_1000_0000) == 0)
    return 1;
  if ((leadByte & b_1110_0000) == b_1100_0000)
    return 2;
  if ((leadByte & b_1111_0000) == b_1110_0000)
    return 3;
  return 4;
}

static unsigned
utf8CharLen(unsigned codepoint) {
  if (codepoint > EXPAT_MAX_CODEPOINT)
    return 0;
  if (codepoint <= 0x7F)
    return 1;
  if (codepoint <= 0x07FF)
    return 2;
  if (codepoint <= 0xFFFF)
    return 3;
  return 4;
}

static unsigned
decodeCharUtf8(const uint8_t *source, size_t maxLength) {
  assert_true(source != NULL);
  if (maxLength == 0)
    return EXPAT_PARTIAL_CHARACTER;

  const uint8_t source_0 = source[0];

  const unsigned len = utf8LeadLen(source_0);

  if (len == 0)
    return EXPAT_MALFORMED_CHARACTER;

  if (len > maxLength)
    return EXPAT_PARTIAL_CHARACTER;

  // Reject invalid continuation bytes
  for (size_t u = 1; u < len - 1; u++) {
    if ((source[u] & b_1100_0000) != b_1000_0000)
      return EXPAT_MALFORMED_CHARACTER;
  }

  switch (len) {
  case 1:
    return source_0;
  case 2:
    return ((source_0 & b_0001_1111) << 6) | (source[1] & b_0011_1111);
  case 3:
    return ((source_0 & b_0000_1111) << 12) | ((source[1] & b_0011_1111) << 6)
           | (source[2] & b_0011_1111);
  case 4:
    return ((source_0 & b_0000_0111) << 18) | ((source[1] & b_0011_1111) << 12)
           | ((source[2] & b_0011_1111) << 6) | (source[3] & b_0011_1111);
  default:
    assert_true(0 && "never gets here");
  }
  return len;
}

START_TEST(test_fifth_edition_utf8_encoding_helper) {
  // Speed things up, the test does not use `XML_Parser`.
  if (g_chunkSize != 0 || g_reparseDeferralEnabledDefault)
    return;

  uint8_t fiveBytes[5];
  for (unsigned codepoint = 0; codepoint <= EXPAT_MAX_CODEPOINT; codepoint++) {
    const unsigned expectedBytesWritten = utf8CharLen(codepoint);

    const unsigned actualBytesWritten = encodeCharUtf8(codepoint, fiveBytes);
    assert_true(actualBytesWritten == expectedBytesWritten);
    fiveBytes[actualBytesWritten] = '\0';

    assert_true(decodeCharUtf8(fiveBytes, actualBytesWritten - 1)
                == EXPAT_PARTIAL_CHARACTER);

    const unsigned decodedCodepoint
        = decodeCharUtf8(fiveBytes, actualBytesWritten);
    assert_true(decodedCodepoint == codepoint);
  }
}
END_TEST

void
make_fifth_edition_test_case(Suite *s) {
  TCase *const tc_fifth_edition = tcase_create("fifth edition tests");
  suite_add_tcase(s, tc_fifth_edition);
  tcase_add_test(tc_fifth_edition, test_fifth_edition_utf8_encoding_helper);
}
