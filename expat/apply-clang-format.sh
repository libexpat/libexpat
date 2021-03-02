#! /usr/bin/env bash
#                          __  __            _
#                       ___\ \/ /_ __   __ _| |_
#                      / _ \\  /| '_ \ / _` | __|
#                     |  __//  \| |_) | (_| | |_
#                      \___/_/\_\ .__/ \__,_|\__|
#                               |_| XML parser
#
# Copyright (c) 2019 Expat development team
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

set -e
set -u
set -o pipefail

clang-format --version

for filename in $(find \
        -name '*.[ch]' \
        -o -name '*.cpp' \
        -o -name '*.cxx' \
        -o -name '*.h.cmake' \
        | sort); do
    # We're expanding tabs here because some versions of clang-format
    # seem to leave leading tabs untouched despite our "UseTab: Never"
    # in file .clang-format.  The "odd" number is to increase the chance of
    # a visual difference.
    expand --tabs=5 --initial "${filename}" | sponge "${filename}"
    clang-format -i -style=file -verbose "${filename}"
done

sed \
        -e 's, @$,@,' \
        -e 's,#\( \+\)cmakedefine,\1#cmakedefine,' \
        -i \
        expat_config.h.cmake
