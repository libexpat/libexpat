all: setup expat expatw elements gennmtab outline xmlwf

setup:
 setup

expat:
 make -f expat.mak

expatw:
 make -f expatw.mak

elements:
 make -f elements.mak

gennmtab:
 make -f gennmtab.mak

outline:
 make -f outline.mak

xmlwf:
 make -f xmlwf.mak

clean:
 deltree /y debug\obj

distclean:
 deltree /y debug\*.*
