all: setup expat expatw elements outline xmlwf

setup:
 setup

expat:
 make -l -flibexpat.mak

expatw:
 make -l -flibexpatw.mak

elements:
 make -l -felements.mak

outline:
 make -l -foutline.mak

xmlwf:
 make -l -fxmlwf.mak

clean:
# works on Win98/ME
# deltree /y debug\obj
# works on WinNT/2000
 del /s/f/q debug\obj

distclean:
# works on Win98/ME
# deltree /y debug\*.*
# works on WinNT/2000
 del /s/f/q debug\*
