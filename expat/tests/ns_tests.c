/* Tests in the "namespace" test case of the Expat test suite
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
#include <string.h>

#if ! defined(__cplusplus)
#  include <stdbool.h>
#endif

#include "expat.h"
#include "internal.h" /* For UNUSED_P() */
#include "minicheck.h"
#include "chardata.h"
#include "structdata.h"
#include "common.h"
#include "dummy.h"
#include "handlers.h"
#include "ns_tests.h"


START_TEST(test_return_ns_triplet) {
  const char *text = "<foo:e xmlns:foo='http://example.org/' bar:a='12'\n"
                     "       xmlns:bar='http://example.org/'>";
  const char *epilog = "</foo:e>";
  const XML_Char *elemstr[]
      = {XCS("http://example.org/ e foo"), XCS("http://example.org/ a bar")};
  XML_SetReturnNSTriplet(g_parser, XML_TRUE);
  XML_SetUserData(g_parser, (void *)elemstr);
  XML_SetElementHandler(g_parser, triplet_start_checker, triplet_end_checker);
  XML_SetNamespaceDeclHandler(g_parser, dummy_start_namespace_decl_handler,
                              dummy_end_namespace_decl_handler);
  g_triplet_start_flag = XML_FALSE;
  g_triplet_end_flag = XML_FALSE;
  init_dummy_handlers();
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_FALSE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  if (! g_triplet_start_flag)
    fail("triplet_start_checker not invoked");
  /* Check that unsetting "return triplets" fails while still parsing */
  XML_SetReturnNSTriplet(g_parser, XML_FALSE);
  if (_XML_Parse_SINGLE_BYTES(g_parser, epilog, (int)strlen(epilog), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  if (! g_triplet_end_flag)
    fail("triplet_end_checker not invoked");
  if (get_dummy_handler_flags()
      != (DUMMY_START_NS_DECL_HANDLER_FLAG | DUMMY_END_NS_DECL_HANDLER_FLAG))
    fail("Namespace handlers not called");
}
END_TEST

/* Test that the parsing status is correctly reset by XML_ParserReset().
 * We usE test_return_ns_triplet() for our example parse to improve
 * coverage of tidying up code executed.
 */
START_TEST(test_ns_parser_reset) {
  XML_ParsingStatus status;

  XML_GetParsingStatus(g_parser, &status);
  if (status.parsing != XML_INITIALIZED)
    fail("parsing status doesn't start INITIALIZED");
  test_return_ns_triplet();
  XML_GetParsingStatus(g_parser, &status);
  if (status.parsing != XML_FINISHED)
    fail("parsing status doesn't end FINISHED");
  XML_ParserReset(g_parser, NULL);
  XML_GetParsingStatus(g_parser, &status);
  if (status.parsing != XML_INITIALIZED)
    fail("parsing status doesn't reset to INITIALIZED");
}
END_TEST

static void
run_ns_tagname_overwrite_test(const char *text, const XML_Char *result) {
  CharData storage;
  CharData_Init(&storage);
  XML_SetUserData(g_parser, &storage);
  XML_SetElementHandler(g_parser, overwrite_start_checker,
                        overwrite_end_checker);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
  CharData_CheckXMLChars(&storage, result);
}

/* Regression test for SF bug #566334. */
START_TEST(test_ns_tagname_overwrite) {
  const char *text = "<n:e xmlns:n='http://example.org/'>\n"
                     "  <n:f n:attr='foo'/>\n"
                     "  <n:g n:attr2='bar'/>\n"
                     "</n:e>";
  const XML_Char *result = XCS("start http://example.org/ e\n")
      XCS("start http://example.org/ f\n")
          XCS("attribute http://example.org/ attr\n")
              XCS("end http://example.org/ f\n")
                  XCS("start http://example.org/ g\n")
                      XCS("attribute http://example.org/ attr2\n")
                          XCS("end http://example.org/ g\n")
                              XCS("end http://example.org/ e\n");
  run_ns_tagname_overwrite_test(text, result);
}
END_TEST

/* Regression test for SF bug #566334. */
START_TEST(test_ns_tagname_overwrite_triplet) {
  const char *text = "<n:e xmlns:n='http://example.org/'>\n"
                     "  <n:f n:attr='foo'/>\n"
                     "  <n:g n:attr2='bar'/>\n"
                     "</n:e>";
  const XML_Char *result = XCS("start http://example.org/ e n\n")
      XCS("start http://example.org/ f n\n")
          XCS("attribute http://example.org/ attr n\n")
              XCS("end http://example.org/ f n\n")
                  XCS("start http://example.org/ g n\n")
                      XCS("attribute http://example.org/ attr2 n\n")
                          XCS("end http://example.org/ g n\n")
                              XCS("end http://example.org/ e n\n");
  XML_SetReturnNSTriplet(g_parser, XML_TRUE);
  run_ns_tagname_overwrite_test(text, result);
}
END_TEST

/* Regression test for SF bug #620343. */
static void XMLCALL
start_ns_clearing_start_element(void *userData, const XML_Char *prefix,
                                const XML_Char *uri) {
  UNUSED_P(prefix);
  UNUSED_P(uri);
  XML_SetStartElementHandler((XML_Parser)userData, NULL);
}

START_TEST(test_start_ns_clears_start_element) {
  /* This needs to use separate start/end tags; using the empty tag
     syntax doesn't cause the problematic path through Expat to be
     taken.
  */
  const char *text = "<e xmlns='http://example.org/'></e>";

  XML_SetStartElementHandler(g_parser, start_element_fail);
  XML_SetStartNamespaceDeclHandler(g_parser, start_ns_clearing_start_element);
  XML_SetEndNamespaceDeclHandler(g_parser, dummy_end_namespace_decl_handler);
  XML_UseParserAsHandlerArg(g_parser);
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_ERROR)
    xml_failure(g_parser);
}
END_TEST

TCase *
make_namespace_test_case(Suite *s) {
  TCase *tc_namespace = tcase_create("XML namespaces");

  suite_add_tcase(s, tc_namespace);
  tcase_add_checked_fixture(tc_namespace, namespace_setup, namespace_teardown);
  tcase_add_test(tc_namespace, test_return_ns_triplet);
  tcase_add_test(tc_namespace, test_ns_tagname_overwrite);
  tcase_add_test(tc_namespace, test_ns_tagname_overwrite_triplet);
  tcase_add_test(tc_namespace, test_start_ns_clears_start_element);

  tcase_add_test(tc_namespace, test_ns_parser_reset);

  return tc_namespace;
}
