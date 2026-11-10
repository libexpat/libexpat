function(check_threshold value expected)
    execute_process(
        COMMAND "${XMLWF}" -b "${value}" -v
        RESULT_VARIABLE result
        OUTPUT_QUIET
        ERROR_VARIABLE error)
    if(NOT "${result}" STREQUAL "${expected}")
        message(FATAL_ERROR "xmlwf -b '${value}': expected ${expected}, got ${result}: ${error}")
    endif()
    if("${expected}" STREQUAL "4" AND NOT error MATCHES "invalid ignore threshold")
        message(FATAL_ERROR "xmlwf -b '${value}': missing argument diagnostic: ${error}")
    endif()
endfunction()

check_threshold("" 4)
check_threshold("-1" 4)
check_threshold(" \t-1" 4)
check_threshold("-18446744073709551615" 4)
check_threshold("18446744073709551616" 4)
check_threshold("1x" 4)
check_threshold("0" 0)
check_threshold("-0" 0)
check_threshold("1" 0)
check_threshold("+1" 0)
check_threshold(" \t1" 0)
check_threshold("18446744073709551615" 0)
