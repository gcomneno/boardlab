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
    target_compile_definitions(chrono-test-replay-gui-hooks PRIVATE SDL_MAIN_HANDLED)
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

# ChronoOops 3D P0: renderer-independent C11 adapter contracts.
add_executable(chrono-test-3d-presentation
    chrono.c
    presentation3d.c
    test_presentation3d.c
)
add_executable(chrono-test-3d-effects
    effects3d.c
    test_effects3d.c
)
add_executable(chrono-test-3d-input
    chrono.c
    input3d.c
    test_input3d.c
)

foreach(adapter IN ITEMS presentation effects input)
    chrono_warnings(chrono-test-3d-${adapter})
    chrono_test_assertions(chrono-test-3d-${adapter})
    add_test(
        NAME chrono-3d-${adapter}
        COMMAND chrono-test-3d-${adapter}
    )
endforeach()

# P2: menu and input-adapter functional qualification.
add_executable(chrono-test-3d-qualification
    chrono.c
    input3d.c
    action_menu3d.c
    test_qualification3d.c
)
chrono_warnings(chrono-test-3d-qualification)
chrono_test_assertions(chrono-test-3d-qualification)
add_test(
    NAME chrono-3d-qualification
    COMMAND chrono-test-3d-qualification
)

# Pure C11 score qualification.
add_executable(chrono-test-3d-score
    score3d.c
    test_score3d.c
)
chrono_warnings(chrono-test-3d-score)
chrono_test_assertions(chrono-test-3d-score)
add_test(
    NAME chrono-3d-score
    COMMAND chrono-test-3d-score
)

# Arcade Hall of Fame: pure C11 ranking qualification.
add_executable(chrono-test-3d-hall
    hall3d.c
    test_hall3d.c
)
chrono_warnings(chrono-test-3d-hall)
chrono_test_assertions(chrono-test-3d-hall)
add_test(
    NAME chrono-3d-hall
    COMMAND chrono-test-3d-hall
)

# Arcade Hall of Fame: persistent storage qualification.
add_executable(chrono-test-3d-hall-store
    hall3d.c
    hall_store3d.c
    test_hall_store3d.c
)
chrono_warnings(chrono-test-3d-hall-store)
chrono_test_assertions(chrono-test-3d-hall-store)
add_test(
    NAME chrono-3d-hall-store
    COMMAND chrono-test-3d-hall-store
)

# ChronoOops 3D P0: REPLAY adapter contract.
add_executable(chrono-test-3d-replay
    replay_progress.c
    replay_store.c
    replay3d.c
    test_replay3d.c
)
chrono_warnings(chrono-test-3d-replay)
chrono_test_assertions(chrono-test-3d-replay)
add_test(
    NAME chrono-3d-replay
    COMMAND chrono-test-3d-replay
)

# Optional native 3D frontend; never required by CLI, SDL2 or C11 tests.
option(CHRONO_ENABLE_3D "Build ChronoOops raylib 3D frontend" OFF)

if(CHRONO_ENABLE_3D)
    find_package(raylib 5.5 QUIET)

    if(NOT TARGET raylib)
        include(FetchContent)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(chrono_raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG 5.5
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(chrono_raylib)
    endif()

    add_executable(chrono-oops-3d
        graphics3d.c
        score3d.c
        hall3d.c
        hall_store3d.c
        hall_runtime3d.c
        chrono.c
        presentation3d.c
        input3d.c
        action_menu3d.c
        effects3d.c
    )
    target_link_libraries(chrono-oops-3d PRIVATE raylib)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/assets/3d")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/3d/municipality_1985.gltf"
        "${CMAKE_CURRENT_BINARY_DIR}/assets/3d/municipality_1985.gltf"
        COPYONLY
    )
    foreach(asset IN ITEMS archive_1985 tower_2025 tower_hydraulic_2025)
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/3d/${asset}.gltf"
            "${CMAKE_CURRENT_BINARY_DIR}/assets/3d/${asset}.gltf"
            COPYONLY
        )
    endforeach()
    foreach(asset IN ITEMS clerk_1985 explorer_2025)
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/3d/${asset}.gltf"
            "${CMAKE_CURRENT_BINARY_DIR}/assets/3d/${asset}.gltf"
            COPYONLY
        )
    endforeach()
    # Optional Kenney Nature Kit 2.1 scenery (CC0).
    foreach(asset IN ITEMS tree_oak plant_bushDetailed)
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/3d/kenney-nature-kit/${asset}.gltf"
            "${CMAKE_CURRENT_BINARY_DIR}/assets/3d/kenney-nature-kit/${asset}.gltf"
            COPYONLY
        )
    endforeach()
    chrono_warnings(chrono-oops-3d)
endif()
