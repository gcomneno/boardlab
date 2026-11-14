cmake_minimum_required(VERSION 3.16)

project(RpgManagerCalcio VERSION 0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

function(rmc_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Werror
        )
    endif()
endfunction()

# Shared BoardLab C11 algorithms.
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../boardlab_c11
    ${CMAKE_CURRENT_BINARY_DIR}/boardlab_c11
)

# Game-specific deterministic stream derivation.
add_library(rmc-streams STATIC streams.c)

target_include_directories(rmc-streams PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-streams PUBLIC
    boardlab-c11-rng
)

rmc_warnings(rmc-streams)

enable_testing()

# Game-specific stream qualification.
# Canonical PCG32 tests belong to BoardLab C11.
add_executable(rmc-test-streams test_streams.c)

target_link_libraries(rmc-test-streams PRIVATE
    rmc-streams
)

if(MSVC)
    target_compile_options(rmc-test-streams PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-streams PRIVATE -UNDEBUG)
endif()

rmc_warnings(rmc-test-streams)

add_test(NAME rmc-streams COMMAND rmc-test-streams)

# Canonical deterministic stream vectors, derivation v1.
add_executable(rmc-test-stream-vectors test_stream_vectors.c)

target_link_libraries(rmc-test-stream-vectors PRIVATE
    rmc-streams
)

rmc_warnings(rmc-test-stream-vectors)

add_test(
    NAME rmc-stream-vectors
    COMMAND rmc-test-stream-vectors
)

# Game-specific persistent entity identity.
add_library(rmc-identity STATIC identity.c)

target_include_directories(rmc-identity PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-identity PUBLIC
    boardlab-c11-ids
)

rmc_warnings(rmc-identity)

add_executable(rmc-test-identity test_identity.c)

target_link_libraries(rmc-test-identity PRIVATE
    rmc-identity
)

if(MSVC)
    target_compile_options(rmc-test-identity PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-identity PRIVATE -UNDEBUG)
endif()

rmc_warnings(rmc-test-identity)

add_test(
    NAME rmc-identity
    COMMAND rmc-test-identity
)

# Football-specific roster validation and formation coverage.
add_library(rmc-roster STATIC roster.c)

target_include_directories(rmc-roster PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-roster PUBLIC
    boardlab-c11-matching
)

rmc_warnings(rmc-roster)

add_executable(rmc-test-roster test_roster.c)

target_link_libraries(rmc-test-roster PRIVATE
    rmc-roster
)

if(MSVC)
    target_compile_options(rmc-test-roster PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-roster PRIVATE -UNDEBUG)
endif()

rmc_warnings(rmc-test-roster)

add_test(
    NAME rmc-roster
    COMMAND rmc-test-roster
)

# Deterministic pre-calendar football world core.
add_library(rmc-world STATIC world.c)

target_include_directories(rmc-world PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-world PUBLIC
    rmc-streams
    rmc-identity
    rmc-roster
)

rmc_warnings(rmc-world)

add_executable(rmc-test-world test_world.c)

target_link_libraries(rmc-test-world PRIVATE
    rmc-world
)

if(MSVC)
    target_compile_options(rmc-test-world PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-world PRIVATE -UNDEBUG)
endif()

rmc_warnings(rmc-test-world)

add_test(
    NAME rmc-world
    COMMAND rmc-test-world
)

# Deterministic double round-robin football calendar.
add_library(rmc-calendar STATIC calendar.c)

target_include_directories(rmc-calendar PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-calendar PUBLIC
    rmc-world
    boardlab-c11-round-robin
)

rmc_warnings(rmc-calendar)

add_executable(rmc-test-calendar test_calendar.c)

target_link_libraries(rmc-test-calendar PRIVATE
    rmc-calendar
)

if(MSVC)
    target_compile_options(rmc-test-calendar PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-calendar PRIVATE -UNDEBUG)
endif()

rmc_warnings(rmc-test-calendar)

add_test(
    NAME rmc-calendar
    COMMAND rmc-test-calendar
)

# Validated pre-candidate career foundation.
add_library(rmc-career STATIC career.c)

target_include_directories(rmc-career PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-career PUBLIC
    rmc-calendar
)

rmc_warnings(rmc-career)

add_executable(rmc-test-career test_career.c)

target_link_libraries(rmc-test-career PRIVATE
    rmc-career
)

if(MSVC)
    target_compile_options(rmc-test-career PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-career PRIVATE -UNDEBUG)
endif()

rmc_warnings(rmc-test-career)

add_test(
    NAME rmc-career
    COMMAND rmc-test-career
)

# Football-specific optimal legal-lineup strength assessment.
add_library(rmc-strength STATIC strength.c)

target_include_directories(rmc-strength PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-strength PUBLIC
    rmc-world
    boardlab-c11-weighted-matching
)

rmc_warnings(rmc-strength)

add_executable(rmc-test-strength test_strength.c)

target_link_libraries(rmc-test-strength PRIVATE
    rmc-strength
)

if(MSVC)
    target_compile_options(rmc-test-strength PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-strength PRIVATE -UNDEBUG)
endif()

rmc_warnings(rmc-test-strength)

add_test(
    NAME rmc-strength
    COMMAND rmc-test-strength
)

# Deterministic selection of three diverse generated teams.
add_library(rmc-candidates STATIC candidates.c)

target_include_directories(rmc-candidates PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-candidates PUBLIC
    rmc-strength
)

rmc_warnings(rmc-candidates)

add_executable(rmc-test-candidates test_candidates.c)

target_link_libraries(rmc-test-candidates PRIVATE
    rmc-candidates
)

if(MSVC)
    target_compile_options(rmc-test-candidates PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-candidates PRIVATE -UNDEBUG)
endif()

rmc_warnings(rmc-test-candidates)

add_test(
    NAME rmc-candidates
    COMMAND rmc-test-candidates
)

# Career setup reuses the canonical candidate selector.
# Keep the existing career foundation and calendar unchanged.
target_link_libraries(rmc-career PUBLIC rmc-candidates)
