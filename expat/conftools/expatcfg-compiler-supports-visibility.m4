# expatcfg-compiler-supports-visibility.m4 --
#
# SYNOPSIS
#
#    EXPATCFG_COMPILER_SUPPORTS_VISIBILITY([ACTION-IF-YES],
#                                          [ACTION-IF-NO])
#
# DESCRIPTION
#
#   Check if  the selected compiler supports  the "visibility" attribute
#   and  set   the  variable  "expatcfg_cv_compiler_supports_visibility"
#   accordingly to "yes" or "no".
#
#   In addition, execute ACTION-IF-YES or ACTION-IF-NO.
#
# LICENSE
#
# Copyright (c) 1998-2000 Thai Open Source Software Center Ltd and Clark Cooper
# Copyright (c) 2001-2018 Expat maintainers
#
# Permission is hereby granted, free  of charge, to any person obtaining
# a  copy  of this  software  and  associated documentation  files  (the
# "Software"), to  deal in  the Software without  restriction, including
# without limitation  the rights to  use, copy, modify,  merge, publish,
# distribute, sublicense,  and/or sell  copies of  the Software,  and to
# permit persons to whom the Software  is furnished to do so, subject to
# the following conditions:
#
# The  above  copyright  notice  and this  permission  notice  shall  be
# included in all copies or substantial portions of the Software.
#
# THE  SOFTWARE IS  PROVIDED  "AS  IS", WITHOUT  WARRANTY  OF ANY  KIND,
# EXPRESS OR  IMPLIED, INCLUDING  BUT NOT LIMITED  TO THE  WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT  SHALL THE AUTHORS OR COPYRIGHT HOLDERS  BE LIABLE FOR ANY
# CLAIM, DAMAGES OR  OTHER LIABILITY, WHETHER IN AN  ACTION OF CONTRACT,
# TORT OR  OTHERWISE, ARISING  FROM, OUT  OF OR  IN CONNECTION  WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

AC_DEFUN([EXPATCFG_COMPILER_SUPPORTS_VISIBILITY],
  [AC_CACHE_CHECK([whether compiler supports visibility],
     [expatcfg_cv_compiler_supports_visibility],
     [AS_VAR_SET([expatcfg_cv_compiler_supports_visibility],[no])
      AS_VAR_COPY([OLDFLAGS],[CFLAGS])
      AS_VAR_APPEND([CFLAGS],[" -fvisibility=hidden -Wall -Werror"])
      AC_COMPILE_IFELSE([AC_LANG_SOURCE([[void __attribute__((visibility("default"))) foo(void); void foo(void) {}]])],
        [AS_VAR_SET([expatcfg_cv_compiler_supports_visibility],[yes])])
      AS_VAR_COPY([CFLAGS],[OLDFLAGS])])
   AS_IF([test "$expatcfg_cv_compiler_supports_visibility" = yes],[$1],[$2])])

# end of file
