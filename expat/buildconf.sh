#! /bin/sh

#
# Create the libtool helper files
#
echo "Copying libtool helper files ..."

#
# find libtoolize, or glibtoolize on MacOS X
#
libtoolize=`conftools/PrintPath glibtoolize libtoolize`
if [ "x$libtoolize" = "x" ]; then
    echo "libtoolize not found in path"
    exit 1
fi

# Remove any libtool files so one can switch between libtool 1.3
# and libtool 1.4 by simply rerunning the buildconf script.
(cd conftools/; rm -f ltmain.sh ltconfig)

#
# Note: we don't use --force (any more) since we have a special
# config.guess/config.sub that we want to ensure is used.
#
# --copy to avoid symlinks; we want originals for the distro
# --automake to make it shut up about "things to do"
#
$libtoolize --copy --automake

ltpath=`dirname $libtoolize`
ltfile=`cd $ltpath/../share/aclocal ; pwd`/libtool.m4
cp $ltfile conftools/libtool.m4

### for a little while... remove stray aclocal.m4 files from
### developers' working copies. we no longer use it. (nothing else
### will remove it, and leaving it creates big problems)
rm -f aclocal.m4

#
# Generate the autoconf header template (expat_config.h.in) and ./configure
#
echo "Creating expat_config.h.in ..."
${AUTOHEADER:-autoheader}

echo "Creating configure ..."
### do some work to toss config.cache?
${AUTOCONF:-autoconf}

# toss this; it gets created by autoconf on some systems
rm -rf autom4te*.cache

# exit with the right value, so any calling script can continue
exit 0
