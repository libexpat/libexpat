#ifndef FALLTHROUGH_H
#  define FALLTHROUGH_H 1

// Explicit fallthrough in switch case to avoid warnings
// with compiler flag -Wimplicit-fallthrough.

#  define _EXPAT_FALLTHROUGH                                                   \
    do {                                                                       \
    } while (0)

#  if defined(__has_attribute)
#    if __has_attribute(fallthrough)
#      undef _EXPAT_FALLTHROUGH
#      define _EXPAT_FALLTHROUGH __attribute__((fallthrough))
#    endif
#  endif

#endif // FALLTHROUGH_H
