#! /bin/bash
# Copyright (C) 2017 Sebastian Pipping <sebastian@pipping.org>
# Licensed under the MIT license
set -e

PS4='# '
set -x


ret=0


# Install missing build time dependencies
sudo apt-get --quiet update
sudo apt-get --quiet install docbook2x


# Run test suite
cd expat

for mode in \
        address \
        lib-coverage \
        ; do
    git clean -X -f
    ./buildconf.sh
    CFLAGS='-g -pipe' ./qa.sh ${mode} || ret=1
done


exit ${ret}
