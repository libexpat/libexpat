#ifndef __EXPAT_FALLTHROUGH
#define __EXPAT_FALLTHROUGH 1

// Shamelessly taken from Python's pyport.h
#ifdef __has_attribute
#  define __expat_has_attribute(x) __has_attribute(x)
#else
#  define __expat_has_attribute(x) 0
#endif

// Explicit fallthrough in switch case to avoid warnings
// with compiler flag -Wimplicit-fallthrough.
//
// __attribute__((fallthrough)) was introduced in GCC 7 and Clang 10 /
// Apple Clang 12.0. Earlier Clang versions support only the C++11
// style fallthrough attribute, not the GCC extension syntax used here,
// and __has_attribute(fallthrough) evaluates to 1.
#if __expat_has_attribute(fallthrough) && (!defined(__clang__) || \
    (!defined(__apple_build_version__) && __clang_major__ >= 10) || \
    (defined(__apple_build_version__) && __clang_major__ >= 12))
#  define _EXPAT_FALLTHROUGH __attribute__((fallthrough))
#else
#  define _EXPAT_FALLTHROUGH do { } while (0)
#endif

#undef __expat_has_attribute

#endif //__EXPAT_FALLTHROUGH
