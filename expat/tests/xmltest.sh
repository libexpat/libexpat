#! /bin/sh

VERBOSE=''

if [ "$1" = '-v' -o "$1" = '--verbose' ] ; then
    VERBOSE="$1"
    shift
fi

if [ ! "$1" = '' ] ; then
    ERRORS=0
    if [ "$VERBOSE" ] ; then
        OUTPUT="/tmp/$$.out"
    else
        OUTPUT="/dev/null"
    fi
    while [ "$1" ] ; do
        FILE="`basename \"$1\"`"
        DIR="`dirname \"$1\"`"
        DIR="`dirname \"$DIR\"`"
        ../xmlwf/xmlwf -d /tmp "$DIR/$FILE"
        diff -u "$DIR/out/$FILE" "/tmp/$FILE" >$OUTPUT
        ERR=$?
        rm "/tmp/$FILE"
        if [ ! "$ERR" = 0 ] ; then
            ERRORS=`expr $ERRORS + 1`
            echo "$DIR/$FILE ... Error"
            cat $OUTPUT
        elif [ "$VERBOSE" ] ; then
            echo "$DIR/$FILE ... Ok"
        fi
        shift
    done
    if [ "$VERBOSE" ] ; then
        rm $OUTPUT
    fi
    if [ ! "$ERRORS" = '0' ] ; then
        echo "    Errors:  $ERRORS"
        exit 1
    fi
else
    SCRIPTDIR="`dirname \"$0\"`"
    cd "$SCRIPTDIR"
    find xmltest -name \*.xml | grep /out/ | xargs ./xmltest.sh $VERBOSE
    if [ ! "$?" = "0" ] ; then
        exit 1
    fi
fi
