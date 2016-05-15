#! /bin/bash
# Copyright (C) 2016 Sebastian Pipping <sebastian@pipping.org>
# Licensed under MIT license

set -o nounset

: ${GCC_CC:=gcc}
: ${GCC_CXX:=g++}
: ${CLANG_CC:=clang}
: ${CLANG_CXX:=clang++}

: ${CC:="${CLANG_CC}"}
: ${CXX:="${CLANG_CXX}"}
: ${MAKE:=make}

: ${BASE_FLAGS:="-pipe -Wall -Wextra -pedantic -Wno-overlength-strings"}

main() {
    local mode="${1:-}"
    local RUNENV
    local BASE_FLAGS="${BASE_FLAGS}"

    case "${mode}" in
    address)
        # http://clang.llvm.org/docs/AddressSanitizer.html
        local CC="${GCC_CC}"
        local CXX="${GCC_CXX}"
        BASE_FLAGS+=" -g -fsanitize=address -fno-omit-frame-pointer"
        ;;
    coverage)
        local CC="${GCC_CC}"
        local CXX="${GCC_CXX}"
        BASE_FLAGS+=" --coverage --no-inline"
        ;;
    memory)
        # http://clang.llvm.org/docs/MemorySanitizer.html
        BASE_FLAGS+=" -fsanitize=memory -fno-omit-frame-pointer -g -O2"
        ;;
    undefined)
        # http://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
        BASE_FLAGS+=" -fsanitize=undefined"
        export UBSAN_OPTIONS=print_stacktrace=1
        ;;
    *)
        echo "Usage:" 1>&2
        echo "  ${0##*/} (address|coverage|memory|undefined)" 1>&2
        exit 1
        ;;
    esac

    CFLAGS="-std=c89 ${BASE_FLAGS}"
    CXXFLAGS="-std=c++98 ${BASE_FLAGS}"

    (
        PS4='# '
        set -e
        set -x

        CC="${CC}" CFLAGS="${CFLAGS}" \
            CXX="${CXX}" CXXFLAGS="${CXXFLAGS}" \
            ./configure

        "${MAKE}" clean all
        "${MAKE}" check run-xmltest
    ) || exit 1

    if [[ "${mode}" = coverage ]]; then
        find -name '*.gcda' | sort | xargs gcov
    fi
}

main "$@"
