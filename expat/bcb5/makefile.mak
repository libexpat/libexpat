all: setup expat expatw elements outline xmlwf

setup:
 setup

expat:
 make -f libexpat.mak

expatw:
 make -f libexpatw.mak

elements:
 make -f elements.mak

outline:
 make -f outline.mak

xmlwf:
 make -f xmlwf.mak

clean:
 deltree /y debug\obj

distclean:
 deltree /y debug\*.*
