
#include <stddef.h>

enum WfCheckResult {
  wellFormed,
  noMemory,
  syntaxError,
  noElements,
  invalidToken,
  unclosedToken,
  partialChar,
  tagMismatch,
  duplicateAttribute,
  junkAfterDocElement
};

enum WfCheckResult wfCheck(const char *s, size_t n,
			   const char **errorPtr,
			   unsigned long *errorLineNumber,
			   unsigned long *errorColNumber);

