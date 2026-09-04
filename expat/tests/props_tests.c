/* Tests related to the public properties API of Expat
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

#include "props_tests.h"

#include "common.h" // for g_chunkSize
#include "expat.h"
#include "internal.h" // for e.g. EXPAT_ALLOC_TRACKER_ACTIVATION_THRESHOLD_DEFAULT
#include "expat.h"

START_TEST(test_props_getter_defaults) {
  // The test is not doing any parsing, so a single run
  // (with `g_chunkSize == 0`) is enough
  if (g_chunkSize != 0)
    return;

  XML_Parser parser = XML_ParserCreate(NULL);

#if XML_GE == 1
  // Case XML_PROP_ALLOC_TRACKER_ACTIVATION_THRESHOLD
  {
    uint64_t actionThresholdBytes1 = 123;
    assert_true(XML_GetPropertyUInt64(
                    parser, XML_PROP_ALLOC_TRACKER_ACTIVATION_THRESHOLD,
                    &actionThresholdBytes1)
                == XML_PROP_ERROR_NONE);
    assert_true(actionThresholdBytes1
                == EXPAT_ALLOC_TRACKER_ACTIVATION_THRESHOLD_DEFAULT);
  }

  // Case XML_PROP_ALLOC_TRACKER_MAXIMUM_AMPLIFICATION
  {
    double maximumAmplification1 = 123.456;
    assert_true(XML_GetPropertyDouble(
                    parser, XML_PROP_ALLOC_TRACKER_MAXIMUM_AMPLIFICATION,
                    &maximumAmplification1)
                == XML_PROP_ERROR_NONE);
    assert_true(maximumAmplification1
                == EXPAT_ALLOC_TRACKER_MAXIMUM_AMPLIFICATION_DEFAULT);
  }

  // Case XML_PROP_BILLION_LAUGHS_ACTIVATION_THRESHOLD
  {
    uint64_t actionThresholdBytes2 = 123;
    assert_true(XML_GetPropertyUInt64(
                    parser, XML_PROP_BILLION_LAUGHS_ACTIVATION_THRESHOLD,
                    &actionThresholdBytes2)
                == XML_PROP_ERROR_NONE);
    assert_true(
        actionThresholdBytes2
        == EXPAT_BILLION_LAUGHS_ATTACK_PROTECTION_ACTIVATION_THRESHOLD_DEFAULT);
  }

  // Case XML_PROP_ALLOC_TRACKER_MAXIMUM_AMPLIFICATION
  {
    double maximumAmplification2 = 123.456;
    assert_true(XML_GetPropertyDouble(
                    parser, XML_PROP_BILLION_LAUGHS_MAXIMUM_AMPLIFICATION,
                    &maximumAmplification2)
                == XML_PROP_ERROR_NONE);
    assert_true(
        maximumAmplification2
        == EXPAT_BILLION_LAUGHS_ATTACK_PROTECTION_MAXIMUM_AMPLIFICATION_DEFAULT);
  }
#endif

  // Case XML_PROP_REPARSE_DEFERRAL_ENABLED
  XML_Bool reparseDeferralEnabled = XML_FALSE;
  assert_true(XML_GetPropertyBool(parser, XML_PROP_REPARSE_DEFERRAL_ENABLED,
                                  &reparseDeferralEnabled)
              == XML_PROP_ERROR_NONE);
  assert_true(reparseDeferralEnabled == g_reparseDeferralEnabledDefault);

  // Case XML_PROP_INVALID
  XML_Bool dummyBool = XML_FALSE;
  double dummyDouble = 123.456;
  uint64_t dummyUInt64 = 123;
  assert_true(XML_GetPropertyBool(parser, XML_PROP_INVALID, &dummyBool)
              == XML_PROP_ERROR_INVALID_KEY);
  assert_true(XML_GetPropertyDouble(parser, XML_PROP_INVALID, &dummyDouble)
              == XML_PROP_ERROR_INVALID_KEY);
  assert_true(XML_GetPropertyUInt64(parser, XML_PROP_INVALID, &dummyUInt64)
              == XML_PROP_ERROR_INVALID_KEY);

  XML_ParserFree(parser);
}
END_TEST

START_TEST(test_props_getter_error_parser_null) {
  // The test is not doing any parsing, so a single run
  // (with `g_chunkSize == 0`) is enough
  if (g_chunkSize != 0)
    return;

  XML_Parser parserNonNull = XML_ParserCreate(NULL);
  XML_Parser parsers[] = {parserNonNull, NULL};

  for (size_t i = 0; i < sizeof(parsers) / sizeof(parsers[0]); i++) {
    XML_Parser parser = parsers[i];
    const enum XML_Prop_Error validKeyExpectedError
        = (parser == NULL) ? XML_PROP_ERROR_PARSER_NULL : XML_PROP_ERROR_NONE;
    // NOTE: Currently the parser is checked for being `NULL` before the key is
    //       being checked for being valid.
    //       That precedence among errors is not considered part of the API
    //       contract: either error would be fine to return.
    const enum XML_Prop_Error invalidKeyExpectedError
        = (parser == NULL) ? XML_PROP_ERROR_PARSER_NULL
                           : XML_PROP_ERROR_INVALID_KEY;

    XML_Bool dummyBool = XML_FALSE;
#if XML_GE == 1
    double dummyDouble = 123.456;
    uint64_t dummyUInt64 = 123;

    assert_true(
        XML_GetPropertyUInt64(
            parser, XML_PROP_ALLOC_TRACKER_ACTIVATION_THRESHOLD, &dummyUInt64)
        == validKeyExpectedError);
    assert_true(
        XML_GetPropertyDouble(
            parser, XML_PROP_ALLOC_TRACKER_MAXIMUM_AMPLIFICATION, &dummyDouble)
        == validKeyExpectedError);
    assert_true(
        XML_GetPropertyUInt64(
            parser, XML_PROP_BILLION_LAUGHS_ACTIVATION_THRESHOLD, &dummyUInt64)
        == validKeyExpectedError);
    assert_true(
        XML_GetPropertyDouble(
            parser, XML_PROP_BILLION_LAUGHS_MAXIMUM_AMPLIFICATION, &dummyDouble)
        == validKeyExpectedError);
#endif
    assert_true(XML_GetPropertyBool(parser, XML_PROP_REPARSE_DEFERRAL_ENABLED,
                                    &dummyBool)
                == validKeyExpectedError);

    assert_true(XML_GetPropertyBool(parser, XML_PROP_INVALID, &dummyBool)
                == invalidKeyExpectedError);
    assert_true(XML_GetPropertyDouble(parser, XML_PROP_INVALID, &dummyDouble)
                == invalidKeyExpectedError);
    assert_true(XML_GetPropertyUInt64(parser, XML_PROP_INVALID, &dummyUInt64)
                == invalidKeyExpectedError);
  }

  XML_ParserFree(parserNonNull);
}
END_TEST

void
make_props_test_case(Suite *s) {
  TCase *const tc_props = tcase_create("properties tests");
  suite_add_tcase(s, tc_props);
  tcase_add_test(tc_props, test_props_getter_defaults);
  tcase_add_test(tc_props, test_props_getter_error_parser_null);
}
