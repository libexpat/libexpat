#define STRICT 1
#include <windows.h>
#include <stdio.h>
#include "filemap.h"

int filemap(const char *name,
	    void (*processor)(const void *, size_t, const char *, void *arg),
	    void *arg)
{
  HANDLE f;
  HANDLE m;
  DWORD size;
  DWORD sizeHi;
  void *p;

  f = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			  FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (f == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "%s: CreateFile failed\n", name);
    return 0;
  }
  size = GetFileSize(f, &sizeHi);
  if (sizeHi) {
    fprintf(stderr, "%s: too big (limit 2Gb)\n", name);
    return 0;
  }
  /* CreateFileMapping barfs on zero length files */
  if (size == 0) {
    fprintf(stderr, "%s: zero-length file\n", name);
    return 0;
  }
  m = CreateFileMapping(f, NULL, PAGE_READONLY, 0, 0, NULL);
  if (m == NULL) {
    fprintf(stderr, "%s: CreateFileMapping failed\n", name);
    CloseHandle(f);
    return 0;
  }
  p = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
  if (p == NULL) {
    CloseHandle(m);
    CloseHandle(f);
    fprintf(stderr, "%s: MapViewOfFile failed\n", name);
    return 0;
  }
  processor(p, size, name, arg); 
  UnmapViewOfFile(p);
  CloseHandle(m);
  CloseHandle(f);
  return 1;
}
