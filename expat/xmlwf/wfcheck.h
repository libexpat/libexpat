
#include <stddef.h>

enum WfCheckResult {
  wellFormed,
  noMemory,
  noElements,
  invalidToken,
  unclosedToken,
  partialChar,
  tagMismatch,
  duplicateAttribute,
  junkAfterDocElement
};

enum WfCheckResult wfCheck(const char *s, size_t n, const char **badPtr);

