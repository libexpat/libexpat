# This file is copyrighted under the BSD-license for buildsystem files of KDE
# copyright 2010, Patrick Spendrin <ps_ml@gmx.de>

cmake_minimum_required(VERSION 3.1.3)

project(expat
    VERSION
        2.2.7
    LANGUAGES
        C
)

set(PACKAGE_BUGREPORT "expat-bugs@libexpat.org")
set(PACKAGE_NAME "expat")
set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME "${PACKAGE_NAME}")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

#
# Configuration defaults
#
if(WINCE)
    set(_EXPAT_BUILD_TOOLS_DEFAULT OFF)
else()
    set(_EXPAT_BUILD_TOOLS_DEFAULT ON)
endif()
if(MSVC OR NOT _EXPAT_BUILD_TOOLS_DEFAULT)
    set(BUILD_doc_default OFF)
else()
    find_program(DOCBOOK_TO_MAN NAMES docbook2x-man db2x_docbook2man docbook2man docbook-to-man)
    if(DOCBOOK_TO_MAN)
        set(BUILD_doc_default ON)
    else()
        set(BUILD_doc_default OFF)
    endif()
endif()

#
# Configuration
#
option(EXPAT_BUILD_TOOLS "build the xmlwf tool for expat library" ${_EXPAT_BUILD_TOOLS_DEFAULT})
option(EXPAT_BUILD_EXAMPLES "build the examples for expat library" ON)
option(BUILD_tests "build the tests for expat library" ON)
option(BUILD_shared "build a shared expat library" ON)
option(BUILD_doc "build man page for xmlwf" ${BUILD_doc_default})
option(BUILD_fuzzers "build fuzzers for the expat library" OFF)
option(USE_libbsd "utilize libbsd (for arc4random_buf)" OFF)
option(INSTALL "install expat files in cmake install target" ON)
set(XML_CONTEXT_BYTES 1024 CACHE STRING "Define to specify how much context to retain around the current parse point")
option(XML_DTD "Define to make parameter entity parsing functionality available" ON)
option(XML_NS "Define to make XML Namespaces functionality available" ON)
option(WARNINGS_AS_ERRORS "Treat all compiler warnings as errors" OFF)
if(NOT WIN32)
    option(XML_DEV_URANDOM "Define to include code reading entropy from `/dev/urandom'." ON)
    set(USE_GETRANDOM "AUTO" CACHE STRING
            "Make use of getrandom function (ON|OFF|AUTO) [default=AUTO]")
    set(USE_SYS_GETRANDOM "AUTO" CACHE STRING
            "Make use of syscall SYS_getrandom (ON|OFF|AUTO) [default=AUTO]")
endif()
option(XML_UNICODE "Use UTF-16 encoded chars (two bytes) instead of UTF-8" OFF)
option(XML_UNICODE_WCHAR_T "Use wchar_t to represent UTF-16 instead of unsigned short" OFF)
option(XML_ATTR_INFO "Define to allow retrieving the byte offsets for attribute names and values" OFF)
option(XML_LARGE_SIZE "Make XML_GetCurrent* functions return <(unsigned) long long> rather than <(unsigned) long>" OFF)
option(XML_MIN_SIZE "Get a smaller (but slower) parser (in particular avoid multiple copies of the tokenizer)" OFF)
if(MSVC)
    set(MSVC_USE_STATIC_CRT OFF CACHE BOOL "Use /MT flag (static CRT) when compiling in MSVC")
endif()

#
# Environment checks
#
if(USE_libbsd)
    find_library(LIB_BSD NAMES bsd)
    if(NOT LIB_BSD)
        message(SEND_ERROR "USE_libbsd option is enabled, but libbsd was not found")
    else()
        set(HAVE_LIBBSD TRUE)
    endif()
endif()

if(NOT WIN32)
    if(XML_DEV_URANDOM)
        set(XML_DEV_URANDOM 1)
    else(XML_DEV_URANDOM)
        set(XML_DEV_URANDOM 0)
    endif(XML_DEV_URANDOM)
endif()

macro(expat_bool_to_int var_ref)
    if(${var_ref})
        set(${var_ref} 1)
    else()
        set(${var_ref} 0)
    endif()
endmacro()

expat_bool_to_int(XML_ATTR_INFO)
expat_bool_to_int(XML_DTD)
expat_bool_to_int(XML_LARGE_SIZE)
expat_bool_to_int(XML_MIN_SIZE)
expat_bool_to_int(XML_NS)
expat_bool_to_int(XML_UNICODE)
expat_bool_to_int(XML_UNICODE_WCHAR_T)

if(XML_LARGE_SIZE)
    add_definitions(-DXML_LARGE_SIZE)
endif()

if(XML_MIN_SIZE)
    add_definitions(-DXML_MIN_SIZE)
endif()

if(XML_UNICODE_WCHAR_T AND NOT XML_UNICODE)
    message(SEND_ERROR "Option XML_UNICODE_WCHAR_T=ON may not be used without XML_UNICODE=ON.")
    set(XML_UNICODE 1)
endif(XML_UNICODE_WCHAR_T AND NOT XML_UNICODE)

if(EXPAT_BUILD_TOOLS AND (XML_UNICODE AND NOT XML_UNICODE_WCHAR_T))
    message(SEND_ERROR "The xmlwf tool can not be built with option XML_UNICODE_WCHAR_T=OFF and XML_UNICODE=ON. Either set XML_UNICODE_WCHAR_T=ON or EXPAT_BUILD_TOOLS=OFF.")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/ConfigureChecks.cmake)

macro(evaluate_detection_results use_ref have_ref thing_lower thing_title)
    if(${use_ref} AND NOT (${use_ref} STREQUAL "AUTO") AND NOT ${have_ref})
        message(SEND_ERROR
                "Use of ${thing_lower} was enforced by ${use_ref}=ON but it could not be found.")
    elseif(NOT ${use_ref} AND ${have_ref})
        message("${thing_title} was found but it will not be used due to ${use_ref}=OFF.")
        set(${have_ref} 0)
    endif()
endmacro()

if(NOT WIN32)
    evaluate_detection_results(USE_GETRANDOM HAVE_GETRANDOM "function getrandom" "Function getrandom")
    evaluate_detection_results(USE_SYS_GETRANDOM HAVE_SYSCALL_GETRANDOM "syscall SYS_getrandom" "Syscall SYS_getrandom")
endif()

macro(expat_install)
    if(INSTALL)
        install(${ARGN})
    endif()
endmacro()

configure_file(expat_config.h.cmake "${CMAKE_CURRENT_BINARY_DIR}/expat_config.h")
add_definitions(-DHAVE_EXPAT_CONFIG_H)
expat_install(FILES "${CMAKE_CURRENT_BINARY_DIR}/expat_config.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


set(EXTRA_COMPILE_FLAGS)
if(FLAG_NO_STRICT_ALIASING)
    set(EXTRA_COMPILE_FLAGS "${EXTRA_COMPILE_FLAGS} -fno-strict-aliasing")
endif()
if(FLAG_VISIBILITY)
  add_definitions(-DXML_ENABLE_VISIBILITY=1)
  set(EXTRA_COMPILE_FLAGS "${EXTRA_COMPILE_FLAGS} -fvisibility=hidden")
endif(FLAG_VISIBILITY)
if (WARNINGS_AS_ERRORS)
    if(MSVC)
        add_definitions(/WX)
    else(MSVC)
        set(EXTRA_COMPILE_FLAGS "${EXTRA_COMPILE_FLAGS} -Werror")
    endif(MSVC)
endif(WARNINGS_AS_ERRORS)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_COMPILE_FLAGS}")

if (MSVC)
    if (MSVC_USE_STATIC_CRT)
        message("-- Using static CRT ${MSVC_USE_STATIC_CRT}")
        foreach(flag_var
                CMAKE_CXX_FLAGS_DEBUG
                CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL
                CMAKE_CXX_FLAGS_RELWITHDEBINFO
                CMAKE_C_FLAGS_DEBUG
                CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL
                CMAKE_C_FLAGS_RELWITHDEBINFO
                )
            string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endforeach()
    endif()
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/lib)
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -wd4996)
endif(MSVC)
if(WIN32)
    if(XML_UNICODE_WCHAR_T)
        set(_POSTFIX_WIDE "w")
    endif()

    if(NOT BUILD_shared)
        if(MSVC_USE_STATIC_CRT)
            set(_POSTFIX_CRT "MT")
        else()
            set(_POSTFIX_CRT "MD")
        endif()
    endif()

    foreach(postfix_var
            CMAKE_DEBUG_POSTFIX
            CMAKE_RELEASE_POSTFIX
            CMAKE_MINSIZEREL_POSTFIX
            CMAKE_RELWITHDEBINFO_POSTFIX
            )
        if(postfix_var STREQUAL "CMAKE_DEBUG_POSTFIX")
            set(_POSTFIX_DEBUG "d")
        else()
            set(_POSTFIX_DEBUG "")
        endif()

        set(${postfix_var} "${_POSTFIX_WIDE}${_POSTFIX_DEBUG}${_POSTFIX_CRT}" CACHE STRING "Windows binary postfix, e.g. expat<postfix=[w][d][MD|MT]>.lib")
    endforeach()
endif(WIN32)

#
# C library
#
set(expat_SRCS
    lib/xmlparse.c
    lib/xmlrole.c
    lib/xmltok.c
    lib/xmltok_impl.c
    lib/xmltok_ns.c
)

if(BUILD_shared)
    set(_SHARED SHARED)
    if(MSVC)
        set(expat_SRCS ${expat_SRCS} lib/libexpat.def)
    endif(MSVC)
else(BUILD_shared)
    set(_SHARED STATIC)
    if(WIN32)
        add_definitions(-DXML_STATIC)
    endif(WIN32)
endif(BUILD_shared)

add_library(expat ${_SHARED} ${expat_SRCS})
if(USE_libbsd)
    target_link_libraries(expat ${LIB_BSD})
endif()

set(LIBCURRENT 7)   # sync
set(LIBREVISION 9)  # with
set(LIBAGE 6)       # configure.ac!
math(EXPR LIBCURRENT_MINUS_AGE "${LIBCURRENT} - ${LIBAGE}")

if(NOT WIN32)
    set_property(TARGET expat PROPERTY VERSION ${LIBCURRENT_MINUS_AGE}.${LIBAGE}.${LIBREVISION})
    set_property(TARGET expat PROPERTY SOVERSION ${LIBCURRENT_MINUS_AGE})
    set_property(TARGET expat PROPERTY NO_SONAME ${NO_SONAME})
endif(NOT WIN32)

expat_install(TARGETS expat EXPORT expat
                      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

expat_install(FILES lib/expat.h lib/expat_external.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#
# pkg-config file
#
set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix "\${prefix}")
set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
configure_file(expat.pc.in ${CMAKE_CURRENT_BINARY_DIR}/expat.pc @ONLY)
expat_install(FILES ${CMAKE_CURRENT_BINARY_DIR}/expat.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

#
# C command line tool xmlwf
#
if(EXPAT_BUILD_TOOLS)
    set(xmlwf_SRCS
        xmlwf/xmlwf.c
        xmlwf/xmlfile.c
        xmlwf/codepage.c
        xmlwf/readfilemap.c
    )

    add_executable(xmlwf ${xmlwf_SRCS})
    set_property(TARGET xmlwf PROPERTY RUNTIME_OUTPUT_DIRECTORY xmlwf)
    target_link_libraries(xmlwf expat)
    expat_install(TARGETS xmlwf DESTINATION ${CMAKE_INSTALL_BINDIR})
    if(BUILD_doc)
        file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/doc)
        add_custom_command(TARGET expat PRE_BUILD COMMAND "${DOCBOOK_TO_MAN}" "${PROJECT_SOURCE_DIR}/doc/xmlwf.xml" && mv "XMLWF.1" "${PROJECT_BINARY_DIR}/doc/xmlwf.1")
        expat_install(FILES "${PROJECT_BINARY_DIR}/doc/xmlwf.1" DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
     endif()
endif()

#
# C code examples
#
if(EXPAT_BUILD_EXAMPLES)
    add_executable(elements examples/elements.c)
    set_property(TARGET elements PROPERTY RUNTIME_OUTPUT_DIRECTORY examples)
    target_link_libraries(elements expat)

    add_executable(outline examples/outline.c)
    set_property(TARGET outline PROPERTY RUNTIME_OUTPUT_DIRECTORY examples)
    target_link_libraries(outline expat)
endif(EXPAT_BUILD_EXAMPLES)

#
# C/C++ test runners
#
if(BUILD_tests)
    ## these are unittests that can be run on any platform
    enable_language(CXX)
    enable_testing()

    set(test_SRCS
        tests/chardata.c
        tests/memcheck.c
        tests/minicheck.c
        tests/structdata.c
    )

    add_executable(runtests tests/runtests.c ${test_SRCS})
    set_property(TARGET runtests PROPERTY RUNTIME_OUTPUT_DIRECTORY tests)
    target_link_libraries(runtests expat)
    add_test(runtests tests/runtests)

    add_executable(runtestspp tests/runtestspp.cpp ${test_SRCS})
    set_property(TARGET runtestspp PROPERTY RUNTIME_OUTPUT_DIRECTORY tests)
    target_link_libraries(runtestspp expat)
    add_test(runtestspp tests/runtestspp)
endif(BUILD_tests)

if(BUILD_fuzzers)
    if(NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
        message(SEND_ERROR
            "Building fuzz targets without Clang (but ${CMAKE_C_COMPILER_ID}) "
            "is not supported. Please set "
            "-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++.")
    endif()

    string(FIND "${CMAKE_EXE_LINKER_FLAGS}" "-fsanitize" sanitizer_present)
    if(${sanitizer_present} EQUAL "-1")
        message(WARNING
            "There was no sanitizer present when building the fuzz targets. "
            "This is likely in error - consider adding "
            "-DCMAKE_C_FLAGS='-fsanitize=<sanitizer>' and "
            "-DCMAKE_CXX_FLAGS='-fsanitize=<sanitizer>' and "
            "-DCMAKE_EXE_LINKER_FLAGS='-fsanitize=<sanitizer>' to your cmake "
            "execution.")
    endif()

    set(encoding_types UTF-16 UTF-8 ISO-8859-1 US-ASCII UTF-16BE UTF-16LE)
    set(fuzz_targets xml_parse_fuzzer xml_parsebuffer_fuzzer)

    foreach(fuzz_target ${fuzz_targets})
        foreach(encoding_type ${encoding_types})
            set(target_name ${fuzz_target}_${encoding_type})
            add_executable(${target_name} fuzz/${fuzz_target}.c ${expat_SRCS})
            target_compile_definitions(${target_name}
                PRIVATE ENCODING_FOR_FUZZING=${encoding_type})
            target_compile_options(${target_name} PRIVATE -fsanitize=fuzzer-no-link)
            # NOTE: Avoiding target_link_options here only because it needs CMake >=3.13
            set_target_properties(${target_name} PROPERTIES LINK_FLAGS -fsanitize=fuzzer)
            set_property(
                TARGET ${target_name} PROPERTY RUNTIME_OUTPUT_DIRECTORY fuzz)
        endforeach()
    endforeach()
endif(BUILD_fuzzers)

#
# Documentation
#
configure_file(Changes changelog COPYONLY)
expat_install(
    FILES
        AUTHORS
        ${CMAKE_CURRENT_BINARY_DIR}/changelog
    DESTINATION
        ${CMAKE_INSTALL_DOCDIR})

#
# CMake files for find_package(expat [..] CONFIG [..])
#
configure_package_config_file(
        cmake/expat-config.cmake.in
        cmake/expat-config.cmake
    INSTALL_DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/expat-${PROJECT_VERSION}/
)
write_basic_package_version_file(
    cmake/expat-config-version.cmake
    COMPATIBILITY SameMajorVersion  # i.e. semver
)
export(
    TARGETS
        expat
    FILE
        cmake/expat-targets.cmake  # not going to be installed
)
expat_install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/expat-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/expat-config-version.cmake
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/expat-${PROJECT_VERSION}/
)
expat_install(
    EXPORT
        expat
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/expat-${PROJECT_VERSION}/
    NAMESPACE
        expat::
)

#
# Summary
#
if(XML_UNICODE)
    if(XML_UNICODE_WCHAR_T)
        set(_EXPAT_CHAR_TYPE "wchar_t")
    else()
        set(_EXPAT_CHAR_TYPE "unsigned short")
    endif()
else()
    set(_EXPAT_CHAR_TYPE "char")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" _EXPAT_BUILD_TYPE_UPPER)

message(STATUS "===========================================================================")
message(STATUS "")
message(STATUS "Configuration")
message(STATUS "  Prefix ..................... ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Build type ................. ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared libraries ........... ${BUILD_shared}")
if(WIN32)
    message(STATUS "  Static CRT ................. ${MSVC_USE_STATIC_CRT}")
endif()
message(STATUS "  Character type ............. ${_EXPAT_CHAR_TYPE}")
if(WIN32)
    message(STATUS "  Binary postfix ............. ${CMAKE_${_EXPAT_BUILD_TYPE_UPPER}_POSTFIX}")
endif()
message(STATUS "")
message(STATUS "  Build documentation ........ ${BUILD_doc}")
message(STATUS "  Build examples ............. ${EXPAT_BUILD_EXAMPLES}")
message(STATUS "  Build fuzzers .............. ${BUILD_fuzzers}")
message(STATUS "  Build tests ................ ${BUILD_tests}")
message(STATUS "  Build tools (xmlwf) ........ ${EXPAT_BUILD_TOOLS}")
message(STATUS "  Install files .............. ${INSTALL}")
message(STATUS "")
message(STATUS "  Features")
message(STATUS "    Attributes info .......... ${XML_ATTR_INFO}")
message(STATUS "    Context bytes ............ ${XML_CONTEXT_BYTES}")
message(STATUS "    DTD support .............. ${XML_DTD}")
message(STATUS "    Large size ............... ${XML_LARGE_SIZE}")
message(STATUS "    Namespace support ........ ${XML_NS}")
message(STATUS "")
message(STATUS "  Entropy sources")
if(WIN32)
    message(STATUS "    rand_s ................... ON")
else()
    message(STATUS "    getrandom ................ ${HAVE_GETRANDOM}")
    message(STATUS "    syscall SYS_getrandom .... ${HAVE_SYSCALL_GETRANDOM}")
    message(STATUS "    libbsd ................... ${USE_libbsd}")
    message(STATUS "    /dev/random .............. ${XML_DEV_URANDOM}")
endif()
message(STATUS "")
if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    message(STATUS "Continue with")
    message(STATUS "  make")
    if(BUILD_tests)
        message(STATUS "  make test")
    endif()
    if(INSTALL)
        message(STATUS "  sudo make install")
    endif()
    message(STATUS "")
endif()
message(STATUS "===========================================================================")
