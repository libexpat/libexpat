#! /bin/bash
#                          __  __            _
#                       ___\ \/ /_ __   __ _| |_
#                      / _ \\  /| '_ \ / _` | __|
#                     |  __//  \| |_) | (_| | |_
#                      \___/_/\_\ .__/ \__,_|\__|
#                               |_| XML parser
#
# Copyright (c) 2017 Expat development team
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

if [[ ${TRAVIS_OS_NAME} = osx ]]; then
    export PATH="/usr/local/opt/coreutils/libexec/gnubin${PATH:+:}${PATH}"
elif [[ ${TRAVIS_OS_NAME} = linux ]]; then
    export PATH="/usr/lib/llvm-9/bin:${PATH}"
fi

PS4='# '
set -x

cd expat
./buildconf.sh

if [[ ${MODE} = distcheck ]]; then
    ./configure ${CONFIGURE_ARGS}
    make distcheck
elif [[ ${MODE} = cmake-oos ]]; then
    mkdir build
    cd build
    cmake ${CMAKE_ARGS} ..
    make VERBOSE=1 all test
    make DESTDIR="${PWD}"/ROOT install
    find ROOT -printf "%P\n" | sort
elif [[ ${MODE} = cppcheck ]]; then
    cppcheck --quiet --error-exitcode=1 .
elif [[ ${MODE} = clang-format ]]; then
    ./apply-clang-format.sh
    git diff --exit-code
elif [[ ${MODE} = coverage-sh ]]; then
    ./coverage.sh
else
    ./qa.sh ${CMAKE_ARGS}
fi
