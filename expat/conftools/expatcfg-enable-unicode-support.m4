# expatcfg-compiler-supports-visibility.m4 -- -*- mode: autoconf -*-
#
# SYNOPSIS
#
#    EXPATCFG_ENABLE_UNICODE_SUPPORT
#
# DESCRIPTION
#
#   Expand to the code needed to check for Unicode support.
#
# LICENSE
#
#   Copyright (c) 2018 The Expat Authors.
#
#   Copying and distribution of this file, with or without modification,
#   are permitted in  any medium without royalty  provided the copyright
#   notice and this  notice are preserved.  This file  is offered as-is,
#   without any warranty.

AC_DEFUN([EXPATCFG_ENABLE_UNICODE_SUPPORT],[
  AX_REQUIRE_DEFINED([AX_APPEND_COMPILE_FLAGS])
  # Check if the caller requested Unicode support.
  #
  # 1. Define  the configuration status  variables and the  command line
  # arguments to "configure".
  #
  AC_CACHE_CHECK([whether to enable UTF-16 output as unsigned short],
    [expatcfg_cv_xml_unicode],
    [AS_VAR_SET([expatcfg_cv_xml_unicode],[no])
     AC_ARG_ENABLE([xml-unicode],
       [AS_HELP_STRING([--enable-xml-unicode],[enable UTF-16 output as unsigned short])],
       [AS_VAR_SET([expatcfg_cv_xml_unicode],[yes])])])
  AC_CACHE_CHECK([whether enable UTF-16 output as wchar_t],
    [expatcfg_cv_xml_unicode_wchar],
    [AS_VAR_SET([expatcfg_cv_xml_unicode_wchar],[no])
     AC_ARG_ENABLE([xml-unicode-wchar],
       [AS_HELP_STRING([--enable-xml-unicode-wchar],[enable UTF-16 output as wchar_t])],
       [AS_VAR_SET([expatcfg_cv_xml_unicode_wchar],[yes])])])
  #
  # 2. Check for mutually exclusive options.
  #
  AS_IF([test "x${expatcfg_cv_xml_unicode}" = xyes -a "x${expatcfg_cv_xml_unicode_wchar}" = xyes],
    [AC_MSG_ERROR([only one option for Unicode support must be enabled],[1])])
  AS_IF([test "x${expatcfg_cv_xml_unicode}" = xyes -o "x${expatcfg_cv_xml_unicode_wchar}" = xyes],
    [AS_VAR_IF([with_xmlwf],[yes],
       [AC_MSG_ERROR([xmlwf cannot be built with Unicode support enabled],[1])])])
  #
  # 3.  Define the  preprocessor symbols  and add  the required  special
  # flags for the compiler.
  #
  AS_VAR_IF([expatcfg_cv_xml_unicode],[yes],
    [AC_DEFINE([XML_UNICODE],[1],[enable UTF-16 output as unsigned short])])
  AS_VAR_IF([expatcfg_cv_xml_unicode_wchar],[yes],
    [AC_DEFINE([XML_UNICODE_WCHAR_T],[1],[enable UTF-16 output as wchar_t])
     AX_APPEND_COMPILE_FLAGS([-fshort-wchar], [CFLAGS])])
  ])

### end of file
