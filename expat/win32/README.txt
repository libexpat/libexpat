
Expat can be built on Windows in three ways: 
  using MS Visual C++ 6, Borland C++ Builder 5 or Cygwin.

* Cygwin:
  This follows the Unix build procedures.

* C++ Builder 5:
  Possible with make files in the BCB5 subdirectory.
  Details can be found in the ReadMe file located there.

* MS Visual C++ 6:
  Based on workspace (.dsw) and project files (.dsp)
  located in the lib subdirectory.

* Special note about building static libraries under MS VC++:

  There are three possible configurations: using the
  single threaded or multithreaded run-time library,
  or using the multi-threaded run-time Dll. That is, 
  one can build three different Expat libraries depending
  on the needs of the application.

  The libraries should be named like this:
  Single-theaded:     libexpatML.lib
  Multi-threaded:     libexpatMT.lib
  Multi-threaded Dll: libexpatMD.lib
  The suffixes conform to the compiler switch settings
  /ML, /MT and /MD for MS VC++.

  By default, the expat-static and expatw-static projects
  are set up to link against the multithreaded run-time library,
  so they will build libexpatMT.lib or libexpatwMT.lib files.

  To build the other versions of the static library, 
  go to Project - Settings:
  - specify a different RTL linkage on the C/C++ tab
    under the category Code Generation.
  - then, on the Library tab, change the output file name
    accordingly, as described above

  An application linking to the static libraries must
  have the global macro XML_STATIC defined.
   @
