xcopy xmltok\\Release\\xmltok.lib lib
xcopy xmlparse\\Release\\xmlparse.lib lib
rm -fr expat
mkdir expat
tar cf - `cat files.txt win32files.txt` | (cd expat; tar xf -)
(cd expat; flip -u `cat ../files.txt`)
rm -f expat.zip
zip -qr expat.zip expat
rm -fr expat
