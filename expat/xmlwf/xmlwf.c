#include <stdio.h>
#include <string.h>
#include "wfcheck.h"
#include "filemap.h"
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

struct ProcessFileArg {
  enum EntityType entityType;
  int result;
};

static
void processFile(const void *data, size_t size, const char *filename, void *p)
{
  const char *badPtr = 0;
  unsigned long badLine = 0;
  unsigned long badCol = 0;
  struct ProcessFileArg *arg = p;
  enum WfCheckResult result;

  result = wfCheck(arg->entityType, data, size, &badPtr, &badLine, &badCol);
  if (result) {
    const char *msg = wfCheckMessage(result);
    fprintf(stdout, "%s:", filename);
    if (badPtr != 0)
      fprintf(stdout, "%lu:%lu:", badLine+1, badCol);
    fprintf(stdout, "E: %s", msg ? msg : "(unknown message)");
    putc('\n', stdout);
    arg->result = 1;
  }
  else
    arg->result = 0;
}

int main(int argc, char **argv)
{
  int i = 1;
  int ret = 0;
  struct ProcessFileArg arg;

#ifdef _MSC_VER
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
#endif
  arg.entityType = documentEntity;

  if (i < argc && strcmp(argv[i], "-g") == 0) {
    i++;
    arg.entityType = generalTextEntity;
  }
  if (i < argc && strcmp(argv[i], "--") == 0)
    i++;
  if (i == argc) {
    fprintf(stderr, "usage: %s [-g] filename ...\n", argv[0]);
    return 1;
  }
  for (; i < argc; i++) {
    if (!filemap(argv[i], processFile, &arg))
      ret = 2;
    else if (arg.result && !ret)
      ret = 1;
  }
  return ret;
}
