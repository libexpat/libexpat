#! /usr/bin/env bash
#
# EXPAT TEST SCRIPT FOR W3C XML TEST SUITE
#
# This script can be used to exercise Expat against the w3c.org xml test
# suite, available from:
#
#   <http://www.w3.org/XML/Test/xmlts20020606.zip>
#
# The script lists  all test cases where Expat shows  a discrepancy from
# the  expected result.   Test  cases where  only  the canonical  output
# differs  are prefixed  with  "Output  differs:", and  a  diff file  is
# generated in the appropriate subdirectory under $OUTDIR.
#
# If there  are output files provided,  the script will use  output from
# xmlwf and compare the desired output  against it.  However, one has to
# take into account that the canonical output produced by xmlwf conforms
# to an older definition of canonical XML and does not generate notation
# declarations.


#### global settings

shopt -s nullglob


#### global variables

# The environment  variable EXPAT_ABS_SRCDIR is  set in the  Makefile to
# the source directory.  The  environment variable EXPAT_ABS_BUILDDIR is
# set in the Makefile to the build directory.
#
declare -r EXPAT_ABS_SRCDIR="${EXPAT_ABS_SRCDIR:?'environment variable EXPAT_ABS_SRCDIR is not set'}"
declare -r EXPAT_ABS_BUILDDIR="${EXPAT_ABS_BUILDDIR:?'environment variable EXPAT_ABS_BUILDDIR is not set'}"

declare -r XMLWF="${1:-${EXPAT_ABS_BUILDDIR}/xmlwf/xmlwf}"

# Unicode-aware diff utility.  Requires Python version 3.
#
declare -r DIFF="${EXPAT_ABS_SRCDIR}/tests/udiffer.py"

declare -r XML_BASE_SRCDIR="$EXPAT_ABS_BUILDDIR"/tests/xmlconf
declare -r XML_BASE_DSTDIR="$EXPAT_ABS_BUILDDIR"/tests/xmltest-output

# Number of successful tests.
#
declare -i SUCCESS=0

# Number of failed tests.
#
declare -i ERROR=0


#### main function

function main () {
    test_well_formed_tests_cases
    test_not_well_formed_tests_cases

    printf 'Passed: %d\nFailed: %d\n' $SUCCESS $ERROR
}


#### well-formed test cases

function test_well_formed_tests_cases () {
    local XML_RELATIVE_SRCDIR XML_FILE_NAME

    cd "$XML_BASE_SRCDIR"
    for XML_RELATIVE_SRCDIR in \
        ibm/valid/P* \
            ibm/invalid/P* \
            xmltest/valid/ext-sa \
            xmltest/valid/not-sa \
            xmltest/invalid \
            xmltest/invalid/not-sa \
            xmltest/valid/sa \
            sun/valid \
            sun/invalid
    do
        make_directory "${XML_BASE_DSTDIR}/${XML_RELATIVE_SRCDIR}"
        cd "${XML_BASE_SRCDIR}/${XML_RELATIVE_SRCDIR}"
        for XML_FILE_NAME in $(ls -1 *.xml | sort -d)
        do
            [[ -f "$XML_FILE_NAME" ]] || continue
            run_xmlwf_well_formed_doc_test "$XML_RELATIVE_SRCDIR" "$XML_FILE_NAME"
            update_test_result_status $?
        done
    done

    XML_RELATIVE_SRCDIR=oasis
    make_directory "${XML_BASE_DSTDIR}/${XML_RELATIVE_SRCDIR}"

    cd "${XML_BASE_SRCDIR}/${XML_RELATIVE_SRCDIR}"
    for XML_FILE_NAME in *pass*.xml ; do
        run_xmlwf_well_formed_doc_test "$XML_RELATIVE_SRCDIR" "$XML_FILE_NAME"
        update_test_result_status $?
    done
}


#### not well-formed test cases

function test_not_well_formed_tests_cases () {
    cd "$XML_BASE_SRCDIR"
    for XML_RELATIVE_SRCDIR in \
        ibm/not-wf/P* \
            ibm/not-wf/p28a \
            ibm/not-wf/misc \
            xmltest/not-wf/ext-sa \
            xmltest/not-wf/not-sa \
            xmltest/not-wf/sa \
            sun/not-wf
    do
        cd "${XML_BASE_SRCDIR}/${XML_RELATIVE_SRCDIR}"
        for XML_FILE_NAME in *.xml
        do
            run_xmlwf_not_well_formed_doc_test "$XML_RELATIVE_SRCDIR" "$XML_FILE_NAME"
            update_test_result_status $?
        done
    done

    XML_RELATIVE_SRCDIR=oasis
    make_directory "${XML_BASE_DSTDIR}/${XML_RELATIVE_SRCDIR}"

    cd "${XML_BASE_SRCDIR}/${XML_RELATIVE_SRCDIR}"
    for XML_FILE_NAME in *fail*.xml
    do
        run_xmlwf_not_well_formed_doc_test "$XML_RELATIVE_SRCDIR" "$XML_FILE_NAME"
        update_test_result_status $?
    done
}


#### not well-formed document processing

# Upon entering this function, the current working directory must be:
#
#   "${XML_BASE_SRCDIR}/${XML_RELATIVE_SRCDIR}"
#
function run_xmlwf_not_well_formed_doc_test () {
    local -r XML_RELATIVE_SRCDIR="${1:?missing XML relative source directory pathname argument in call to ${FUNCNAME}}"
    local -r XML_FILE_NAME="${2:?missing XML file name argument in call to ${FUNCNAME}}"

    local -r XMLWF_NOT_WELL_FORMED_LOG_DIR="${XML_BASE_DSTDIR}/${XML_RELATIVE_SRCDIR}"
    local -r XMLWF_NOT_WELL_FORMED_LOG_FILE="${XMLWF_NOT_WELL_FORMED_LOG_DIR}/${XML_FILE_NAME}.log"
    local NOT_WELL_FORMED_ERROR_DESCRIPTION

    if ! test -f "$XML_FILE_NAME"
    then
        print_error_message 'missing source XML file: %s/%s' "$XML_RELATIVE_SRCDIR" "$XML_FILE_NAME"
        return 1
    fi

    make_directory "$XMLWF_NOT_WELL_FORMED_LOG_DIR" || return $?

    # In case the document is  well formed, this command: prints nothing
    # to stdout.
    #
    # In case the  document is not well formed, this  command: prints to
    # stdout a description of the "not well formed" error.
    #
    $XMLWF -p "$XML_FILE_NAME" > "$XMLWF_NOT_WELL_FORMED_LOG_FILE" || return $?

    read NOT_WELL_FORMED_ERROR_DESCRIPTION < "$XMLWF_NOT_WELL_FORMED_LOG_FILE"
    if test -z "$NOT_WELL_FORMED_ERROR_DESCRIPTION"
    then
        print_log_message 'Expected not well-formed: %s/%s' "$XML_RELATIVE_SRCDIR" "$XML_FILE_NAME"
        return 1
    else
        return 0
    fi
}


#### well-formed document processing

# Upon entering this function, the current working directory must be:
#
#   "${XML_BASE_SRCDIR}/${XML_RELATIVE_SRCDIR}"
#
function run_xmlwf_well_formed_doc_test () {
    local -r XML_RELATIVE_SRCDIR="${1:?missing XML relative source directory pathname argument in call to ${FUNCNAME}}"
    local -r XML_FILE_NAME="${2:?missing XML file name argument in call to ${FUNCNAME}}"

    local -r XMLWF_NOT_WELL_FORMED_LOG_DIR="${XML_BASE_DSTDIR}/${XML_RELATIVE_SRCDIR}"
    local -r XMLWF_NOT_WELL_FORMED_LOG_FILE="${XMLWF_NOT_WELL_FORMED_LOG_DIR}/${XML_FILE_NAME}.log"
    local NOT_WELL_FORMED_ERROR_DESCRIPTION

    local -r XML_PRECOMPUTED_TRANSFORMED_PATHNAME="${XML_BASE_SRCDIR}/${XML_RELATIVE_SRCDIR}/out/${XML_FILE_NAME}"
    local -r XMLWF_TRANSFORMED_OUTPUT_DIR="${XML_BASE_DSTDIR}/${XML_RELATIVE_SRCDIR}"
    local -r XMLWF_TRANSFORMED_OUTPUT_FILE="${XMLWF_TRANSFORMED_OUTPUT_DIR}/${XML_FILE_NAME}"
    local -r XMLWF_TRANSFORMED_DIFF_FILE="${XMLWF_TRANSFORMED_OUTPUT_DIR}/${XML_FILE_NAME}.diff"

    if ! test -f "$XML_FILE_NAME"
    then
        print_error_message 'missing source XML file: %s/%s' "$XML_RELATIVE_SRCDIR" "$XML_FILE_NAME"
        return 1
    fi

    make_directory "$XMLWF_TRANSFORMED_OUTPUT_DIR"  || return $?
    make_directory "$XMLWF_NOT_WELL_FORMED_LOG_DIR" || return $?

    # In case the document is  well formed, this command: prints nothing
    # to  stdout;  outputs a  transformed  version  of the  document  to
    # $XMLWF_TRANSFORMED_OUTPUT_DIR.
    #
    # In case the  document is not well formed, this  command: prints to
    # stdout a description of the "not well formed" error.
    #
    $XMLWF -p -N -d "$XMLWF_TRANSFORMED_OUTPUT_DIR" "$XML_FILE_NAME" > "$XMLWF_NOT_WELL_FORMED_LOG_FILE" || return $?

    read NOT_WELL_FORMED_ERROR_DESCRIPTION < "$XMLWF_NOT_WELL_FORMED_LOG_FILE"
    if test -z "$NOT_WELL_FORMED_ERROR_DESCRIPTION"
    then
        # Not  all  the  source  files have  a  precomputed  transformed
        # version.
        if test -f "$XML_PRECOMPUTED_TRANSFORMED_PATHNAME"
        then
            "$DIFF" "$XML_PRECOMPUTED_TRANSFORMED_PATHNAME" "$XMLWF_TRANSFORMED_OUTPUT_FILE" > "$XMLWF_TRANSFORMED_DIFF_FILE"

            if test -s "$XMLWF_TRANSFORMED_DIFF_FILE"
            then
                # The diff file exists and it is not empty.
                print_log_message 'Output differs: %s/%s' "$XML_RELATIVE_SRCDIR" "$XML_FILE_NAME"
                return 1
            fi
        fi
        return 0
    else
        print_log_message 'in %s: %s' "$XML_RELATIVE_SRCDIR" "$NOT_WELL_FORMED_ERROR_DESCRIPTION"
        return 1
    fi
}


#### utility functions

function print_error_message () {
    local -r TEMPLATE="${1:?missing template argument in call to ${FUNCNAME}}"
    shift
    printf "xmltest.sh: ${TEMPLATE}\n" "$@" >&2
}

function print_log_message () {
    local -r TEMPLATE="${1:?missing template argument in call to ${FUNCNAME}}"
    shift
    printf "${TEMPLATE}\n" "$@"
}

function make_directory () {
    local -r PATHNAME="${1:?missing directory pathname argument in call to ${FUNCNAME}}"

    if ! test -d "$PATHNAME"
    then /bin/mkdir -p "$PATHNAME" || return $?
    fi
    return 0
}

function update_test_result_status () {
    local -r EXIT_STATUS="${1:?missing exit status argument in call to ${FUNCNAME}}"

    if test "$EXIT_STATUS" -eq 0
    then let ++SUCCESS
    else let ++ERROR
    fi
}


#### done

main

### end of file
