#include <stdio.h>
#include "wfcheck.h"
#include "filemap.h"

static
void processFile(const void *data, size_t size, const char *filename, void *arg)
{
  const char *badPtr = 0;
  unsigned long badLine = 0;
  unsigned long badCol = 0;
  int *ret = arg;
  enum WfCheckResult result;

  result = wfCheck(data, size, &badPtr, &badLine, &badCol);
  if (result) {
    static const char *message[] = {
      0,
      "out of memory",
      "syntax error",
      "no element found",
      "invalid token",
      "unclosed token",
      "unclosed token",
      "mismatched tag",
      "duplicate attribute",
      "junk after document element",
    };
    fprintf(stderr, "%s:", filename);
    if (badPtr != 0)
      fprintf(stderr, "%lu:%lu:", badLine+1, badCol);
    fprintf(stderr, "E: %s", message[result]);
    putc('\n', stderr);
    if (!*ret)
      *ret = 1;
  }
}


int main(int argc, char **argv)
{
  int i;
  int ret = 0;
  if (argc == 1) {
    fprintf(stderr, "usage: %s filename ...\n", argv[0]);
    return 1;
  }
  for (i = 1; i < argc; i++) {
    if (!filemap(argv[i], processFile, &ret))
      ret = 2;
  }
  return ret;
}
