rm -fr expat
mkdir expat
tar cf - `cat files.txt` | (cd expat; tar xf -)
files=`grep -v expat.mak files.txt`
(cd expat; flip -u $files)
mkdir expat/bin
mkdir expat/lib
xcopy bin\\xmltok.dll expat\\bin
xcopy bin\\xmlparse.dll expat\\bin
xcopy xmltok\\Release\\xmltok.lib expat\\lib
xcopy xmlparse\\Release\\xmlparse.lib expat\\lib
xcopy bin\\xmlwf.exe expat\\bin
rm -f expat.zip
zip -qr expat.zip expat
rm -fr expat
