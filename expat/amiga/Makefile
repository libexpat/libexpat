#
# Makefile for AmigaOS
#

.PHONY: help all check clean install uninstall

tooldir = SDK:C
includedir = SDK:Local/clib2/include
libdir = SDK:Local/clib2/lib

LIBRARY = libexpat.a

help:
	@echo "Expat XML Parser"
	@echo ""
	@echo "Requirements:"
	@echo "- AmigaOS 4.0"
	@echo "- SDK 51.5"
	@echo "- clib2 1.188"
	@echo ""
	@echo "Targets:"
	@echo "all - build expat lib, all tools, test suite and run tests"
	@echo "check - run test suite"
	@echo "clean - clean all product files"
	@echo "install - install expat lib and headers in SDK"
	@echo "uninstall - uninstall expat lib and headers"

all: $(LIBRARY) xmlwf elements outline check benchmark

check: runtests
	runtests

clean:
	-delete /lib/#?.o quiet
	-delete /xmlwf/#?.o quiet
	-delete /examples/#?.o quiet
	-delete /tests/#?.o quiet
	-delete $(LIBRARY) xmlwf runtests elements outline benchmark quiet

install: xmlwf $(LIBRARY)
	copy xmlwf $(tooldir) quiet
	copy /lib/expat.h /lib/expat_external.h $(includedir) quiet
	copy $(LIBRARY) $(libdir) quiet

uninstall:
	-delete $(tooldir)/xmlwf quiet
	-delete $(includedir)/expat.h $(includedir)/expat_external.h quiet
	-delete $(libdir)/$(LIBRARY) quiet

CC = gcc
LIBTOOL = ar
STRIP = strip
LIBDIR = /lib

INCLUDES = -I../lib
LDFLAGS =
CFLAGS = -DNDEBUG -O2
LTFLAGS = -crs
STRIPFLAGS = -R.comment

COMPILE = $(CC) $(CFLAGS) $(INCLUDES)

LINK_LIB = $(LIBTOOL) $(LTFLAGS)

LINK_EXE = $(COMPILE) $(LDFLAGS) -o $@

LIB_OBJS = $(LIBDIR)/xmlparse.o $(LIBDIR)/xmltok.o $(LIBDIR)/xmlrole.o
LIB_LINK_OBJS = $(subst $(LIBDIR),..$(LIBDIR),$(LIB_OBJS))
$(LIBRARY): $(LIB_OBJS)
	$(LINK_LIB) $@ $(LIB_LINK_OBJS)

$(LIBDIR)/xmlparse.o: $(LIBDIR)/xmlparse.c $(LIBDIR)/expat.h \
	$(LIBDIR)/xmlrole.h $(LIBDIR)/xmltok.h $(LIBDIR)/expat_external.h \
	$(LIBDIR)/internal.h $(LIBDIR)/amigaconfig.h

$(LIBDIR)/xmlrole.o: $(LIBDIR)/xmlrole.c $(LIBDIR)/ascii.h \
	$(LIBDIR)/xmlrole.h $(LIBDIR)/expat_external.h $(LIBDIR)/internal.h \
	$(LIBDIR)/amigaconfig.h

$(LIBDIR)/xmltok.o: $(LIBDIR)/xmltok.c $(LIBDIR)/xmltok_impl.c \
	$(LIBDIR)/xmltok_ns.c $(LIBDIR)/ascii.h $(LIBDIR)/asciitab.h \
	$(LIBDIR)/iasciitab.h $(LIBDIR)/latin1tab.h $(LIBDIR)/nametab.h \
	$(LIBDIR)/utf8tab.h $(LIBDIR)/xmltok.h $(LIBDIR)/xmltok_impl.h \
	$(LIBDIR)/expat_external.h $(LIBDIR)/internal.h $(LIBDIR)/amigaconfig.h

XMLWF_OBJS = /xmlwf/xmlwf.o /xmlwf/xmlfile.o /xmlwf/codepage.o /xmlwf/readfilemap.o
XMLWF_LINK_OBJS = $(subst /xmlwf/,../xmlwf/,$(XMLWF_OBJS))
/xmlwf/xmlwf.o: /xmlwf/xmlwf.c
/xmlwf/xmlfile.o: /xmlwf/xmlfile.c
/xmlwf/codepage.o: /xmlwf/codepage.c
/xmlwf/readfilemap.o: /xmlwf/readfilemap.c
xmlwf: $(XMLWF_OBJS) $(LIBRARY)
	$(LINK_EXE) $(XMLWF_LINK_OBJS) $(LIBRARY) -lm
	$(STRIP) $(STRIPFLAGS) $@

/examples/elements.o: /examples/elements.c
elements: /examples/elements.o $(LIBRARY)
	$(LINK_EXE) ../examples/elements.o $(LIBRARY) -lm
	$(STRIP) $(STRIPFLAGS) $@

/examples/outline.o: /examples/outline.c
outline: /examples/outline.o $(LIBRARY)
	$(LINK_EXE) ../examples/outline.o $(LIBRARY) -lm
	$(STRIP) $(STRIPFLAGS) $@

/tests/benchmark/benchmark.o: /tests/benchmark/benchmark.c
benchmark: /tests/benchmark/benchmark.o $(LIBRARY)
	$(LINK_EXE) ../tests/benchmark/benchmark.o $(LIBRARY) -lm
	$(STRIP) $(STRIPFLAGS) $@

/tests/chardata.o: /tests/chardata.c /tests/chardata.h
/tests/minicheck.o: /tests/minicheck.c /tests/minicheck.h
/tests/runtests.o: /tests/runtests.c /tests/chardata.h
runtests: /tests/runtests.o /tests/chardata.o /tests/minicheck.o $(LIBRARY)
	$(LINK_EXE) ../tests/runtests.o ../tests/chardata.o ../tests/minicheck.o $(LIBRARY) -lm

%.o: %.c
	$(COMPILE) -o ..$@ -c ..$<
