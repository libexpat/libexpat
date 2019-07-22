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

    export QA_FUZZIT=""
    if [[ ${QA_COMPILER} = clang ]]; then
        export UBSAN_OPTIONS=""
        case "${QA_SANITIZER}" in
            address)
                # http://clang.llvm.org/docs/AddressSanitizer.html
                BASE_COMPILE_FLAGS+=" -g -fsanitize=address -fno-omit-frame-pointer"
                BASE_LINK_FLAGS+=" -g -Wc,-fsanitize=address"  # "-Wc," is for libtool
                export QA_FUZZIT="asan"
                ;;
            memory)
                # http://clang.llvm.org/docs/MemorySanitizer.html
                BASE_COMPILE_FLAGS+=" -fsanitize=memory -fno-omit-frame-pointer -g -O2 -fsanitize-memory-track-origins -fsanitize-blacklist=$PWD/memory-sanitizer-blacklist.txt"
                export QA_FUZZIT="msan"
                ;;
            undefined)
                # http://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
                BASE_COMPILE_FLAGS+=" -fsanitize=undefined"
                BASE_LINK_FLAGS+=" -fsanitize=undefined"
                export UBSAN_OPTIONS=print_stacktrace=1
                export QA_FUZZIT="ubsan"
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
    CXXFLAGS="${BASE_COMPILE_FLAGS} ${CXXFLAGS:-}"
    LDFLAGS="${BASE_LINK_FLAGS} ${LDFLAGS:-}"
}


run_configure() {
    RUN CC="${CC}" CFLAGS="${CFLAGS}" \
            CXX="${CXX}" CXXFLAGS="${CXXFLAGS}" \
            LD="${LD}" LDFLAGS="${LDFLAGS}" \
            ./configure "--enable-stdcxx=c++98" "$@" \
        || { RUN cat config.log ; false ; }
}


run_compile() {
    RUN CFLAGS="${CFLAGS} -Werror" \
            CXXFLAGS="${CXXFLAGS} -Werror" \
            "${MAKE}" clean all
}


run_fuzzit() {
    FUZZIT_API_KEY=e59b073a8ddfe3686de10624accf111b90c67da08d8d86768da623724be054f820dda54b1e140519c5608c92bcc8d327
    if [ ${TRAVIS_EVENT_TYPE} -eq 'cron' ]; then
        FUZZING_TYPE=fuzzing
    else
        FUZZING_TYPE=sanity
    fi
    if [ "$TRAVIS_PULL_REQUEST" = "false" ]; then
        FUZZIT_BRANCH="${TRAVIS_BRANCH}"
    else
        FUZZIT_BRANCH="PR-${TRAVIS_PULL_REQUEST}"
    fi
    FUZZIT_ARGS="--type ${FUZZING_TYPE} --branch ${FUZZIT_BRANCH} --revision ${TRAVIS_COMMIT}"
    if [ -n "$UBSAN_OPTIONS" ]; then
        FUZZIT_ARGS+=" --ubsan_options ${UBSAN_OPTIONS}"
    fi
    wget -O fuzzit https://github.com/fuzzitdev/fuzzit/releases/download/v1.2.5/fuzzit_1.2.5_Linux_x86_64
    chmod +x fuzzit
    ./fuzzit auth ${FUZZIT_API_KEY}
    set -x
    grep "$QA_FUZZIT" tests/fuzz/fuzzitid.txt | cut -d" " -f2-3 | while read i; do
        # id is the second and last word after space
        targetid=${i##* }
        # binary is the first word before space
        targetbin=${i%% *}
        ./fuzzit c job ${FUZZIT_ARGS} ${targetid} ./tests/fuzz/${targetbin}
    done
    set +x
}

run_tests() {
    case "${QA_PROCESSOR}" in
        egypt) return 0 ;;
    esac

    if [[ ${CC} =~ mingw ]]; then
        # NOTE: Filenames are hardcoded for Travis Ubuntu trusty, as of now
        for i in tests xmlwf xmlwf/.libs ; do
            RUN ln -s \
                    /usr/i686-w64-mingw32/lib/libwinpthread-1.dll \
                    /usr/lib/gcc/i686-w64-mingw32/4.8/libgcc_s_sjlj-1.dll \
                    /usr/lib/gcc/i686-w64-mingw32/4.8/libstdc++-6.dll \
                    ../lib/.libs/libexpat-1.dll \
                    ${i}/
        done
    fi

    RUN "${MAKE}" \
            CFLAGS="${CFLAGS} -Werror" \
            CXXFLAGS="${CXXFLAGS} -Werror" \
            check run-xmltest \
        || {
            RUN cat tests/runtests.log
            RUN cat tests/runtestspp.log
            false
        }

    if [[ -n "$QA_FUZZIT" && `ls tests/fuzz/*fuzzer | wc -l` -gt 1 ]]; then
        run_fuzzit
    fi

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

    run_configure "$@"
    run_compile
    run_tests
    run_processor
}


dump_config() {
    cat <<EOF
Configuration:
  QA_COMPILER=${QA_COMPILER}
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


process_config() {
    case "${QA_COMPILER:=gcc}" in
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
  QA_COMPILER=(clang|gcc)                  # default: gcc
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
