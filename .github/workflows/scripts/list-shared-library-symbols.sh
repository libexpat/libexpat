#! /usr/bin/env bash
# Copyright (C) 2021 Sebastian Pipping <sebastian@pipping.org>
# Licensed under the MIT license
set -e -u -o pipefail
if [[ $# -ne 1 ]]; then
    echo "usage: $(basename "$0") SO_FILE_PATH" >&2
    exit 1
fi
nm -D -p "${1}" | fgrep ' T ' | awk '{print $3}' | sort -f -d
