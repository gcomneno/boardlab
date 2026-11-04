cmake_minimum_required(VERSION 3.16)

project(ChronoOops VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

function(chrono_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
        )
    endif()
endfunction()

add_executable(chrono-oops
    launcher.c
    chrono.c
    mission002.c
    mission003.c
    mission004.c
    mission005.c
    terminal.c
    terminal002.c
    terminal003.c
    terminal004.c
    terminal005.c
)

target_compile_definitions(chrono-oops PRIVATE CHRONO_UNIFIED_CLI)
chrono_warnings(chrono-oops)

enable_testing()

# Test assertions must remain active even with Release's NDEBUG.
function(chrono_test_assertions target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${target} PRIVATE -UNDEBUG)
    endif()
endfunction()

add_executable(chrono-test-001 chrono.c test_chrono.c)
chrono_warnings(chrono-test-001)
chrono_test_assertions(chrono-test-001)
add_test(NAME chrono-mission-001 COMMAND chrono-test-001)

foreach(mission RANGE 2 5)
    set(mission_id "00${mission}")
    add_executable(chrono-test-${mission_id}
        mission${mission_id}.c
        test_mission${mission_id}.c
    )
    chrono_warnings(chrono-test-${mission_id})
    chrono_test_assertions(chrono-test-${mission_id})
    add_test(
        NAME chrono-mission-${mission_id}
        COMMAND chrono-test-${mission_id}
    )
endforeach()
