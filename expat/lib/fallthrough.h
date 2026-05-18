#ifndef __FALLTHROUGH_H
#  define __FALLTHROUGH_H 1

// Explicit fallthrough in switch case to avoid warnings
// with compiler flag -Wimplicit-fallthrough.
// [[ ]] attributes are a C23 feature
#  if (__STDC_VERSION__ >= 202311L) && defined(__has_c_attribute)
#    if __has_c_attribute(fallthrough)
#      define _EXPAT_FALLTHROUGH [[fallthrough]]
#    endif
#  endif
#  if ! defined(_EXPAT_FALLTHROUGH) && defined(__has_attribute)
#    if __has_attribute(fallthrough)
#      define _EXPAT_FALLTHROUGH __attribute__((fallthrough))
#    endif
#  endif
#  if ! defined(_EXPAT_FALLTHROUGH)
#    define _EXPAT_FALLTHROUGH                                                 \
      do {                                                                     \
      } while (0)
#  endif

#endif // __FALLTHROUGH_H
