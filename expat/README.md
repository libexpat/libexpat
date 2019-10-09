[![Travis CI Build Status](https://travis-ci.org/libexpat/libexpat.svg?branch=master)](https://travis-ci.org/libexpat/libexpat)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/libexpat/libexpat?svg=true)](https://ci.appveyor.com/project/libexpat/libexpat)
[![Packaging status](https://repology.org/badge/tiny-repos/expat.svg)](https://repology.org/metapackage/expat/versions)


# Expat, Release 2.2.9

This is Expat, a C library for parsing XML, started by
[James Clark](https://en.wikipedia.org/wiki/James_Clark_(programmer)) in 1997.
Expat is a stream-oriented XML parser.  This means that you register
handlers with the parser before starting the parse.  These handlers
are called when the parser discovers the associated structures in the
document being parsed.  A start tag is an example of the kind of
structures for which you may register handlers.

Expat supports the following compilers:
- GNU GCC >=4.5
- LLVM Clang >=3.5
- Microsoft Visual Studio >=9.0/2008

Windows users can use the
[`expat_win32` package](https://sourceforge.net/projects/expat/files/expat_win32/),
which includes both precompiled libraries and executables, and source code for
developers.

Expat is [free software](https://www.gnu.org/philosophy/free-sw.en.html).
You may copy, distribute, and modify it under the terms of the License
contained in the file
[`COPYING`](https://github.com/libexpat/libexpat/blob/master/expat/COPYING)
distributed with this package.
This license is the same as the MIT/X Consortium license.


## Build and install

To build Expat from a source distribution, you first run CMake
in the top level distribution directory:

```console
cmake .
```

There are many options which you may provide to CMake (which you
can discover by running `cmake -LH .`).  But the
one of most interest is the one that sets the installation directory.
By default, CMake will set things up to install
libexpat into `/usr/local/lib`, `expat.h` into `/usr/local/include`, and
`xmlwf` into `/usr/local/bin`.  If, for example, you'd prefer to install
into `/home/me/mystuff/lib`, `/home/me/mystuff/include`, and
`/home/me/mystuff/bin`, you can tell CMake about that with:

```console
cmake -DCMAKE_INSTALL_PREFIX=/home/me/mystuff .
```

Another interesting option is to enable 64-bit integer support for
line and column numbers and the over-all byte index:

```console
cmake -DEXPAT_LARGE_SIZE=ON .
```

However, such a modification would be a breaking change to the ABI
and is therefore not recommended for general use &mdash; e.g. as part of
a Linux distribution &mdash; but rather for builds with special requirements.

After running CMake, the `make` command will build
things and `make install` will install things into their proper
location. Note that you need to have write permission into
the directories into which things will be installed.


### Building with support for wide characters

If you are interested in building Expat to provide document
information in UTF-16 encoding rather than the default UTF-8,
pass `-DEXPAT_CHAR_TYPE=ushort` or `-DEXPAT_CHAR_TYPE=wchar_t` to CMake.
Version and error strings remain to be `char`.

Please note that `-DEXPAT_CHAR_TYPE=ushort` will also need

- `-DEXPAT_BUILD_EXAMPLES=OFF`
- `-DEXPAT_BUILD_TESTS=OFF`
- `-DEXPAT_BUILD_TOOLS=OFF`

because `unsigned short` based strings are not supported in these contexts.

For `-DEXPAT_CHAR_TYPE=wchar_t`, please note that `wchar_t` takes 2 bytes on
Windows &mdash; a *good* fit for UTF-16 &mdash; but 4 bytes on Unix &mdash;
a *bad* fit for UTF-16.
As a result, on Unix you will need to

- have a libc compiled with `-fshort-wchar` around(!),
- pass `-DCMAKE_{C,CXX}_FLAGS=-fshort-wchar` to CMake, and
- pass `-DEXPAT_BUILD_TOOLS=OFF` to CMake
  (because xmlwf does not support to be compiled this way).


### Installation with DESTDIR

Using `DESTDIR` is supported.  It works as follows:

```console
make install DESTDIR=/path/to/image
```

overrides the in-makefile set `DESTDIR`, because variable-setting priority is

1. commandline
1. in-makefile
1. environment

Note: This only applies to the Expat library itself, building UTF-16 versions
of xmlwf and the tests is currently not supported.

When using Expat with a project using autoconf for configuration, you
can use the probing macro in `conftools/expat.m4` to determine how to
include Expat.  See the comments at the top of that file for more
information.

A reference manual is available in the file `doc/reference.html` in this
distribution.


### CMake build options

For an idea of the available (non-advanced) options for building with CMake:

```console
# rm -f CMakeCache.txt ; cmake -D_EXPAT_HELP=ON -LH . | grep -B1 ':.*=' | sed 's,^--$,,'
// Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel ...
CMAKE_BUILD_TYPE:STRING=

// Install path prefix, prepended onto install directories.
CMAKE_INSTALL_PREFIX:PATH=/usr/local

// Path to a program.
DOCBOOK_TO_MAN:FILEPATH=/usr/bin/docbook2x-man

// build man page for xmlwf
EXPAT_BUILD_DOCS:BOOL=ON

// build the examples for expat library
EXPAT_BUILD_EXAMPLES:BOOL=ON

// build fuzzers for the expat library
EXPAT_BUILD_FUZZERS:BOOL=OFF

// build pkg-config file
EXPAT_BUILD_PKGCONFIG:BOOL=ON

// build the tests for expat library
EXPAT_BUILD_TESTS:BOOL=ON

// build the xmlwf tool for expat library
EXPAT_BUILD_TOOLS:BOOL=ON

// Character type to use (char|ushort|wchar_t) [default=char]
EXPAT_CHAR_TYPE:STRING=char

// install expat files in cmake install target
EXPAT_ENABLE_INSTALL:BOOL=ON

// Use /MT flag (static CRT) when compiling in MSVC
EXPAT_MSVC_STATIC_CRT:BOOL=OFF

// build fuzzers via ossfuzz for the expat library
EXPAT_OSSFUZZ_BUILD:BOOL=OFF

// build a shared expat library
EXPAT_SHARED_LIBS:BOOL=ON

// Treat all compiler warnings as errors
EXPAT_WARNINGS_AS_ERRORS:BOOL=OFF

// Make use of getrandom function (ON|OFF|AUTO) [default=AUTO]
EXPAT_WITH_GETRANDOM:STRING=AUTO

// utilize libbsd (for arc4random_buf)
EXPAT_WITH_LIBBSD:BOOL=OFF

// Make use of syscall SYS_getrandom (ON|OFF|AUTO) [default=AUTO]
EXPAT_WITH_SYS_GETRANDOM:STRING=AUTO
```
