/*
Copyright (c) 1998, 1999 Thai Open Source Software Center Ltd
See the file copying.txt for copying permission.
*/

#include <string.h>

#ifdef XML_WINLIB

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#define malloc(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define calloc(x, y) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (x)*(y))
#define free(x) HeapFree(GetProcessHeap(), 0, (x))
#define realloc(x, y) HeapReAlloc(GetProcessHeap(), 0, x, y)
#define abort() /* as nothing */

#else /* not XML_WINLIB */

#include <stdlib.h>

#endif /* not XML_WINLIB */

/* This file can be used for any definitions needed in
particular environments. */

#ifdef MOZILLA

#include "nspr.h"
#define malloc(x) PR_Malloc(x)
#define realloc(x, y) PR_Realloc((x), (y))
#define calloc(x, y) PR_Calloc((x),(y))
#define free(x) PR_Free(x)
#define int int32

#endif /* MOZILLA */
