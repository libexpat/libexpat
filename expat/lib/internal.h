/* internal.h

   Internal definitions used by Expat.  This is not needed to compile
   client code.

   The following definitions are made:

   FASTCALL -- Used for most internal functions to specify that the
               fastest possible calling convention be used.

   inline   -- Used for selected internal functions for which inlining
               may improve performance on some platforms.
*/

#if defined(__GNUC__)
#define FASTCALL __attribute__((stdcall, regparm(3)))
#elif defined(WIN32)
#define FASTCALL __fastcall
#else
#define FASTCALL
#endif

#ifndef XML_MIN_SIZE
#if !defined(__cplusplus) && !defined(inline)
#ifdef __GNUC__
#define inline __inline
#endif /* __GNUC__ */
#endif
#endif /* XML_MIN_SIZE */

#ifdef __cplusplus
#define inline inline
#else
#ifndef inline
#define inline
#endif
#endif
