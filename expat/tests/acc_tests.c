/* Tests in the "accounting" test case for the Expat test suite
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

#include <stdio.h>
#include <string.h>

#include "expat_config.h"

#include "expat.h"
#include "internal.h"
#include "common.h"
#include "minicheck.h"
#include "chardata.h"
#include "handlers.h"
#include "acc_tests.h"

#if defined(XML_DTD)
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
portableNAN(void) {
  return strtof("nan", NULL);
}

static float
portableINFINITY(void) {
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
    set_subtest("char %u", (unsigned)uc);
    const char *const printable = unsignedCharToPrintable(uc);
    if (printable == NULL)
      fail("unsignedCharToPrintable returned NULL");
    if (strlen(printable) < (size_t)1)
      fail("unsignedCharToPrintable returned empty string");
  }

  // Two concrete samples
  set_subtest("char 'A'");
  if (strcmp(unsignedCharToPrintable('A'), "A") != 0)
    fail("unsignedCharToPrintable result mistaken");
  set_subtest("char '\\'");
  if (strcmp(unsignedCharToPrintable('\\'), "\\\\") != 0)
    fail("unsignedCharToPrintable result mistaken");
}
END_TEST
#endif // defined(XML_DTD)

void
make_accounting_test_case(Suite *s) {
#if defined(XML_DTD)
  TCase *tc_accounting = tcase_create("accounting tests");

  suite_add_tcase(s, tc_accounting);

  tcase_add_test(tc_accounting, test_accounting_precision);
  tcase_add_test(tc_accounting, test_billion_laughs_attack_protection_api);
  tcase_add_test(tc_accounting, test_helper_unsigned_char_to_printable);
#else
  UNUSED_P(s);
#endif /* defined(XML_DTD) */
}
