xcopy /y xmltok\Release\xmltok.lib lib
xcopy /y xmlparse\Release\xmlparse.lib lib
rm -f expat.zip
cd ..
sed -e 's;^^;expat/;' expat\files.txt | zip -qr expat\expat.zip -@ -ll
sed -e 's;^^;expat/;' expat\win32files.txt | zip -qr expat\expat.zip -@
cd expat 
