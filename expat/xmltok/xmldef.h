/* This file can be used for any definitions needed in
particular environments. */

#ifdef MOZILLA

#include "nspr.h"
#define malloc(x) PR_Calloc(1,(x))
#define free(x) PR_Free(x)
#define int int32

#endif /* MOZILLA */
