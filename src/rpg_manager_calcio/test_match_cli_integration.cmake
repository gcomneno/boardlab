cmake_minimum_required(VERSION 3.16)
if(NOT DEFINED CLI OR NOT DEFINED WORK)
    message(FATAL_ERROR "CLI and WORK are required")
endif()
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/empty.txt" "")
file(WRITE "${WORK}/interactive.txt" "\n\n\n\n\n1\n1\n1\n1\n")
file(WRITE "${WORK}/halftime-eof.txt" "\n\n\n\n\n1\n1\n1\n")

function(run_case name expected input)
    execute_process(COMMAND "${CLI}" ${ARGN}
        INPUT_FILE "${WORK}/${input}.txt"
        OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE result TIMEOUT 30)
    if(NOT "${result}" STREQUAL "${expected}")
        message(FATAL_ERROR "${name}: exit=${result}, expected=${expected}\n${out}\n${err}")
    endif()
    if(expected EQUAL 0 AND NOT name STREQUAL "help")
        if(NOT out MATCHES "Intervallo:" OR NOT out MATCHES "Finale:" OR
           NOT out MATCHES "FINISHED/COMPLETED" OR NOT out MATCHES "secondi=5400")
            message(FATAL_ERROR "${name}: missing genuine completion\n${out}")
        endif()
    elseif(NOT expected EQUAL 0)
        if(out MATCHES "Partita completata" OR out MATCHES "Finale:")
            message(FATAL_ERROR "${name}: false completion\n${out}")
        endif()
    endif()
    set(case_output "${out}" PARENT_SCOPE)
endfunction()

run_case(help 0 empty --help)
if(case_output MATCHES "Universo" OR NOT case_output MATCHES "PRESS_NORMAL")
    message(FATAL_ERROR "help generated a world or omitted tokens")
endif()
run_case(auto 0 empty --auto --verify-replay)
if(NOT case_output MATCHES "Replay semantico: PASS")
    message(FATAL_ERROR "real journal replay did not PASS")
endif()
set(auto_output "${case_output}")
run_case(interactive 0 interactive)
foreach(transcript IN ITEMS auto_output case_output)
    if(NOT "${${transcript}}" MATCHES "LIVE 45:00 Inizio tempo" OR
       NOT "${${transcript}}" MATCHES "LIVE 90:00 Fischio finale")
        message(FATAL_ERROR "${transcript}: missing second-half/final commentary")
    endif()
endforeach()
string(FIND "${auto_output}" "Finale:" auto_start)
string(FIND "${case_output}" "Finale:" interactive_start)
string(SUBSTRING "${auto_output}" ${auto_start} -1 auto_summary)
string(SUBSTRING "${case_output}" ${interactive_start} -1 interactive_summary)
if(NOT auto_summary STREQUAL interactive_summary)
    message(FATAL_ERROR "numbered interactive defaults differ from --auto")
endif()
run_case(setup_eof 3 empty)
run_case(half_eof 3 halftime-eof)
if(NOT case_output MATCHES "Intervallo:")
    message(FATAL_ERROR "EOF test did not reach half-time")
endif()
run_case(unknown 2 empty --unknown)
run_case(overflow 2 empty --auto --seed 18446744073709551616)
run_case(missing 2 empty --auto --half DIRECT DRIBBLE_FIRST)

# Constrain the child only; no production allocator hook or global test state.
# A tiny data-segment limit still permits --help but cannot hold flow storage.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    find_program(BASH_EXECUTABLE bash)
    if(BASH_EXECUTABLE)
        execute_process(COMMAND "${BASH_EXECUTABLE}" -c
            "ulimit -d 2048; exec \"$1\" --help" cli-resource "${CLI}"
            INPUT_FILE "${WORK}/empty.txt"
            RESULT_VARIABLE help_result OUTPUT_VARIABLE help_out ERROR_VARIABLE help_err TIMEOUT 10)
        if(NOT help_result EQUAL 0)
            message(FATAL_ERROR "constrained --help failed: ${help_result}: ${help_err}")
        endif()
        execute_process(COMMAND "${BASH_EXECUTABLE}" -c
            "ulimit -d 2048; exec \"$1\" --auto" cli-resource "${CLI}"
            INPUT_FILE "${WORK}/empty.txt"
            RESULT_VARIABLE resource_result OUTPUT_VARIABLE resource_out ERROR_VARIABLE resource_err TIMEOUT 10)
        if(NOT resource_result EQUAL 1 OR resource_out MATCHES "Partita completata" OR
           NOT resource_err MATCHES "runtime/risorse/I/O")
            message(FATAL_ERROR "resource failure was not propagated: ${resource_result}\n${resource_out}\n${resource_err}")
        endif()
    endif()
endif()
message(STATUS "CLI subprocess integration PASS (finite stdin and timeouts)")
