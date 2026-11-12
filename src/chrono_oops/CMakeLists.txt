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
    input.c
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

add_executable(chrono-test-input input.c test_input.c)
chrono_warnings(chrono-test-input)
chrono_test_assertions(chrono-test-input)
add_test(NAME chrono-input COMMAND chrono-test-input)

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

# Optional native 2D frontend. The deterministic C11 engine is shared with CLI.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(CHRONO_SDL2 QUIET IMPORTED_TARGET sdl2)
    pkg_check_modules(CHRONO_SDL2_TTF QUIET IMPORTED_TARGET SDL2_ttf)
endif()

if(TARGET PkgConfig::CHRONO_SDL2 AND TARGET PkgConfig::CHRONO_SDL2_TTF)
    add_executable(chrono-oops-2d
        graphics2d.c
        chrono.c
    )
    target_link_libraries(chrono-oops-2d PRIVATE
        PkgConfig::CHRONO_SDL2
        PkgConfig::CHRONO_SDL2_TTF
    )
    chrono_warnings(chrono-oops-2d)
endif()

# Regression of the actual M001 GUI action table and hitboxes.
# The included GUI main is renamed by the test translation unit.
if(TARGET chrono-oops-2d)
    add_executable(chrono-test-2d-001
        test_graphics2d.c
        chrono.c
    )
    target_link_libraries(chrono-test-2d-001 PRIVATE
        PkgConfig::CHRONO_SDL2
        PkgConfig::CHRONO_SDL2_TTF
    )
    chrono_warnings(chrono-test-2d-001)
    chrono_test_assertions(chrono-test-2d-001)
    add_test(
        NAME chrono-2d-mission-001
        COMMAND chrono-test-2d-001
    )
endif()
