cmake_minimum_required(VERSION 3.16)

project(ChronoOops VERSION 1.3.0 LANGUAGES C)

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
        replay_store.c
        replay_progress.c
        graphics2d.c
        chrono.c
        mission002.c
        mission003.c
        mission004.c
        mission005.c
    )
    target_compile_definitions(chrono-oops-2d PRIVATE CHRONO_WITH_M002 SDL_MAIN_HANDLED)
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
        replay_store.c
        replay_progress.c
        test_graphics2d.c
        chrono.c
    )
    target_link_libraries(chrono-test-2d-001 PRIVATE
        PkgConfig::CHRONO_SDL2
        PkgConfig::CHRONO_SDL2_TTF
    )
    target_compile_definitions(chrono-test-2d-001 PRIVATE SDL_MAIN_HANDLED)
    chrono_warnings(chrono-test-2d-001)
    chrono_test_assertions(chrono-test-2d-001)
    add_test(
        NAME chrono-2d-mission-001
        COMMAND chrono-test-2d-001
    )
endif()

# Mission 002 graphical adapter regression: actual SDL input,
# causal debt propagation, paradox atomicity and both outcomes.
if(TARGET chrono-oops-2d)
    add_executable(chrono-test-2d-002
        test_graphics2d_m002.c
        mission002.c
    )
    target_link_libraries(chrono-test-2d-002 PRIVATE
        PkgConfig::CHRONO_SDL2
        PkgConfig::CHRONO_SDL2_TTF
    )
    target_compile_definitions(chrono-test-2d-002 PRIVATE SDL_MAIN_HANDLED)
    chrono_warnings(chrono-test-2d-002)
    chrono_test_assertions(chrono-test-2d-002)
    add_test(
        NAME chrono-2d-mission-002
        COMMAND chrono-test-2d-002
    )
endif()

# Mission 003 graphical adapter regression: SDL events, causal paradox,
# safety propagation, victory, defeats, and restart.
if(TARGET chrono-oops-2d)
    add_executable(chrono-test-2d-003
        test_graphics2d_m003.c
        mission003.c
    )
    target_link_libraries(chrono-test-2d-003 PRIVATE
        PkgConfig::CHRONO_SDL2
        PkgConfig::CHRONO_SDL2_TTF
    )
    target_compile_definitions(chrono-test-2d-003 PRIVATE SDL_MAIN_HANDLED)
    chrono_warnings(chrono-test-2d-003)
    chrono_test_assertions(chrono-test-2d-003)
    add_test(
        NAME chrono-2d-mission-003
        COMMAND chrono-test-2d-003
    )
endif()

# Mission 004 SDL regression: dual causal propagation, reversible
# single-effect paths, verification invalidation, victory and restart.
if(TARGET chrono-oops-2d)
    add_executable(chrono-test-2d-004
        test_graphics2d_m004.c
        mission004.c
    )
    target_link_libraries(chrono-test-2d-004 PRIVATE
        PkgConfig::CHRONO_SDL2
        PkgConfig::CHRONO_SDL2_TTF
    )
    target_compile_definitions(chrono-test-2d-004 PRIVATE SDL_MAIN_HANDLED)
    chrono_warnings(chrono-test-2d-004)
    chrono_test_assertions(chrono-test-2d-004)
    add_test(
        NAME chrono-2d-mission-004
        COMMAND chrono-test-2d-004
    )
endif()

# Mission 005 SDL regression: historical evidence propagation,
# superficial correction recovery, campaign victory and restart.
if(TARGET chrono-oops-2d)
    add_executable(chrono-test-2d-005
        test_graphics2d_m005.c
        mission005.c
    )
    target_link_libraries(chrono-test-2d-005 PRIVATE
        PkgConfig::CHRONO_SDL2
        PkgConfig::CHRONO_SDL2_TTF
    )
    target_compile_definitions(chrono-test-2d-005 PRIVATE SDL_MAIN_HANDLED)
    chrono_warnings(chrono-test-2d-005)
    chrono_test_assertions(chrono-test-2d-005)
    add_test(
        NAME chrono-2d-mission-005
        COMMAND chrono-test-2d-005
    )
endif()

# Native distributable: CLI always, graphical frontend when SDL2 is available.
include(GNUInstallDirs)

install(TARGETS chrono-oops
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT Runtime
)

if(TARGET chrono-oops-2d)
    install(TARGETS chrono-oops-2d
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION .
        COMPONENT Runtime
    )
endif()

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/README.it.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/chrono-oops"
    COMPONENT Runtime
)

set(CPACK_PACKAGE_NAME "chrono-oops")
set(CPACK_PACKAGE_VENDOR "GiadaWare BoardLab")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "ChronoOops! cooperative time-travel campaign")
set(CPACK_PACKAGE_FILE_NAME
    "chrono-oops-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_GENERATOR "TGZ;ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
include(CPack)

# REPLAY-02A: independent C11 progression register.
add_executable(chrono-test-replay-progress
    replay_progress.c
    test_replay_progress.c
)
chrono_warnings(chrono-test-replay-progress)
chrono_test_assertions(chrono-test-replay-progress)
add_test(
    NAME chrono-replay-progress
    COMMAND chrono-test-replay-progress
)

# REPLAY-02B: exercise GUI replay helpers without opening a window.
if(TARGET chrono-oops-2d)
    add_executable(chrono-test-replay-gui-hooks
        replay_store.c
        test_replay_gui_hooks.c
        replay_progress.c
        chrono.c
        mission002.c
        mission003.c
        mission004.c
        mission005.c
    )
    target_link_libraries(chrono-test-replay-gui-hooks PRIVATE
        PkgConfig::CHRONO_SDL2
        PkgConfig::CHRONO_SDL2_TTF
    )
    chrono_warnings(chrono-test-replay-gui-hooks)
    chrono_test_assertions(chrono-test-replay-gui-hooks)
    add_test(
        NAME chrono-replay-gui-hooks
        COMMAND chrono-test-replay-gui-hooks
    )
endif()

# REPLAY-02C: portable, atomic local progress persistence.
add_executable(chrono-test-replay-store
    replay_progress.c
    replay_store.c
    test_replay_store.c
)
chrono_warnings(chrono-test-replay-store)
chrono_test_assertions(chrono-test-replay-store)
add_test(
    NAME chrono-replay-store
    COMMAND chrono-test-replay-store
)
