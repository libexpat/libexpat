CC=gcc
# If you know what your system's byte order is, define BYTE_ORDER:
# use -DBYTE_ORDER=12 for little-endian byte order;
# use -DBYTE_ORDER=21 for big-endian (network) byte order.
CFLAGS=-O2 -Ixmltok -Ixmlparse
# Use one of the next two lines; unixfilemap is better if it works.
FILEMAP_OBJ=xmlwf/unixfilemap.o
#FILEMAP_OBJ=xmlwf/readfilemap.o
OBJS=xmltok/xmltok.o \
  xmltok/xmlrole.o \
  xmlwf/xmlwf.o \
  xmlwf/codepage.o \
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
