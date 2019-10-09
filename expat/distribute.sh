#! /usr/bin/env bash
# Copyright (C) 2018 Sebastian Pipping <sebastian@pipping.org>
# Licensed under the MIT license
#
# Creates release tarball and detached GPG signature file for upload

set -e

PS4='# '
set -x

version="$(./conftools/get-version.sh lib/expat.h)"

./make-distcheck.sh

extensions=(
    gz
    bz2
    lz
    xz
    zip
)

for ext in ${extensions[@]} ; do
    archive=expat-${version}.tar.${ext}
    gpg --armor --output ${archive}.asc --detach-sign ${archive}
    gpg --verify ${archive}.asc ${archive}
done
