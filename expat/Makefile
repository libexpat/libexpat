CC=gcc
CFLAGS=-O2 -I. -Ixmlparse
# Use one of the next two lines; unixfilemap is better if it works.
FILEMAP_OBJ=xmlwf/unixfilemap.o
#FILEMAP_OBJ=xmlwf/readfilemap.o
OBJS=xmltok/xmltok.o \
  xmltok/xmlrole.o \
  xmlwf/xmlwf.o \
  xmlparse/xmlparse.o \
  xmlparse/hashtable.o \
  $(FILEMAP_OBJ)
EXE=

all: xmlwf/xmlwf$(EXE)

xmlwf/xmlwf$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
	rm -f $(OBJS) xmlwf/xmlwf$(EXE)

xmltok/nametab.h: gennmtab/gennmtab$(EXE)
	rm -f $@
	gennmtab/gennmtab$(EXE) >$@

gennmtab/gennmtab$(EXE): gennmtab/gennmtab.c
	$(CC) $(CFLAGS) -o $@ gennmtab/gennmtab.c

xmltok/xmltok.o: xmltok/nametab.h

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
