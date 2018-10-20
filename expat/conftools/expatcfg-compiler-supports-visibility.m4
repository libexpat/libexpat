dnl expatcfg-compiler-supports-visibility.m4 --
dnl
dnl SYNOPSIS
dnl
dnl    EXPAT_COMPILER_SUPPORTS_VISIBILITY([ACTION-IF-YES], [ACTION-IF-NO])
dnl

AC_DEFUN([EXPAT_COMPILER_SUPPORTS_VISIBILITY],
  [AC_MSG_CHECKING(whether compiler supports visibility)
   AS_VAR_COPY([OLDFLAGS],[CFLAGS])
   AS_VAR_APPEND(CFLAGS,[" -fvisibility=hidden -Wall -Werror"])
   AC_COMPILE_IFELSE([AC_LANG_SOURCE([[void __attribute__((visibility("default"))) foo(void); void foo(void) {}]])],
     [AC_MSG_RESULT(yes)
      AS_VAR_COPY([CFLAGS],[OLDFLAGS])
      $1],
     [AC_MSG_RESULT(no)
      AS_VAR_COPY([CFLAGS],[OLDFLAGS])
      $2])])

dnl end of file
