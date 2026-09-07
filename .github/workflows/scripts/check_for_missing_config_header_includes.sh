#! /usr/bin/env bash
#                          __  __            _
#                       ___\ \/ /_ __   __ _| |_
#                      / _ \\  /| '_ \ / _` | __|
#                     |  __//  \| |_) | (_| | |_
#                      \___/_/\_\ .__/ \__,_|\__|
#                               |_| XML parser
#
# Copyright (c) 2026 Sebastian Pipping <sebastian@pipping.org>
# Licensed under the MIT license:
#
# Permission is  hereby granted,  free of charge,  to any  person obtaining
# a  copy  of  this  software   and  associated  documentation  files  (the
# "Software"),  to  deal in  the  Software  without restriction,  including
# without  limitation the  rights  to use,  copy,  modify, merge,  publish,
# distribute, sublicense, and/or sell copies of the Software, and to permit
# persons  to whom  the Software  is  furnished to  do so,  subject to  the
# following conditions:
#
# The above copyright  notice and this permission notice  shall be included
# in all copies or substantial portions of the Software.
#
# THE  SOFTWARE  IS  PROVIDED  "AS  IS",  WITHOUT  WARRANTY  OF  ANY  KIND,
# EXPRESS  OR IMPLIED,  INCLUDING  BUT  NOT LIMITED  TO  THE WARRANTIES  OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
# NO EVENT SHALL THE AUTHORS OR  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR  OTHER LIABILITY, WHETHER  IN AN  ACTION OF CONTRACT,  TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
# USE OR OTHER DEALINGS IN THE SOFTWARE.

set -e -u -o pipefail

macros="$(cat .github/workflows/data/expat_config_h_cmake__expected.txt \
              .github/workflows/data/expat_config_h_in__expected.txt \
            | sort -u \
                | grep -v \
                    -e __func__ \
                    -e const)"
pattern=$(echo -n "${macros}" | tr '\n' '|')

# NOTE: Making expat.h include expat_config.h now would pull all those macros into user code
#       and potentially break things for them
#       The internal headers are fine without an include to expat_config.h, as long as the
#       file including them includes expat_config.h prior
files_expected="$(git grep -l -w -E "${pattern}" -- expat/lib/ expat/tests/ expat/xmlwf/ \
                    | grep -v -F \
                        -e expat/lib/expat.h \
                        -e expat/lib/internal.h \
                        -e expat/lib/xmlrole.h \
                        -e expat/lib/xmltok.h \
                        -e expat/lib/xmltok_impl.c \
                        -e expat/lib/xmltok_ns.c)"
files_actual="$(git grep -l '^ *# *include.*expat_config.h' -- expat/lib/ expat/tests/ expat/xmlwf/)"

files_missing_include="$(grep -v -F -f <(echo "${files_actual}") <<<"${files_expected}" || true)"

if [[ -n "${files_missing_include}" ]]; then
  for filename in ${files_missing_include}; do
    echo "[${filename}]"
    git -c color.ui=always grep -h -n -E "${pattern}" -- ${filename} | sed 's,^,  ,'
  done

  exit 1
fi
