Short:    Expat XML parsing library
Author:   James Clark
Uploader: ssolie@telus.net (Steven Solie)
Type:     dev/misc
Version:  1.95.8
Requires: AmigaOS 4.0, SDK 51.5, clib2 1.188

This is a port of expat for AmigaOS 4.0 PPC.

Currently clib2 is supported although it should be possible to use
the library with newlib (e.g. add the -newlib option to GCC).


Building:
---------
To build expat library, xmlwf tool, examples and run the test suite,
simply type 'make all'.

To install expat into the AmigaOS SDK type 'make install'.

To uninstall expat, type 'make uninstall'.

To run the test suite, type 'make check'.


Configuration:
--------------
You may want to edit the lib/amigaconfig.h file to remove DTD and/or
XML namespace support if they are not required by your application
for a slightly smaller and faster implementation.


To Do:
------
- wide character support (UTF-16)
- create a shared library option
