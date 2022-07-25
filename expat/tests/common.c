/* Commonly used functions for the Expat test suite
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

#include "expat.h"
#include "internal.h" /* for UNUSED_P */
#include "minicheck.h"
#include "common.h"


void
tcase_add_test__ifdef_xml_dtd(TCase *tc, tcase_test_function test) {
#ifdef XML_DTD
  tcase_add_test(tc, test);
#else
  UNUSED_P(tc);
  UNUSED_P(test);
#endif
}

void
basic_setup(void) {
  g_parser = XML_ParserCreate(NULL);
  if (g_parser == NULL)
    fail("Parser not created.");
}

void
basic_teardown(void) {
  if (g_parser != NULL) {
    XML_ParserFree(g_parser);
    g_parser = NULL;
  }
}

/* Generate a failure using the parser state to create an error message;
   this should be used when the parser reports an error we weren't
   expecting.
*/
void
_xml_failure(XML_Parser parser, const char *file, int line) {
  char buffer[1024];
  enum XML_Error err = XML_GetErrorCode(parser);
  sprintf(buffer,
          "    %d: %" XML_FMT_STR " (line %" XML_FMT_INT_MOD
          "u, offset %" XML_FMT_INT_MOD "u)\n    reported from %s, line %d\n",
          err, XML_ErrorString(err), XML_GetCurrentLineNumber(parser),
          XML_GetCurrentColumnNumber(parser), file, line);
  _fail_unless(0, file, line, buffer);
}

enum XML_Status
_XML_Parse_SINGLE_BYTES(XML_Parser parser, const char *s, int len,
                        int isFinal) {
  enum XML_Status res = XML_STATUS_ERROR;
  int offset = 0;

  if (len == 0) {
    return XML_Parse(parser, s, len, isFinal);
  }

  for (; offset < len; offset++) {
    const int innerIsFinal = (offset == len - 1) && isFinal;
    const char c = s[offset]; /* to help out-of-bounds detection */
    res = XML_Parse(parser, &c, sizeof(char), innerIsFinal);
    if (res != XML_STATUS_OK) {
      return res;
    }
  }
  return res;
}

void
_expect_failure(const char *text, enum XML_Error errorCode,
                const char *errorMessage, const char *file, int lineno) {
  if (_XML_Parse_SINGLE_BYTES(g_parser, text, (int)strlen(text), XML_TRUE)
      == XML_STATUS_OK)
    /* Hackish use of _fail_unless() macro, but let's us report
       the right filename and line number. */
    _fail_unless(0, file, lineno, errorMessage);
  if (XML_GetErrorCode(g_parser) != errorCode)
    _xml_failure(g_parser, file, lineno);
}

