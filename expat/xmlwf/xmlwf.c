/*
The contents of this file are subject to the Mozilla Public License
Version 1.0 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations
under the License.

The Original Code is expat.

The Initial Developer of the Original Code is James Clark.
Portions created by James Clark are Copyright (C) 1998
James Clark. All Rights Reserved.

Contributor(s):
*/

#include "xmlparse.h"
#include "filemap.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>

#ifdef _MSC_VER
#include <io.h>
#endif

#ifdef _POSIX_SOURCE
#include <unistd.h>
#endif

#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY _O_BINARY
#else
#define O_BINARY 0
#endif
#endif

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

#ifdef _DEBUG
#define READ_SIZE 16
#else
#define READ_SIZE (1024*8)
#endif

static void characterData(void *userData, const char *s, int len)
{
  FILE *fp = userData;
  for (; len > 0; --len, ++s) {
    switch (*s) {
    case '&':
      fputs("&amp;", fp);
      break;
    case '<':
      fputs("&lt;", fp);
      break;
    case '>':
      fputs("&gt;", fp);
      break;
    case '"':
      fputs("&quot;", fp);
      break;
    case 9:
    case 10:
    case 13:
      fprintf(fp, "&#%d;", *s);
      break;
    default:
      putc(*s, fp);
      break;
    }
  }
}

/* Lexicographically comparing UTF-8 encoded attribute values,
is equivalent to lexicographically comparing based on the character number. */

static int attcmp(const void *att1, const void *att2)
{
  return strcmp(*(const char **)att1, *(const char **)att2);
}

static void startElement(void *userData, const char *name, const char **atts)
{
  int nAtts;
  const char **p;
  FILE *fp = userData;
  putc('<', fp);
  fputs(name, fp);

  p = atts;
  while (*p)
    ++p;
  nAtts = (p - atts) >> 1;
  if (nAtts > 1)
    qsort((void *)atts, nAtts, sizeof(char *) * 2, attcmp);
  while (*atts) {
    putc(' ', fp);
    fputs(*atts++, fp);
    putc('=', fp);
    putc('"', fp);
    characterData(userData, *atts, strlen(*atts));
    putc('"', fp);
    atts++;
  }
  putc('>', fp);
}

static void endElement(void *userData, const char *name)
{
  FILE *fp = userData;
  putc('<', fp);
  putc('/', fp);
  fputs(name, fp);
  putc('>', fp);
}

static void processingInstruction(void *userData, const char *target, const char *data)
{
  FILE *fp = userData;
  fprintf(fp, "<?%s %s?>", target, data);
}

typedef struct {
  XML_Parser parser;
  int *retPtr;
} PROCESS_ARGS;

static
void reportError(XML_Parser parser, const char *filename)
{
  int code = XML_GetErrorCode(parser);
  const char *message = XML_ErrorString(code);
  if (message)
    fprintf(stdout, "%s:%d:%ld: %s\n",
	    filename,
	    XML_GetErrorLineNumber(parser),
	    XML_GetErrorColumnNumber(parser),
	  message);
  else
    fprintf(stderr, "%s: (unknown message %d)\n", filename, code);
}

static
void processFile(const void *data, size_t size, const char *filename, void *args)
{
  XML_Parser parser = ((PROCESS_ARGS *)args)->parser;
  int *retPtr = ((PROCESS_ARGS *)args)->retPtr;
  if (!XML_Parse(parser, data, size, 1)) {
    reportError(parser, filename);
    *retPtr = 0;
  }
  else
    *retPtr = 1;
}

static
int processStream(const char *filename, XML_Parser parser)
{
  int fd = open(filename, O_BINARY|O_RDONLY);
  if (fd < 0) {
    perror(filename);
    return 0;
  }
  for (;;) {
    int nread;
    char *buf = XML_GetBuffer(parser, READ_SIZE);
    if (!buf) {
      close(fd);
      fprintf(stderr, "%s: out of memory\n", filename);
      return 0;
    }
    nread = read(fd, buf, READ_SIZE);
    if (nread < 0) {
      perror(filename);
      close(fd);
      return 0;
    }
    if (!XML_ParseBuffer(parser, nread, nread == 0)) {
      reportError(parser, filename);
      close(fd);
      return 0;
    }
    if (nread == 0) {
      close(fd);
      break;;
    }
  }
  return 1;
}

static
void usage(const char *prog)
{
  fprintf(stderr, "usage: %s [-r] [-d output-dir] [-e encoding] file ...\n", prog);
  exit(1);
}

int main(int argc, char **argv)
{
  int i;
  const char *outputDir = 0;
  const char *encoding = 0;
  int useFilemap = 1;

#ifdef _MSC_VER
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
#endif

  i = 1;
  while (i < argc && argv[i][0] == '-') {
    int j;
    if (argv[i][1] == '-' && argv[i][2] == '\0') {
      i++;
      break;
    }
    j = 1;
    if (argv[i][j] == 'r') {
      useFilemap = 0;
      j++;
    }
    if (argv[i][j] == 'd') {
      if (argv[i][j + 1] == '\0') {
	if (++i == argc)
	  usage(argv[0]);
	outputDir = argv[i];
      }
      else
	outputDir = argv[i] + j + 1;
      i++;
    }
    else if (argv[i][j] == 'e') {
      if (argv[i][j + 1] == '\0') {
	if (++i == argc)
	  usage(argv[0]);
	encoding = argv[i];
      }
      else
	encoding = argv[i] + j + 1;
      i++;
    }
    else if (argv[i][j] == '\0' && j > 1)
      i++;
    else
      usage(argv[0]);
  }
  if (i == argc)
    usage(argv[0]);
  for (; i < argc; i++) {
    FILE *fp = 0;
    char *outName = 0;
    int result;
    XML_Parser parser = XML_ParserCreate(encoding);
    if (outputDir) {
      const char *file = argv[i];
      if (strrchr(file, '/'))
	file = strrchr(file, '/') + 1;
#ifdef WIN32
      if (strrchr(file, '\\'))
	file = strrchr(file, '\\') + 1;
#endif
      outName = malloc(strlen(outputDir) + strlen(file) + 2);
      strcpy(outName, outputDir);
      strcat(outName, "/");
      strcat(outName, file);
      fp = fopen(outName, "wb");
      if (!fp) {
	perror(outName);
	exit(1);
      }
      XML_SetUserData(parser, fp);
      XML_SetElementHandler(parser, startElement, endElement);
      XML_SetCharacterDataHandler(parser, characterData);
      XML_SetProcessingInstructionHandler(parser, processingInstruction);
    }
    if (useFilemap) {
      PROCESS_ARGS args;
      args.retPtr = &result;
      args.parser = parser;
      if (!filemap(argv[i], processFile, &args))
	result = 0;
    }
    else
      result = processStream(argv[i], parser);
    if (outputDir) {
      fclose(fp);
      if (!result)
	remove(outName);
      free(outName);
    }
    XML_ParserFree(parser);
  }
  return 0;
}
