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
#   Copyright (c) 2018 The Expat Authors.
#
#   Copying and distribution of this file, with or without modification,
#   are permitted in  any medium without royalty  provided the copyright
#   notice and this  notice are preserved.  This file  is offered as-is,
#   without any warranty.

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
      [getrandom],     [AC_DEFINE([HAVE_GETRANDOM],         [1], [Define to 1 if you have the `getrandom' function.])]
      [SYS_getrandom], [AC_DEFINE([HAVE_SYSCALL_GETRANDOM], [1], [Define to 1 if you have `syscall' and `SYS_getrandom'.])])
  ])

### end of file
