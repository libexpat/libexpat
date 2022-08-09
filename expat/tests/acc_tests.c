/* Tests in the "accounting" test case of the Expat test suite
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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

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
#include "acc_tests.h"

TCase *
make_accounting_test_case(Suite *s) {
#if defined(XML_DTD)
  TCase *tc_accounting = tcase_create("accounting tests");

  suite_add_tcase(s, tc_accounting);

  return tc_accounting;
#else
  return NULL;
#endif
}
