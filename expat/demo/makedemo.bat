@echo off
set LIB=..\xmlparse\Release;..\xmltok\Release;..\lib;%LIB%
set INCLUDE=..\xmlparse;..\xmltok;%INCLUDE%
set CL=/nologo /DXMLTOKAPI=__declspec(dllimport) /DXMLPARSEAPI=__declspec(dllimport) xmlparse.lib xmltok.lib
cl /Fe..\bin\demo demo.c
