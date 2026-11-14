cmake_minimum_required(VERSION 3.16)

project(BoardLabC11 VERSION 0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

function(boardlab_c11_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Werror
        )
    endif()
endfunction()

add_library(boardlab-c11-rng STATIC rng.c)

target_include_directories(boardlab-c11-rng PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

boardlab_c11_warnings(boardlab-c11-rng)

enable_testing()

add_executable(boardlab-c11-test-rng test_rng.c)

target_link_libraries(boardlab-c11-test-rng
    PRIVATE boardlab-c11-rng
)

if(MSVC)
    target_compile_options(boardlab-c11-test-rng PRIVATE /UNDEBUG)
else()
    target_compile_options(boardlab-c11-test-rng PRIVATE -UNDEBUG)
endif()

boardlab_c11_warnings(boardlab-c11-test-rng)

add_test(
    NAME boardlab-c11-rng
    COMMAND boardlab-c11-test-rng
)

# General-purpose monotonic persistent ID allocation.
add_library(boardlab-c11-ids STATIC ids.c)

target_include_directories(boardlab-c11-ids PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

boardlab_c11_warnings(boardlab-c11-ids)

add_executable(boardlab-c11-test-ids test_ids.c)

target_link_libraries(boardlab-c11-test-ids PRIVATE
    boardlab-c11-ids
)

if(MSVC)
    target_compile_options(boardlab-c11-test-ids PRIVATE /UNDEBUG)
else()
    target_compile_options(boardlab-c11-test-ids PRIVATE -UNDEBUG)
endif()

boardlab_c11_warnings(boardlab-c11-test-ids)

add_test(
    NAME boardlab-c11-ids
    COMMAND boardlab-c11-test-ids
)

# General-purpose deterministic bipartite matching.
add_library(boardlab-c11-matching STATIC matching.c)

target_include_directories(boardlab-c11-matching PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

boardlab_c11_warnings(boardlab-c11-matching)

add_executable(boardlab-c11-test-matching test_matching.c)

target_link_libraries(boardlab-c11-test-matching PRIVATE
    boardlab-c11-matching
)

if(MSVC)
    target_compile_options(boardlab-c11-test-matching PRIVATE /UNDEBUG)
else()
    target_compile_options(boardlab-c11-test-matching PRIVATE -UNDEBUG)
endif()

boardlab_c11_warnings(boardlab-c11-test-matching)

add_test(
    NAME boardlab-c11-matching
    COMMAND boardlab-c11-test-matching
)

# General-purpose deterministic single round-robin scheduling.
add_library(boardlab-c11-round-robin STATIC round_robin.c)

target_include_directories(boardlab-c11-round-robin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

boardlab_c11_warnings(boardlab-c11-round-robin)

add_executable(boardlab-c11-test-round-robin test_round_robin.c)

target_link_libraries(boardlab-c11-test-round-robin PRIVATE
    boardlab-c11-round-robin
)

if(MSVC)
    target_compile_options(boardlab-c11-test-round-robin PRIVATE /UNDEBUG)
else()
    target_compile_options(boardlab-c11-test-round-robin PRIVATE -UNDEBUG)
endif()

boardlab_c11_warnings(boardlab-c11-test-round-robin)

add_test(
    NAME boardlab-c11-round-robin
    COMMAND boardlab-c11-test-round-robin
)

# General-purpose deterministic maximum-weight bipartite matching.
add_library(boardlab-c11-weighted-matching STATIC
    weighted_matching.c
)

target_include_directories(boardlab-c11-weighted-matching PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

boardlab_c11_warnings(boardlab-c11-weighted-matching)

add_executable(boardlab-c11-test-weighted-matching
    test_weighted_matching.c
)

target_link_libraries(boardlab-c11-test-weighted-matching PRIVATE
    boardlab-c11-weighted-matching
)

if(MSVC)
    target_compile_options(
        boardlab-c11-test-weighted-matching PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        boardlab-c11-test-weighted-matching PRIVATE -UNDEBUG
    )
endif()

boardlab_c11_warnings(boardlab-c11-test-weighted-matching)

add_test(
    NAME boardlab-c11-weighted-matching
    COMMAND boardlab-c11-test-weighted-matching
)
