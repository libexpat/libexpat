#! /bin/bash
#
# make-release.sh: make an Expat release
#
# USAGE: make-release.sh tagname
#
# Note: tagname may be HEAD to just grab the head revision (e.g. for testing)
#

if test $# != 1; then
  echo "USAGE: $0 tagname"
  exit 1
fi

tmpdir=expat-release.$$
if test -e $tmpdir; then
  echo "ERROR: oops. chose the $tmpdir subdir, but it exists."
  exit 1
fi

echo "Checking out into temporary area: $tmpdir"
cvs -d :pserver:anonymous@cvs.expat.sourceforge.net:/cvsroot/expat export -r "$1" -d $tmpdir expat || exit 1

echo ""
echo "----------------------------------------------------------------------"
echo "Preparing $tmpdir for release (running buildconf.sh)"
(cd $tmpdir && ./buildconf.sh) || exit 1

# figure out the release version
hdr="$tmpdir/lib/expat.h"
MAJOR_VERSION="`sed -n -e '/MAJOR_VERSION/s/[^0-9]*//gp' $hdr`"
MINOR_VERSION="`sed -n -e '/MINOR_VERSION/s/[^0-9]*//gp' $hdr`"
MICRO_VERSION="`sed -n -e '/MICRO_VERSION/s/[^0-9]*//gp' $hdr`"
vsn=$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION

echo ""
echo "Release version: $vsn"

distdir=expat-$vsn
if test -e $distdir; then
  echo "ERROR: for safety, you must manually remove $distdir."
  rm -rf $tmpdir
  exit 1
fi
mkdir $distdir || exit 1

echo ""
echo "----------------------------------------------------------------------"
echo "Building $distdir based on the MANIFEST:"
files="`sed -e 's/[ 	]:.*$//' $tmpdir/MANIFEST`"
for file in $files; do
  echo "Copying $file..."
  (cd $tmpdir && cp -Pp $file ../$distdir) || exit 1
done

echo ""
echo "----------------------------------------------------------------------"
echo "Removing (temporary) checkout directory..."
rm -rf $tmpdir

tarball=$distdir.tar.gz
echo "Constructing $tarball..."
tar cf - $distdir | gzip -9 > $tarball

echo "Done."
