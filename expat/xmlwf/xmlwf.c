#define STRICT 1
#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include "wfcheck.h"

static
int doFile(const char *name)
{
  HANDLE f;
  HANDLE m;
  DWORD size;
  const char *p;
  const char *bad = 0;
  int ret;
  enum WfCheckResult result;

  f = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			  FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (f == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "%s: CreateFile failed\n", name);
    return 0;
  }
  size = GetFileSize(f, NULL);
  m = CreateFileMapping(f, NULL, PAGE_READONLY, 0, 0, NULL);
  if (m == NULL) {
    fprintf(stderr, "%s: CreateFileMapping failed\n", name);
    CloseHandle(f);
    return 0;
  }
  p = (const char *)MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
  if (p == NULL) {
    CloseHandle(m);
    CloseHandle(f);
    fprintf(stderr, "%s: MapViewOfFile failed\n", name);
    return 0;
  }
  result = wfCheck(p, size, &bad);
  if (result) {
    static const char *message[] = {
      0,
      "out of memory",
      "no element found",
      "invalid token after %lu bytes",
      "unclosed token started after %lu bytes",
      "unclosed token started after %lu bytes",
      "mismatched tag after %lu bytes",
      "duplicate attribute after %lu bytes",
      "junk after document element after %lu bytes",
    };
    fprintf(stderr, "%s: ", name);
    fprintf(stderr, message[result], (unsigned long)(bad - p));
    putc('\n', stderr);
    ret = 1;
  }
  else
    ret = 0;
  UnmapViewOfFile(p);
  CloseHandle(m);
  CloseHandle(f);
  return ret;
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
    int n = doFile(argv[i]);
    if (n > ret)
      ret = n;
  }
  return ret;
}
