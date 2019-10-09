#! /usr/bin/env bash
#                          __  __            _
#                       ___\ \/ /_ __   __ _| |_
#                      / _ \\  /| '_ \ / _` | __|
#                     |  __//  \| |_) | (_| | |_
#                      \___/_/\_\ .__/ \__,_|\__|
#                               |_| XML parser
#
# Copyright (c) 2019 Expat development team
# Licensed under the MIT license:
#
# Permission is  hereby granted,  free of charge,  to any  person obtaining
# a  copy  of  this  software   and  associated  documentation  files  (the
# "Software"),  to  deal in  the  Software  without restriction,  including
# without  limitation the  rights  to use,  copy,  modify, merge,  publish,
# distribute, sublicense, and/or sell copies of the Software, and to permit
# persons  to whom  the Software  is  furnished to  do so,  subject to  the
# following conditions:
#
# The above copyright  notice and this permission notice  shall be included
# in all copies or substantial portions of the Software.
#
# THE  SOFTWARE  IS  PROVIDED  "AS  IS",  WITHOUT  WARRANTY  OF  ANY  KIND,
# EXPRESS  OR IMPLIED,  INCLUDING  BUT  NOT LIMITED  TO  THE WARRANTIES  OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
# NO EVENT SHALL THE AUTHORS OR  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR  OTHER LIABILITY, WHETHER  IN AN  ACTION OF CONTRACT,  TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
# USE OR OTHER DEALINGS IN THE SOFTWARE.

set -e
set -u
PS4='# '

create_git_archive() (
    local tarname="$1"
    local format="$2"
    local git_archive_args=(
        --format="${format}"
        --prefix="${tarname}"/
        --output="${tarname}.${format}"
        "${@:3}"
    )
    set -x
    git archive "${git_archive_args[@]}" HEAD
)

add_generated_file() {
    local tarname="$1"
    local format="$2"
    local generated_file="$3"
    local origin_file="$4"

    if [[ ! -e "${generated_file}" ]]; then
        echo "FATAL: Generated file ${generated_file} does not exist, needs a build." >&2
        exit 1
    fi

    if [[ ! -e "${origin_file}" ]]; then
        echo "FATAL: Source file ${origin_file} does not exist, sanity check failed." >&2
        exit 1
    fi

    if [[ "$(stat -c %Y "${origin_file}")" -gt "$(stat -c %Y "${generated_file}")" ]]; then
        echo "FATAL: Source file \"${origin_file}\" is more recent than \"${generated_file}\", needs a build." >&2
        exit 1
    fi

    local in_archive_generated_file_dir="${tarname}/$(dirname "${generated_file}")"
    local in_archive_generated_file="${in_archive_generated_file_dir}/$(basename "${generated_file}")"
    mkdir -p "${in_archive_generated_file_dir}"
    cp "${generated_file}" "${in_archive_generated_file_dir}"/

    case "${format}" in
    tar)
        ( set -x; tar rf "${tarname}.${format}" "${in_archive_generated_file}" )
        ;;
    zip)
        ( set -x; zip -q9r "${tarname}.${format}" "${in_archive_generated_file}" )
        ;;
    esac

    rm "${in_archive_generated_file}"
    rmdir -p "${in_archive_generated_file_dir}"
}

compress_tarball() (
    local tarname="$1"
    local command="$2"
    local format="$3"
    echo "${PS4}${command} -c ${*:4} < ${tarname}.tar > ${tarname}.tar.${format}" >&2
    "${command}" -c "${@:4}" < "${tarname}".tar > "${tarname}.tar.${format}"
)

check_tarball() (
    local tarname="$1"
    set -x
    tar xf "${tarname}".tar
    (
        cd "${tarname}"

        mkdir build
        cd build

        cmake "${@:2}" ..
        make
        make test
        make DESTDIR="${PWD}"/ROOT/ install
    )
    rm -Rf "${tarname}"
)

report() {
    local tarname="$1"
    echo =================================================
    echo "${tarname} archives ready for distribution:"
    ls -1 "${tarname}".*
    echo =================================================
}

cleanup() (
    local tarname="$1"
    set -x
    rm "${tarname}".tar
)

main() {
    local PACKAGE=expat
    local PACKAGE_VERSION="$(./conftools/get-version.sh lib/expat.h)"
    local PACKAGE_TARNAME="${PACKAGE}-${PACKAGE_VERSION}"

    create_git_archive ${PACKAGE_TARNAME} tar
    add_generated_file ${PACKAGE_TARNAME} tar doc/xmlwf.1 doc/xmlwf.xml
    check_tarball ${PACKAGE_TARNAME} "$@"

    create_git_archive ${PACKAGE_TARNAME} zip -9
    add_generated_file ${PACKAGE_TARNAME} zip doc/xmlwf.1 doc/xmlwf.xml

    compress_tarball ${PACKAGE_TARNAME} bzip2 bz2 -9
    compress_tarball ${PACKAGE_TARNAME} gzip  gz  --best
    compress_tarball ${PACKAGE_TARNAME} lzip  lz  -9
    compress_tarball ${PACKAGE_TARNAME} xz    xz  -e

    cleanup ${PACKAGE_TARNAME}

    report ${PACKAGE_TARNAME}
}

main "$@"
