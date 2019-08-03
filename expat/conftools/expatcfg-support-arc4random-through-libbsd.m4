# expatcfg-support-arc4random-through-libbsd.m4 -- -*- mode: autoconf -*-
#
# SYNOPSIS
#
#    EXPATCFG_SUPPORT_ARC4RANDOM_THROUGH_LIBBSD
#
# DESCRIPTION
#
#   Enable  support for  the function  "arc4random_buf" or  the function
#   "arc4random" through loading libbsd.
#
#   The   cached   variable  "expatcfg_cv_support_arc4random"   is   set
#   accordingly to: "no", "arc4random_buf", "arc4random".
#
#   The preprocessor symbols "HAVE_ARC4RANDOM_BUF" and "HAVE_ARC4RANDOM"
#   are defined or undefined accordingly.
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

AC_DEFUN([EXPATCFG_SUPPORT_ARC4RANDOM_THROUGH_LIBBSD],[
  # Always define the command line argument.
  AC_ARG_WITH([libbsd],
    [AS_HELP_STRING([--with-libbsd], [utilize libbsd (for arc4random_buf)])],
    [],
    [AS_VAR_SET([with_libbsd],[no])])

  # Define the cached variable.
  AC_CACHE_CHECK([enable support for function arc4random],
    [expatcfg_cv_support_arc4random],
    [AS_VAR_SET([expatcfg_cv_support_arc4random],[no])

     # We assume  "with_libbsd" can be  one among: yes, no,  check.  If
     # the library is not found: we  raise an error only when the value
     # is "yes".
     AS_IF([test "x${with_libbsd}" != xno],
       [AC_CHECK_LIB([bsd],
          [arc4random_buf],
          [AS_VAR_SET([expatcfg_cv_support_arc4random],[yes])],
          [AS_IF([test "x${with_libbsd}" = xyes],
                 [AC_MSG_ERROR([Enforced use of libbsd cannot be satisfied.])])])])

     # Whether libbsd can be loaded or not: we do not care here.  Try to
     # link with "arc4random_buf", if successful we are done.  Otherwise
     # try to link with "arc4random".
     AC_LINK_IFELSE([AC_LANG_SOURCE([
#include <stdlib.h>  /* for arc4random_buf on BSD, for NULL */
#if defined(HAVE_LIBBSD)
# include <bsd/stdlib.h>
#endif
int main() {
  arc4random_buf(NULL, 0U);
  return 0;
}
])],
       [AS_VAR_SET([expatcfg_cv_support_arc4random],[arc4random_buf])],
       [AC_LINK_IFELSE([AC_LANG_SOURCE([
#if defined(HAVE_LIBBSD)
# include <bsd/stdlib.h>
#else
# include <stdlib.h>
#endif
int main() {
    arc4random();
    return 0;
}
])],
          [AS_VAR_SET([expatcfg_cv_support_arc4random],[arc4random])])])

    # End of cached variable definition.
    ])

    # Whatever the  value of  the cached variable:  appropriately define
    # the preprocessor symbols.
    AS_CASE([expatcfg_cv_support_arc4random],
      [arc4random_buf], [AC_DEFINE([HAVE_ARC4RANDOM_BUF], [1], [Define to 1 if you have the `arc4random_buf' function.])],
      [arc4random],     [AC_DEFINE([HAVE_ARC4RANDOM],     [1], [Define to 1 if you have the `arc4random' function.])])
  ])

### end of file
