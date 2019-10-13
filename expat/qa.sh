#! /usr/bin/env bash
# Copyright (C) 2016 Sebastian Pipping <sebastian@pipping.org>
# Licensed under MIT license

set -e
set -o nounset


ANNOUNCE() {
    local open='\e[1m'
    local close='\e[0m'

    echo -e -n "${open}" >&2
    echo -n "# $*" >&2
    echo -e "${close}" >&2
}


WARNING() {
    local open='\e[1;33m'
    local close='\e[0m'

    echo -e -n "${open}" >&2
    echo -n "WARNING: $*" >&2
    echo -e "${close}" >&2
}


RUN() {
    ANNOUNCE "$@"
    env "$@"
}


populate_environment() {
    : ${MAKE:=make}

    case "${QA_COMPILER}" in
        clang)
            : ${CC:=clang}
            : ${CXX:=clang++}
            : ${LD:=clang++}
            ;;
        gcc)
            : ${CC:=gcc}
            : ${CXX:=g++}
            : ${LD:=ld}
            ;;
    esac

    : ${BASE_COMPILE_FLAGS:="-pipe -Wall -Wextra -pedantic -Wno-overlength-strings -Wno-long-long"}
    : ${BASE_LINK_FLAGS:=}

    if [[ ${QA_COMPILER} = clang ]]; then
        case "${QA_SANITIZER}" in
            address)
                # http://clang.llvm.org/docs/AddressSanitizer.html
                BASE_COMPILE_FLAGS+=" -g -fsanitize=address -fno-omit-frame-pointer -fno-common"
                BASE_LINK_FLAGS+=" -g -fsanitize=address"
                ;;
            memory)
                # http://clang.llvm.org/docs/MemorySanitizer.html
                BASE_COMPILE_FLAGS+=" -fsanitize=memory -fno-omit-frame-pointer -g -O2 -fsanitize-memory-track-origins -fsanitize-blacklist=$PWD/memory-sanitizer-blacklist.txt"
                ;;
            undefined)
                # http://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
                BASE_COMPILE_FLAGS+=" -fsanitize=undefined"
                BASE_LINK_FLAGS+=" -fsanitize=undefined"
                export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1:abort_on_error=1"
                ;;
        esac
    fi


    if [[ ${QA_COMPILER} = gcc ]]; then
        case "${QA_PROCESSOR}" in
            egypt) BASE_COMPILE_FLAGS+=" -fdump-rtl-expand" ;;
            gcov) BASE_COMPILE_FLAGS+=" --coverage -O0" ;;
        esac
    fi


    CFLAGS="-std=c99 ${BASE_COMPILE_FLAGS} ${CFLAGS:-}"
    CXXFLAGS="-std=c++98 ${BASE_COMPILE_FLAGS} ${CXXFLAGS:-}"
    LDFLAGS="${BASE_LINK_FLAGS} ${LDFLAGS:-}"
}


run_cmake() {
    local cmake_args=(
        -DCMAKE_C_COMPILER="${CC}"
        -DCMAKE_C_FLAGS="${CFLAGS}"

        -DCMAKE_CXX_COMPILER="${CXX}"
        -DCMAKE_CXX_FLAGS="${CXXFLAGS}"

        -DCMAKE_LINKER="${LD}"
        -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}"
        -DCMAKE_SHARED_LINKER_FLAGS="${LDFLAGS}"

        -DEXPAT_WARNINGS_AS_ERRORS=ON
    )
    RUN cmake "${cmake_args[@]}" "$@" .
}


run_compile() {
    local make_args=(
        VERBOSE=1
        -j2
    )

    RUN "${MAKE}" "${make_args[@]}" clean all
}


run_tests() {
    case "${QA_PROCESSOR}" in
        egypt) return 0 ;;
    esac

    if [[ ${CC} =~ mingw ]]; then
        # NOTE: Filenames are hardcoded for Travis Ubuntu trusty, as of now
        for i in tests xmlwf ; do
            RUN ln -s \
                    /usr/i686-w64-mingw32/lib/libwinpthread-1.dll \
                    /usr/lib/gcc/i686-w64-mingw32/*/libgcc_s_sjlj-1.dll \
                    /usr/lib/gcc/i686-w64-mingw32/*/libstdc++-6.dll \
                    "$PWD"/libexpat{,w}.dll \
                    ${i}/
        done
    fi

    local make_args=(
        CTEST_OUTPUT_ON_FAILURE=1
        CTEST_PARALLEL_LEVEL=2
        VERBOSE=1
        test
    )
    [[ $* =~ -DEXPAT_DTD=OFF ]] || make_args+=( run-xmltest )

    RUN "${MAKE}" "${make_args[@]}"
}


run_processor() {
    if [[ ${QA_COMPILER} != gcc ]]; then
        return 0
    fi

    case "${QA_PROCESSOR}" in
    egypt)
        local DOT_FORMAT="${DOT_FORMAT:-svg}"
        local o="callgraph.${DOT_FORMAT}"
        ANNOUNCE "egypt ...... | dot ...... > ${o}"
        find -name '*.expand' \
                | sort \
                | xargs -r egypt \
                | unflatten -c 20 \
                | dot -T${DOT_FORMAT} -Grankdir=LR \
                > "${o}"
        ;;
    gcov)
        for gcov_dir in lib xmlwf ; do
        (
            cd "${gcov_dir}" || exit 1
            for gcda_file in $(find . -name '*.gcda' | sort) ; do
                RUN gcov -s .libs/ ${gcda_file}
            done
        )
        done

        RUN find -name '*.gcov' | sort
        ;;
    esac
}


run() {
    populate_environment
    dump_config

    run_cmake "$@"
    run_compile
    run_tests "$@"
    run_processor
}


dump_config() {
    cat <<EOF
Configuration:
  QA_COMPILER=${QA_COMPILER}  # auto-detected from \$CC and \$CXX
  QA_PROCESSOR=${QA_PROCESSOR}  # GCC only
  QA_SANITIZER=${QA_SANITIZER}  # Clang only

  CFLAGS=${CFLAGS}
  CXXFLAGS=${CXXFLAGS}
  LDFLAGS=${LDFLAGS}

  CC=${CC}
  CXX=${CXX}
  LD=${LD}
  MAKE=${MAKE}

Compiler (\$CC):
EOF
    "${CC}" --version | sed 's,^,  ,'
    echo
}


classify_compiler() {
    local i
    for i in "${CC:-}" "${CXX:-}"; do
        [[ "$i" =~ clang ]] && { echo clang ; return ; }
    done
    echo gcc
}


process_config() {
    case "${QA_COMPILER:=$(classify_compiler)}" in
        clang|gcc) ;;
        *) usage; exit 1 ;;
    esac


    if [[ ${QA_COMPILER} != gcc && -n ${QA_PROCESSOR:-} ]]; then
        WARNING "QA_COMPILER=${QA_COMPILER} is not 'gcc' -- ignoring QA_PROCESSOR=${QA_PROCESSOR}"
    fi

    case "${QA_PROCESSOR:=gcov}" in
        egypt|gcov) ;;
        *) usage; exit 1 ;;
    esac


    if [[ ${QA_COMPILER} != clang && -n ${QA_SANITIZER:-} ]]; then
        WARNING "QA_COMPILER=${QA_COMPILER} is not 'clang' -- ignoring QA_SANITIZER=${QA_SANITIZER}" >&2
    fi

    case "${QA_SANITIZER:=address}" in
        address|memory|undefined) ;;
        *) usage; exit 1 ;;
    esac
}


usage() {
    cat <<"EOF"
Usage:
  $ ./qa.sh [ARG ..]

Environment variables
  QA_COMPILER=(clang|gcc)                  # default: auto-detected
  QA_PROCESSOR=(egypt|gcov)                # default: gcov
  QA_SANITIZER=(address|memory|undefined)  # default: address

EOF
}


main() {
    if [[ ${1:-} = --help ]]; then
        usage; exit 0
    fi

    process_config

    run "$@"
}


main "$@"
