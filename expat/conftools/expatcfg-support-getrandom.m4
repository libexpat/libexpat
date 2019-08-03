# expatcfg-support-getrandom.m4 -- -*- mode: autoconf -*-
#
# SYNOPSIS
#
#    EXPATCFG_SUPPORT_GETRANDOM
#
# DESCRIPTION
#
#   Enable support for the function "getrandom" or the function
#   "SYS_getrandom".
#
#   The cached variable "expatcfg_cv_support_getrandom" is set
#   accordingly to: "no", "getrandom", "SYS_getrandom".
#
#   The       preprocessor       symbols      "HAVE_GETRANDOM"       and
#   "HAVE_SYSCALL_GETRANDOM" are defined or undefined accordingly.
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

AC_DEFUN([EXPATCFG_SUPPORT_GETRANDOM],[
  AC_CACHE_CHECK([enable support for function getrandom (Linux 3.17+, glibc 2.25+) or syscall SYS_getrandom (Linux 3.17+)],
    [expatcfg_cv_support_getrandom],
    [AS_VAR_SET([expatcfg_cv_support_getrandom],[no])

     # Try  to  link  with  "getrandom",  if  successful  we  are  done.
     # Otherwise try to link with "SYS_getrandom".
     AC_LINK_IFELSE([AC_LANG_SOURCE([
#include <stdlib.h>  /* for NULL */
#include <sys/random.h>
int main() {
  return getrandom(NULL, 0U, 0U);
}
])],
       [AS_VAR_SET([expatcfg_cv_support_getrandom],[getrandom])],
       [AC_LINK_IFELSE([AC_LANG_SOURCE([
#include <stdlib.h>  /* for NULL */
#include <unistd.h>  /* for syscall */
#include <sys/syscall.h>  /* for SYS_getrandom */
int main() {
  syscall(SYS_getrandom, NULL, 0, 0);
  return 0;
}
])],
          [AS_VAR_SET([expatcfg_cv_support_getrandom],[SYS_getrandom])])])

    # End of cached variable definition.
    ])

    # Whatever the  value of  the cached variable:  appropriately define
    # the preprocessor symbols.
    AS_CASE([expatcfg_cv_support_getrandom],
      [getrandom],     [AC_DEFINE([HAVE_GETRANDOM],         [1], [Define to 1 if you have the `getrandom' function.])],
      [SYS_getrandom], [AC_DEFINE([HAVE_SYSCALL_GETRANDOM], [1], [Define to 1 if you have `syscall' and `SYS_getrandom'.])])
  ])

### end of file
