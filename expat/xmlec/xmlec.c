#include <stdio.h>

#ifdef _MSC_VER
#define XMLTOKAPI __declspec(dllimport)
#endif
#include "xmltok.h"

#ifdef FILEMAP

#define STRICT 1
#include <windows.h>

static
int XmlSkipProlog(const char **s, const char *end, const char **nextTokP);

int XmlParse(const char *s, size_t n, const char *filename)
{
  unsigned nElements = 0;
  const char *start = s;
  const char *end = s + n;
  const char *next;
  int tok = XmlSkipProlog(&s, end, &next);
  for (;;) {
    switch (tok) {
    case XML_TOK_NONE:
      if (nElements == 0) {
	fprintf(stderr, "%s: no instance\n", filename);
	return 0;
      }
      printf("%8u %s\n", nElements, filename);
      return 1;
    case XML_TOK_INVALID:
      fprintf(stderr, "%s: well-formedness error at byte %lu\n",
	      filename, (unsigned long)(s - start));
      return 0;
    case XML_TOK_PARTIAL:
      fprintf(stderr, "%s: unclosed token started at byte %lu\n",
	      filename, (unsigned long)(s - start));
      return 0;
    case XML_TOK_COMMENT:
      break;
    case XML_TOK_START_TAG:
      nElements++;
      break;
    default:
      break;
    }
    s = next;
    tok = XmlContentTok(s, end, &next);
  }
  /* not reached */
}

static
int XmlSkipProlog(const char **startp, const char *end, const char **nextTokP)
{
  const char *s = *startp;
  for (;;) {
    int tok = XmlPrologTok(s, end, nextTokP);
    switch (tok) {
    case XML_TOK_NONE:
    case XML_TOK_INVALID:
    case XML_TOK_PARTIAL:
    case XML_TOK_START_TAG:
      *startp = s;
      return tok;
    default:
      break;
    }
    s = *nextTokP;
  }
  /* not reached */
}

static
int doFile(const char *name)
{
  HANDLE f;
  HANDLE m;
  DWORD size;
  const char *p;
  int ret;

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
  ret = XmlParse(p, size, name);
  UnmapViewOfFile(p);
  CloseHandle(m);
  CloseHandle(f);
  return ret;
}

#else /* not FILEMAP */

#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

struct XmlTokBuffer {
  char *buf;
  char *end;
  char *ptr;
  size_t size;
  int fd;
  int doneProlog;
  int eof;
  unsigned long endOffset;
};

#define XmlTokBufferOffset(tb) ((tb)->endOffset - ((tb)->end - (tb)->ptr))
#define READSIZE 1024

void outOfMemory()
{
  fprintf(stderr, "out of memory\n");
  exit(1);
}

void XmlTokBufferInit(struct XmlTokBuffer *tb, int fd)
{
  tb->buf = malloc(READSIZE);
  if (!tb->buf)
    outOfMemory();
  tb->end = tb->buf;
  tb->ptr = tb->buf;
  tb->size = READSIZE;
  tb->fd = fd;
  tb->doneProlog = 0;
  tb->eof = 0;
  tb->endOffset = 0;
}

void XmlTokBufferFree(struct XmlTokBuffer *tb)
{
  free(tb->buf);
}

int XmlGetToken(struct XmlTokBuffer *tb, const char **tokStart, size_t *tokLength)
{
  int tok;
  for (;;) {
    int nBytes;
    const char *start = tb->ptr;
    if (!tb->doneProlog) {
      tok = XmlPrologTok(start, tb->end, &tb->ptr);
      if (tok == XML_TOK_START_TAG)
	tb->doneProlog = 1;
    }
    else
      tok = XmlContentTok(start, tb->end, &tb->ptr);
    if (tok >= 0) {
      *tokStart = start;
      *tokLength = tb->ptr - start;
      break;
    }
    if (tb->eof)
      break;
    /* Read in multiples of READSIZE, unless read() has previously
       less than it could. */
    if (tb->end == tb->buf + tb->size) {
      size_t keep = tb->end - tb->ptr;
      if (keep == 0)
	tb->ptr = tb->end = tb->buf;
      else if (keep + READSIZE <= tb->size) {
	unsigned nBlocks = (tb->size - keep)/READSIZE;
	char *newEnd = tb->buf + tb->size - (nBlocks * READSIZE);
        memmove(newEnd - keep, tb->ptr, keep);
        tb->end = newEnd;
        tb->ptr = newEnd - keep;
      }
      else {
	char *newBuf, *newEnd;
	tb->size += READSIZE;
	newBuf = malloc(tb->size);
	if (!newBuf)
	  outOfMemory();
	newEnd = newBuf + tb->size - READSIZE;
	memcpy(newEnd - keep, tb->ptr, keep);
	free(tb->buf);
	tb->buf = newBuf;
	tb->end = newEnd;
	tb->ptr = newEnd - keep;
      }
    }
    nBytes = read(tb->fd, tb->end, (tb->buf + tb->size) - tb->end);
    if (nBytes == 0) {
      tb->eof = 1;
      break;
    }
    if (nBytes < 0)
      return XML_TOK_NONE;
    tb->end += nBytes;
    tb->endOffset += nBytes;
  }
  return tok;
}

int doFile(const char *filename)
{
  unsigned nElements = 0;
  struct XmlTokBuffer buf;
  int fd = open(filename, O_RDONLY|O_BINARY);

  if (fd < 0) {
    fprintf(stderr, "%s: cannot open\n", filename);
    return 0;
  }
  XmlTokBufferInit(&buf, fd);
  for (;;) {
    const char *tokStr;
    size_t tokLen;
    int tok = XmlGetToken(&buf, &tokStr, &tokLen);
    switch (tok) {
    case XML_TOK_NONE:
      if (nElements == 0) {
	fprintf(stderr, "%s: no instance\n", filename);
	return 0;
      }
      printf("%8u %s\n", nElements, filename);
      close(fd);
      XmlTokBufferFree(&buf);
      return 1;
    case XML_TOK_INVALID:
      fprintf(stderr, "%s: well-formedness error at byte %lu\n",
	      filename, XmlTokBufferOffset(&buf));
      
      close(fd);
      XmlTokBufferFree(&buf);
      return 0;
    case XML_TOK_PARTIAL:
      fprintf(stderr, "%s: unclosed token started at byte %lu\n",
	      filename, XmlTokBufferOffset(&buf));
      close(fd);
      XmlTokBufferFree(&buf);
      return 0;
    case XML_TOK_COMMENT:
      break;
    case XML_TOK_START_TAG:
      nElements++;
      break;
    default:
      break;
    }
  }

}

#endif /* not FILEMAP */

int main(int argc, char **argv)
{
  int i;
  int ret = 0;
  if (argc == 1) {
    fprintf(stderr, "usage: %s filename ...\n", argv[0]);
    return 1;
  }
  for (i = 1; i < argc; i++)
    if (!doFile(argv[i]))
      ret = 1;
  return ret;
}
