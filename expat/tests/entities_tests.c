/* Unit tests related to internal handling of entities
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

#include "entities_tests.h"
#include "expat_config.h" // for XML_GE

#include "ascii.h"    // for ASCII_*
#include "common.h"   // for g_chunkSize, xcsdup_str
#include "entities.h" // for isCDataCommentOrPIStart
#include "xmltok.h"   // for XML_TOK_*
#include "xcs.h"      // for xcslen

START_TEST(test_is_cdata_comment_or_pi_start) {
  // Speed up the test, chunk size expected to not matter in this test
  if (g_chunkSize != 0)
    return;

  XML_Char *const cdataOpen = xcsdup_str("<![CDATA[");
  XML_Char *const commentOpen = xcsdup_str("<!--");
  XML_Char *const piOpen = xcsdup_str("<?");

#if XML_GE == 1
  const size_t expectedCdataOpenLen = xcslen(cdataOpen);
  const size_t expectedCommentOpenLen = xcslen(commentOpen);
  const size_t expectedPiOpenLen = xcslen(piOpen);

  size_t actualCdataOpenLen = 0;
  assert_true(isCDataCommentOrPIStart(cdataOpen, expectedCdataOpenLen,
                                      &actualCdataOpenLen)
              == XML_TOK_CDATA_SECT_OPEN);
  assert_true(actualCdataOpenLen == expectedCdataOpenLen);

  size_t actualCommentOpenLen = 0;
  assert_true(isCDataCommentOrPIStart(commentOpen, expectedCommentOpenLen,
                                      &actualCommentOpenLen)
              == XML_TOK_COMMENT);
  assert_true(actualCommentOpenLen == expectedCommentOpenLen);

  size_t actualPiOpenLen = 0;
  assert_true(
      isCDataCommentOrPIStart(piOpen, expectedPiOpenLen, &actualPiOpenLen)
      == XML_TOK_PI);
  assert_true(actualCdataOpenLen == expectedCdataOpenLen);

  size_t dummy = 0;
  assert_true(isCDataCommentOrPIStart(piOpen, 0, &dummy) == XML_TOK_INVALID);
  assert_true(isCDataCommentOrPIStart(piOpen, 1, &dummy) == XML_TOK_INVALID);
#endif // XML_GE == 1

  free(cdataOpen);
  free(commentOpen);
  free(piOpen);
}
END_TEST

void
make_entities_test_case(Suite *s) {
  TCase *const tc_entities = tcase_create("entities tests");
  suite_add_tcase(s, tc_entities);
  tcase_add_test(tc_entities, test_is_cdata_comment_or_pi_start);
}
