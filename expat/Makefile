CC=gcc
CFLAGS=-O2 -I.
# Use one of the next two lines; unixfilemap is better if it works.
FILEMAP_OBJ=xmlwf/unixfilemap.o
#FILEMAP_OBJ=xmlwf/readfilemap.o
XMLWF_OBJS=xmltok.o \
  xmlrole.o \
  xmlwf/xmlwf.o \
  xmlwf/wfcheck.o \
  xmlwf/wfcheckmessage.o \
  xmlwf/hashtable.o \
  $(FILEMAP_OBJ)
XMLEC_OBJS=xmltok.o xmlec/xmlec.o
EXE=

all: xmlwf xmlec

xmlwf: $(XMLWF_OBJS)
	$(CC) $(CFLAGS) -o $@ $(XMLWF_OBJS)

xmlec: $(XMLEC_OBJS)
	$(CC) $(CFLAGS) -o $@ $(XMLEC_OBJS)

clean:
	rm -f $(XMLWF_OBJS) xmlwf$(EXE) $(XMLEC_OBJS) xmlec$(EXE)

nametab.h: gennmtab/gennmtab$(EXE)
	rm -f $@
	gennmtab/gennmtab$(EXE) >$@

gennmtab/gennmtab$(EXE): gennmtab/gennmtab.c
	$(CC) $(CFLAGS) -o $@ gennmtab/gennmtab.c

xmltok.o: nametab.h
