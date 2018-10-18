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

# Number of successful tests.
#
declare -i SUCCESS=0

# Number of failed tests.
#
declare -i ERROR=0

# Enable/disable verbose execution.
#
declare VERBOSE=false

#### main function

function main () {
    local OPTNAME OPT_ABS_SRCDIR OPT_ABS_BUILDDIR OPT_RUNNER OPT_XMLWF
    local OPT_ABS_XMLCONF OPT_ABS_OUTPUT

    while getopts :vhS:B:R:X:T:O: OPTNAME
    do
        case ${OPTNAME} in
            S)
                OPT_ABS_SRCDIR="${OPTARG}"
                ;;
            B)
                OPT_ABS_BUILDDIR="${OPTARG}"
                ;;
            R)
                OPT_RUNNER="${OPTARG}"
                ;;
            X)
                OPT_XMLWF="${OPTARG}"
                ;;
            T)
                OPT_ABS_XMLCONF="${OPTARG}"
                ;;
            O)
                OPT_ABS_OUTPUT="${OPTARG}"
                ;;
            h)
                print_usage_screen_and_exit
                ;;
            v)
                VERBOSE=true
                ;;
            \?)
                print_error_message 'invalid option "-%s"' "${OPTARG}"
                exit 1
                ;;
        esac
    done

    shift $((OPTIND - 1))
    if test -n "$1"
    then
        print_error_message 'invalid command line argument "%s"' "$1"
        exit 1
    fi

    # EXPAT_ABS_SRCDIR is  set to the  absolute pathname to  the "expat"
    # source  directory.   EXPAT_ABS_BUILDDIR  is set  to  the  absolute
    # pathname to the build directory in which we ran "configure".
    #
    declare -r EXPAT_ABS_SRCDIR="${OPT_ABS_SRCDIR:?'option variable OPT_ABS_SRCDIR is not set'}"
    declare -r EXPAT_ABS_BUILDDIR="${OPT_ABS_BUILDDIR:?'option variable OPT_ABS_BUILDDIR is not set'}"

    declare -r RUNNER="${OPT_RUNNER:-${EXPAT_ABS_BUILDDIR}/run.sh}"
    declare -r XMLWF="${OPT_XMLWF:-${EXPAT_ABS_BUILDDIR}/xmlwf/xmlwf}"

    declare -r ABS_TEST_SUITE_DIR="${OPT_ABS_XMLCONF:-${EXPAT_ABS_BUILDDIR}/tests/xmlconf}"
    declare -r ABS_TESTS_OUTPUT_DIR="${OPT_ABS_OUTPUT:-${EXPAT_ABS_BUILDDIR}/tests/xmltest-output}"

    # Unicode-aware diff utility.  Requires Python version 3.
    #
    declare -r DIFF="${EXPAT_ABS_SRCDIR}"/tests/udiffer.py

    print_verbose_message 'configuration:
  EXPAT_ABS_SRCDIR=%s
  EXPAT_ABS_BUILDDIR=%s
  RUNNER=%s
  XMLWF=%s
  ABS_TEST_SUITE_DIR=%s
  ABS_TESTS_OUTPUT_DIR=%s\n' \
                          "${EXPAT_ABS_SRCDIR}"        \
                          "${EXPAT_ABS_BUILDDIR}"      \
                          "${RUNNER}"                  \
                          "${XMLWF}"                   \
                          "${ABS_TEST_SUITE_DIR}"      \
                          "${ABS_TESTS_OUTPUT_DIR}"

    test_well_formed_tests_cases
    test_not_well_formed_tests_cases

    printf 'Passed: %d\nFailed: %d\n' ${SUCCESS} ${ERROR}
}

function print_usage_screen_and_exit () {
    printf 'Usage: xmltest.sh -S ABS_SRCDIR -B ABS_BUILDDIR [OPTIONS]
Options:
  -S ABS_SRCDIR
       Select the absolute pathname to the "expat" source directory.
  -B ABS_BUILDDIR
       Select the absolute pathname to the build directory in which we
       ran "configure".
  -R RUNNER
       Select the absolute pathname to "run.sh".
       Defaults to: "$ABS_BUILDDIR/run.sh".
  -X XMLWF
       Select the absolute pathname to "xmlwf".
       Defaults to: "$ABS_BUILDDIR/xmlwf/xmlwf".
  -T XMLCONF
       Select the absolute pathname to the directory "xmlconf" from the
       downloaded XML test suite.
       Defaults to: "$ABS_BUILDDIR/tests/xmlconf".
  -O OUTPUT
       Select the absolute pathname to the tests output directory.
       Defaults to: "$ABS_BUILDDIR/tests/xmltest-output".
  -v
       Enable verbose execution.
  -h
       Print the usage screen and exit.\n' >&2
    exit 0
}


#### well-formed test cases

function test_well_formed_tests_cases () {
    local XML_RELATIVE_SRCDIR XML_FILE_NAME

    cd "${ABS_TEST_SUITE_DIR}" || return $?
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
        make_directory "${ABS_TESTS_OUTPUT_DIR}/${XML_RELATIVE_SRCDIR}" || return $?
        cd "${ABS_TEST_SUITE_DIR}/${XML_RELATIVE_SRCDIR}" || return $?
        for XML_FILE_NAME in $(ls -1 *.xml | sort -d)
        do
            [[ -f "${XML_FILE_NAME}" ]] || continue
            run_xmlwf_well_formed_doc_test "${XML_RELATIVE_SRCDIR}" "${XML_FILE_NAME}"
            update_test_result_status $?
        done
    done

    XML_RELATIVE_SRCDIR=oasis
    make_directory "${ABS_TESTS_OUTPUT_DIR}/${XML_RELATIVE_SRCDIR}" || return $?

    cd "${ABS_TEST_SUITE_DIR}/${XML_RELATIVE_SRCDIR}" || return $?
    for XML_FILE_NAME in *pass*.xml ; do
        run_xmlwf_well_formed_doc_test "${XML_RELATIVE_SRCDIR}" "${XML_FILE_NAME}"
        update_test_result_status $?
    done
}


#### not well-formed test cases

function test_not_well_formed_tests_cases () {
    local XML_RELATIVE_SRCDIR XML_FILE_NAME

    cd "${ABS_TEST_SUITE_DIR}" || return $?
    for XML_RELATIVE_SRCDIR in \
        ibm/not-wf/P* \
            ibm/not-wf/p28a \
            ibm/not-wf/misc \
            xmltest/not-wf/ext-sa \
            xmltest/not-wf/not-sa \
            xmltest/not-wf/sa \
            sun/not-wf
    do
        cd "${ABS_TEST_SUITE_DIR}/${XML_RELATIVE_SRCDIR}" || return $?
        for XML_FILE_NAME in *.xml
        do
            run_xmlwf_not_well_formed_doc_test "${XML_RELATIVE_SRCDIR}" "${XML_FILE_NAME}"
            update_test_result_status $?
        done
    done

    XML_RELATIVE_SRCDIR=oasis
    make_directory "${ABS_TESTS_OUTPUT_DIR}/${XML_RELATIVE_SRCDIR}" || return $?

    cd "${ABS_TEST_SUITE_DIR}/${XML_RELATIVE_SRCDIR}" || return $?
    for XML_FILE_NAME in *fail*.xml
    do
        run_xmlwf_not_well_formed_doc_test "${XML_RELATIVE_SRCDIR}" "${XML_FILE_NAME}"
        update_test_result_status $?
    done
}


#### not well-formed document processing

# Upon entering this function, the current working directory must be:
#
#   "${ABS_TEST_SUITE_DIR}/${XML_RELATIVE_SRCDIR}"
#
function run_xmlwf_not_well_formed_doc_test () {
    local -r XML_RELATIVE_SRCDIR="${1:?missing XML relative source directory pathname argument in call to ${FUNCNAME}}"
    local -r XML_FILE_NAME="${2:?missing XML file name argument in call to ${FUNCNAME}}"

    local -r XMLWF_NOT_WELL_FORMED_LOG_DIR="${ABS_TESTS_OUTPUT_DIR}/${XML_RELATIVE_SRCDIR}"
    local -r XMLWF_NOT_WELL_FORMED_LOG_FILE="${XMLWF_NOT_WELL_FORMED_LOG_DIR}/${XML_FILE_NAME}.log"
    local NOT_WELL_FORMED_ERROR_DESCRIPTION

    if ! test -f "${XML_FILE_NAME}"
    then
        print_error_message 'missing source XML file: %s/%s' "${XML_RELATIVE_SRCDIR}" "${XML_FILE_NAME}"
        return 1
    fi

    make_directory "${XMLWF_NOT_WELL_FORMED_LOG_DIR}" || return $?

    # In case the document is  well formed, this command: prints nothing
    # to stdout.
    #
    # In case the  document is not well formed, this  command: prints to
    # stdout a description of the "not well formed" error.
    #
    "${RUNNER}" "${XMLWF}" -p "${XML_FILE_NAME}" > "${XMLWF_NOT_WELL_FORMED_LOG_FILE}" || return $?

    read NOT_WELL_FORMED_ERROR_DESCRIPTION < "${XMLWF_NOT_WELL_FORMED_LOG_FILE}"
    if test -z "${NOT_WELL_FORMED_ERROR_DESCRIPTION}"
    then
        print_log_message 'Expected not well-formed: %s/%s' "${XML_RELATIVE_SRCDIR}" "${XML_FILE_NAME}"
        return 1
    else
        return 0
    fi
}


#### well-formed document processing

# Upon entering this function, the current working directory must be:
#
#   "${ABS_TEST_SUITE_DIR}/${XML_RELATIVE_SRCDIR}"
#
function run_xmlwf_well_formed_doc_test () {
    local -r XML_RELATIVE_SRCDIR="${1:?missing XML relative source directory pathname argument in call to ${FUNCNAME}}"
    local -r XML_FILE_NAME="${2:?missing XML file name argument in call to ${FUNCNAME}}"

    local -r XMLWF_NOT_WELL_FORMED_LOG_DIR="${ABS_TESTS_OUTPUT_DIR}/${XML_RELATIVE_SRCDIR}"
    local -r XMLWF_NOT_WELL_FORMED_LOG_FILE="${XMLWF_NOT_WELL_FORMED_LOG_DIR}/${XML_FILE_NAME}.log"
    local NOT_WELL_FORMED_ERROR_DESCRIPTION

    local -r XML_PRECOMPUTED_TRANSFORMED_PATHNAME="${ABS_TEST_SUITE_DIR}/${XML_RELATIVE_SRCDIR}/out/${XML_FILE_NAME}"
    local -r XMLWF_TRANSFORMED_OUTPUT_DIR="${ABS_TESTS_OUTPUT_DIR}/${XML_RELATIVE_SRCDIR}"
    local -r XMLWF_TRANSFORMED_OUTPUT_FILE="${XMLWF_TRANSFORMED_OUTPUT_DIR}/${XML_FILE_NAME}"
    local -r XMLWF_TRANSFORMED_DIFF_FILE="${XMLWF_TRANSFORMED_OUTPUT_DIR}/${XML_FILE_NAME}.diff"

    if ! test -f "${XML_FILE_NAME}"
    then
        print_error_message 'missing source XML file: %s/%s' "${XML_RELATIVE_SRCDIR}" "${XML_FILE_NAME}"
        return 1
    fi

    make_directory "${XMLWF_TRANSFORMED_OUTPUT_DIR}"  || return $?
    make_directory "${XMLWF_NOT_WELL_FORMED_LOG_DIR}" || return $?

    # In case the document is  well formed, this command: prints nothing
    # to  stdout;  outputs a  transformed  version  of the  document  to
    # ${XMLWF_TRANSFORMED_OUTPUT_DIR}.
    #
    # In case the  document is not well formed, this  command: prints to
    # stdout a description of the "not well formed" error.
    #
    "${RUNNER}" "${XMLWF}" -p -N -d "${XMLWF_TRANSFORMED_OUTPUT_DIR}" "${XML_FILE_NAME}" > "${XMLWF_NOT_WELL_FORMED_LOG_FILE}" || return $?

    read NOT_WELL_FORMED_ERROR_DESCRIPTION < "${XMLWF_NOT_WELL_FORMED_LOG_FILE}"
    if test -z "${NOT_WELL_FORMED_ERROR_DESCRIPTION}"
    then
        # Not  all  the  source  files have  a  precomputed  transformed
        # version.
        if test -f "${XML_PRECOMPUTED_TRANSFORMED_PATHNAME}"
        then
            "${DIFF}" "${XML_PRECOMPUTED_TRANSFORMED_PATHNAME}" "${XMLWF_TRANSFORMED_OUTPUT_FILE}" > "${XMLWF_TRANSFORMED_DIFF_FILE}"

            if test -s "${XMLWF_TRANSFORMED_DIFF_FILE}"
            then
                # The diff file exists and it is not empty.
                print_log_message 'Output differs: %s/%s' "${XML_RELATIVE_SRCDIR}" "${XML_FILE_NAME}"
                return 1
            fi
        fi
        return 0
    else
        print_log_message 'in %s: %s' "${XML_RELATIVE_SRCDIR}" "${NOT_WELL_FORMED_ERROR_DESCRIPTION}"
        return 1
    fi
}


#### utility functions

function print_error_message () {
    local -r TEMPLATE="${1:?missing template argument in call to ${FUNCNAME}}"
    shift
    printf "xmltest.sh: ${TEMPLATE}\n" "$@" >&2
}

function print_verbose_message () {
    local -r TEMPLATE="${1:?missing template argument in call to ${FUNCNAME}}"
    shift
    if ${VERBOSE}
    then printf "xmltest.sh: ${TEMPLATE}\n" "$@" >&2
    fi
}

function print_log_message () {
    local -r TEMPLATE="${1:?missing template argument in call to ${FUNCNAME}}"
    shift
    printf "${TEMPLATE}\n" "$@"
}

function make_directory () {
    local -r PATHNAME="${1:?missing directory pathname argument in call to ${FUNCNAME}}"

    if ! test -d "${PATHNAME}"
    then mkdir -p "${PATHNAME}" || return $?
    fi
    return 0
}

function update_test_result_status () {
    local -r EXIT_STATUS="${1:?missing exit status argument in call to ${FUNCNAME}}"

    if test "${EXIT_STATUS}" -eq 0
    then let ++SUCCESS
    else let ++ERROR
    fi
}


#### done

main "$@"

### end of file
