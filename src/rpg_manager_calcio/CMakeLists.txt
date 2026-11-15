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

# M001-B — Football-specific match state validation.
add_library(rmc-match-state STATIC match_state.c)

target_include_directories(rmc-match-state PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-state PUBLIC
    rmc-calendar
)

rmc_warnings(rmc-match-state)

add_executable(rmc-test-match-state test_match_state.c)

target_link_libraries(rmc-test-match-state PRIVATE
    rmc-match-state
)

if(MSVC)
    target_compile_options(rmc-test-match-state PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-match-state PRIVATE -UNDEBUG)
endif()

rmc_warnings(rmc-test-match-state)

add_test(
    NAME rmc-match-state
    COMMAND rmc-test-match-state
)

# M001-B — Deterministic match lifecycle transitions.
add_library(rmc-match-transitions STATIC match_transitions.c)

target_include_directories(rmc-match-transitions PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-transitions PUBLIC
    rmc-match-state
)

rmc_warnings(rmc-match-transitions)

add_executable(rmc-test-match-transitions
    test_match_transitions.c
)

target_link_libraries(rmc-test-match-transitions PRIVATE
    rmc-match-transitions
)

if(MSVC)
    target_compile_options(
        rmc-test-match-transitions PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        rmc-test-match-transitions PRIVATE -UNDEBUG
    )
endif()

rmc_warnings(rmc-test-match-transitions)

add_test(
    NAME rmc-match-transitions
    COMMAND rmc-test-match-transitions
)

# M001-B — Controlled regulation-time clock.
add_library(rmc-match-clock STATIC match_clock.c)

target_include_directories(rmc-match-clock PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-clock PUBLIC
    rmc-match-state
    rmc-match-stoppage
)

rmc_warnings(rmc-match-clock)

add_executable(rmc-test-match-clock test_match_clock.c)

target_link_libraries(rmc-test-match-clock PRIVATE
    rmc-match-clock
    rmc-match-transitions
)

if(MSVC)
    target_compile_options(rmc-test-match-clock PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-match-clock PRIVATE -UNDEBUG)
endif()

rmc_warnings(rmc-test-match-clock)

add_test(
    NAME rmc-match-clock
    COMMAND rmc-test-match-clock
)

# M001-B — Versioned deterministic stoppage-time policy.
add_library(rmc-match-stoppage STATIC match_stoppage.c)

target_include_directories(rmc-match-stoppage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

rmc_warnings(rmc-match-stoppage)

add_executable(rmc-test-match-stoppage
    test_match_stoppage.c
)

target_link_libraries(rmc-test-match-stoppage PRIVATE
    rmc-match-stoppage
)

if(MSVC)
    target_compile_options(
        rmc-test-match-stoppage PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        rmc-test-match-stoppage PRIVATE -UNDEBUG
    )
endif()

rmc_warnings(rmc-test-match-stoppage)

add_test(
    NAME rmc-match-stoppage
    COMMAND rmc-test-match-stoppage
)

# M001-B — Persistent match interruption accounting.
add_library(rmc-match-interruption STATIC match_interruption.c)

target_include_directories(rmc-match-interruption PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-interruption PUBLIC
    rmc-match-state
    rmc-match-stoppage
)

rmc_warnings(rmc-match-interruption)

add_executable(rmc-test-match-interruption
    test_match_interruption.c
)

target_link_libraries(rmc-test-match-interruption PRIVATE
    rmc-match-interruption
    rmc-match-clock
    rmc-match-transitions
)

if(MSVC)
    target_compile_options(
        rmc-test-match-interruption PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        rmc-test-match-interruption PRIVATE -UNDEBUG
    )
endif()

rmc_warnings(rmc-test-match-interruption)

add_test(
    NAME rmc-match-interruption
    COMMAND rmc-test-match-interruption
)

# M001-B — Deterministic possession and period boundary policy.
add_library(rmc-match-sequence STATIC match_sequence.c)

target_include_directories(rmc-match-sequence PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-sequence PUBLIC
    rmc-match-state
    rmc-match-stoppage
)

rmc_warnings(rmc-match-sequence)

add_executable(rmc-test-match-sequence
    test_match_sequence.c
)

target_link_libraries(rmc-test-match-sequence PRIVATE
    rmc-match-sequence
    rmc-match-transitions
)

if(MSVC)
    target_compile_options(
        rmc-test-match-sequence PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        rmc-test-match-sequence PRIVATE -UNDEBUG
    )
endif()

rmc_warnings(rmc-test-match-sequence)

add_test(
    NAME rmc-match-sequence
    COMMAND rmc-test-match-sequence
)

# M001-B — Atomic application of sequence boundaries.
add_library(rmc-match-sequence-apply STATIC
    match_sequence_apply.c
)

target_include_directories(rmc-match-sequence-apply PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-sequence-apply PUBLIC
    rmc-match-sequence
    rmc-match-transitions
)

rmc_warnings(rmc-match-sequence-apply)

add_executable(rmc-test-match-sequence-apply
    test_match_sequence_apply.c
)

target_link_libraries(rmc-test-match-sequence-apply PRIVATE
    rmc-match-sequence-apply
)

if(MSVC)
    target_compile_options(
        rmc-test-match-sequence-apply PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        rmc-test-match-sequence-apply PRIVATE -UNDEBUG
    )
endif()

rmc_warnings(rmc-test-match-sequence-apply)

add_test(
    NAME rmc-match-sequence-apply
    COMMAND rmc-test-match-sequence-apply
)

# M001-B — Structured append-only match event log.
add_library(rmc-match-event-log STATIC match_event_log.c)

target_include_directories(rmc-match-event-log PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-event-log PUBLIC
    rmc-match-state
)

rmc_warnings(rmc-match-event-log)

add_executable(rmc-test-match-event-log
    test_match_event_log.c
)

target_link_libraries(rmc-test-match-event-log PRIVATE
    rmc-match-event-log
    rmc-match-transitions
)

if(MSVC)
    target_compile_options(
        rmc-test-match-event-log PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        rmc-test-match-event-log PRIVATE -UNDEBUG
    )
endif()

rmc_warnings(rmc-test-match-event-log)

add_test(
    NAME rmc-match-event-log
    COMMAND rmc-test-match-event-log
)

# M001-B — Atomic period transition and event recording.
add_library(rmc-match-period-commit STATIC
    match_period_commit.c
)

target_include_directories(rmc-match-period-commit PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-period-commit PUBLIC
    rmc-match-event-log
    rmc-match-sequence-apply
)

rmc_warnings(rmc-match-period-commit)

add_executable(rmc-test-match-period-commit
    test_match_period_commit.c
)

target_link_libraries(rmc-test-match-period-commit PRIVATE
    rmc-match-period-commit
    rmc-match-clock
)

if(MSVC)
    target_compile_options(
        rmc-test-match-period-commit PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        rmc-test-match-period-commit PRIVATE -UNDEBUG
    )
endif()

rmc_warnings(rmc-test-match-period-commit)

add_test(
    NAME rmc-match-period-commit
    COMMAND rmc-test-match-period-commit
)

# M001-B — Frozen integer logistic probability model.
add_library(rmc-match-probability STATIC
    match_probability.c
)

target_include_directories(rmc-match-probability PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

rmc_warnings(rmc-match-probability)

add_executable(rmc-test-match-probability
    test_match_probability.c
)

target_link_libraries(rmc-test-match-probability PRIVATE
    rmc-match-probability
)

if(MSVC)
    target_compile_options(
        rmc-test-match-probability PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        rmc-test-match-probability PRIVATE -UNDEBUG
    )
endif()

rmc_warnings(rmc-test-match-probability)

add_test(
    NAME rmc-match-probability
    COMMAND rmc-test-match-probability
)

# M001-B — First executable football action: pass.
add_library(rmc-match-pass-action STATIC
    match_pass_action.c
)

target_include_directories(rmc-match-pass-action PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-pass-action PUBLIC
    rmc-match-state
    rmc-match-sequence
    rmc-match-clock
    rmc-match-event-log
    rmc-match-probability
    rmc-streams
)

rmc_warnings(rmc-match-pass-action)

add_executable(rmc-test-match-pass-action
    test_match_pass_action.c
)

target_link_libraries(rmc-test-match-pass-action PRIVATE
    rmc-match-pass-action
    rmc-match-transitions
)

if(MSVC)
    target_compile_options(
        rmc-test-match-pass-action PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        rmc-test-match-pass-action PRIVATE -UNDEBUG
    )
endif()

rmc_warnings(rmc-test-match-pass-action)

add_test(
    NAME rmc-match-pass-action
    COMMAND rmc-test-match-pass-action
)

# M001-B — Contested loose-ball recovery.
add_library(rmc-match-recovery STATIC
    match_recovery.c
)

target_include_directories(rmc-match-recovery PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-recovery PUBLIC
    rmc-match-pass-action
    rmc-match-tackle-action
    rmc-match-cross-action
    rmc-match-sequence
    rmc-match-clock
    rmc-match-event-log
    rmc-match-probability
)

rmc_warnings(rmc-match-recovery)

add_executable(rmc-test-match-recovery
    test_match_recovery.c
)

target_link_libraries(rmc-test-match-recovery PRIVATE
    rmc-match-recovery
    rmc-match-shot-action
    rmc-match-transitions
    rmc-match-sequence-apply
)

if(MSVC)
    target_compile_options(
        rmc-test-match-recovery PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        rmc-test-match-recovery PRIVATE -UNDEBUG
    )
endif()

rmc_warnings(rmc-test-match-recovery)

add_test(
    NAME rmc-match-recovery
    COMMAND rmc-test-match-recovery
)

# M001-B1.21 — Ball carry action.
add_library(rmc-match-carry-action STATIC
    match_carry_action.c
)

target_include_directories(rmc-match-carry-action PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-carry-action PUBLIC
    rmc-match-clock
    rmc-match-sequence
    rmc-match-event-log
    rmc-match-probability
    rmc-streams
)

rmc_warnings(rmc-match-carry-action)

add_executable(rmc-test-match-carry-action
    test_match_carry_action.c
)

target_link_libraries(rmc-test-match-carry-action PRIVATE
    rmc-match-carry-action
    rmc-match-transitions
)

if(MSVC)
    target_compile_options(
        rmc-test-match-carry-action PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        rmc-test-match-carry-action PRIVATE -UNDEBUG
    )
endif()

rmc_warnings(rmc-test-match-carry-action)

add_test(
    NAME rmc-match-carry-action
    COMMAND rmc-test-match-carry-action
)

# M001-B1.23 — Contested dribbling action.
add_library(rmc-match-dribble-action STATIC
    match_dribble_action.c
)

target_include_directories(rmc-match-dribble-action PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-dribble-action PUBLIC
    rmc-match-clock
    rmc-match-sequence
    rmc-match-event-log
    rmc-match-probability
    rmc-streams
)

rmc_warnings(rmc-match-dribble-action)

add_executable(rmc-test-match-dribble-action
    test_match_dribble_action.c
)

target_link_libraries(rmc-test-match-dribble-action PRIVATE
    rmc-match-dribble-action
    rmc-match-transitions
)

if(MSVC)
    target_compile_options(
        rmc-test-match-dribble-action PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        rmc-test-match-dribble-action PRIVATE -UNDEBUG
    )
endif()

rmc_warnings(rmc-test-match-dribble-action)

add_test(
    NAME rmc-match-dribble-action
    COMMAND rmc-test-match-dribble-action
)

# M001-B1.24 — Defender-initiated tackle action.
add_library(rmc-match-tackle-action STATIC
    match_tackle_action.c
)

target_include_directories(rmc-match-tackle-action PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-tackle-action PUBLIC
    rmc-match-clock
    rmc-match-sequence
    rmc-match-event-log
    rmc-match-probability
    rmc-streams
)

rmc_warnings(rmc-match-tackle-action)

add_executable(rmc-test-match-tackle-action
    test_match_tackle_action.c
)

target_link_libraries(rmc-test-match-tackle-action PRIVATE
    rmc-match-tackle-action
    rmc-match-transitions
)

if(MSVC)
    target_compile_options(
        rmc-test-match-tackle-action PRIVATE /UNDEBUG
    )
else()
    target_compile_options(
        rmc-test-match-tackle-action PRIVATE -UNDEBUG
    )
endif()

rmc_warnings(rmc-test-match-tackle-action)

add_test(
    NAME rmc-match-tackle-action
    COMMAND rmc-test-match-tackle-action
)

# M001-B1.25 — Cross action and causal loose-ball recovery.
add_library(rmc-match-cross-action STATIC match_cross_action.c)

target_include_directories(rmc-match-cross-action PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rmc-match-cross-action PUBLIC
    rmc-match-clock
    rmc-match-sequence
    rmc-match-event-log
    rmc-match-probability
    rmc-streams
)

rmc_warnings(rmc-match-cross-action)

add_executable(rmc-test-match-cross-action test_match_cross_action.c)

target_link_libraries(rmc-test-match-cross-action PRIVATE
    rmc-match-cross-action
    rmc-match-transitions
)

if(MSVC)
    target_compile_options(rmc-test-match-cross-action PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-match-cross-action PRIVATE -UNDEBUG)
endif()

rmc_warnings(rmc-test-match-cross-action)

add_test(NAME rmc-match-cross-action COMMAND rmc-test-match-cross-action)

# M001-B1.26-B — Frozen Shot v1 and atomic deadline closure.
add_library(rmc-match-shot-action STATIC match_shot_action.c)
target_include_directories(rmc-match-shot-action PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(rmc-match-shot-action PUBLIC
    rmc-match-clock
    rmc-match-period-commit
    rmc-match-probability
    rmc-streams
)
rmc_warnings(rmc-match-shot-action)

add_executable(rmc-test-match-shot-action test_match_shot_action.c)
target_link_libraries(rmc-test-match-shot-action PRIVATE rmc-match-shot-action)
if(MSVC)
    target_compile_options(rmc-test-match-shot-action PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-match-shot-action PRIVATE -UNDEBUG)
endif()
rmc_warnings(rmc-test-match-shot-action)
add_test(NAME rmc-match-shot-action COMMAND rmc-test-match-shot-action)

# M001-B1.27-B — Instantaneous causal Shot v1 restarts.
add_library(rmc-match-restart-action STATIC match_restart_action.c)
target_include_directories(rmc-match-restart-action PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(rmc-match-restart-action PUBLIC
    rmc-match-event-log
    rmc-match-sequence
    rmc-match-stoppage
)
rmc_warnings(rmc-match-restart-action)

add_executable(rmc-test-match-restart-action test_match_restart_action.c)
target_link_libraries(rmc-test-match-restart-action PRIVATE
    rmc-match-restart-action
    rmc-match-shot-action
    rmc-match-pass-action
)
if(MSVC)
    target_compile_options(rmc-test-match-restart-action PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-match-restart-action PRIVATE -UNDEBUG)
endif()
# Exercise allocation failure before commit without adding a production hook.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(rmc-test-match-restart-action PRIVATE RMC_WRAP_MALLOC)
    target_link_options(rmc-test-match-restart-action PRIVATE "-Wl,--wrap=malloc")
endif()
rmc_warnings(rmc-test-match-restart-action)
add_test(NAME rmc-match-restart-action COMMAND rmc-test-match-restart-action)

# M001-B1.28-B — Separate atomic period-opening kickoffs.
add_library(rmc-match-opening-kickoff STATIC match_opening_kickoff.c)
target_include_directories(rmc-match-opening-kickoff PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(rmc-match-opening-kickoff PUBLIC
    rmc-match-event-log
    rmc-match-sequence
    rmc-match-stoppage
)
rmc_warnings(rmc-match-opening-kickoff)

add_executable(rmc-test-match-opening-kickoff test_match_opening_kickoff.c)
target_link_libraries(rmc-test-match-opening-kickoff PRIVATE
    rmc-match-opening-kickoff
    rmc-match-transitions
    rmc-match-restart-action
    rmc-match-shot-action
    rmc-match-pass-action
    rmc-match-carry-action
    rmc-match-dribble-action
    rmc-match-tackle-action
    rmc-match-recovery
    rmc-match-interruption
)
if(MSVC)
    target_compile_options(rmc-test-match-opening-kickoff PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-match-opening-kickoff PRIVATE -UNDEBUG)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(rmc-test-match-opening-kickoff PRIVATE RMC_WRAP_MALLOC)
    target_link_options(rmc-test-match-opening-kickoff PRIVATE "-Wl,--wrap=malloc")
endif()
rmc_warnings(rmc-test-match-opening-kickoff)
add_test(NAME rmc-match-opening-kickoff COMMAND rmc-test-match-opening-kickoff)

# M001-B1.28-C — Stateful flow and pure Selector v1; existing engines unchanged.
add_library(rmc-match-flow STATIC match_flow.c)
target_include_directories(rmc-match-flow PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rmc-match-flow PUBLIC
    rmc-match-opening-kickoff rmc-match-restart-action rmc-match-period-commit
    rmc-match-pass-action rmc-match-carry-action rmc-match-dribble-action
    rmc-match-tackle-action rmc-match-cross-action rmc-match-shot-action
    rmc-match-recovery)
rmc_warnings(rmc-match-flow)

add_library(rmc-match-flow-selector STATIC match_flow_selector.c)
target_link_libraries(rmc-match-flow-selector PUBLIC rmc-match-flow)
rmc_warnings(rmc-match-flow-selector)

foreach(flow_test IN ITEMS match-flow match-flow-replay)
    string(REPLACE "-" "_" flow_source "${flow_test}")
    add_executable(rmc-test-${flow_test} test_${flow_source}.c)
    target_link_libraries(rmc-test-${flow_test} PRIVATE rmc-match-flow-selector rmc-career)
    if(MSVC)
        target_compile_options(rmc-test-${flow_test} PRIVATE /UNDEBUG)
    else()
        target_compile_options(rmc-test-${flow_test} PRIVATE -UNDEBUG)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_definitions(rmc-test-${flow_test} PRIVATE RMC_WRAP_MALLOC)
        target_link_options(rmc-test-${flow_test} PRIVATE "-Wl,--wrap=malloc")
    endif()
    rmc_warnings(rmc-test-${flow_test})
    add_test(NAME rmc-test-${flow_test} COMMAND rmc-test-${flow_test})
endforeach()

# M001-B1.28-D — Playable managerial stdio adapter over qualified Match Flow.
add_library(rmc-match-cli-runner STATIC match_cli.c)
target_link_libraries(rmc-match-cli-runner PUBLIC rmc-match-flow-selector rmc-career)
rmc_warnings(rmc-match-cli-runner)

add_executable(rmc-match-cli match_cli_main.c)
set_target_properties(rmc-match-cli PROPERTIES OUTPUT_NAME rmc-match)
target_link_libraries(rmc-match-cli PRIVATE rmc-match-cli-runner)
rmc_warnings(rmc-match-cli)

add_executable(rmc-test-match-cli test_match_cli.c)
target_link_libraries(rmc-test-match-cli PRIVATE rmc-match-cli-runner)
if(MSVC)
    target_compile_options(rmc-test-match-cli PRIVATE /UNDEBUG)
else()
    target_compile_options(rmc-test-match-cli PRIVATE -UNDEBUG)
endif()
rmc_warnings(rmc-test-match-cli)
add_test(NAME rmc-match-cli COMMAND rmc-test-match-cli)
set_tests_properties(rmc-match-cli PROPERTIES TIMEOUT 180)
add_test(NAME rmc-match-cli-integration
    COMMAND ${CMAKE_COMMAND}
        -DCLI=$<TARGET_FILE:rmc-match-cli>
        -DWORK=${CMAKE_CURRENT_BINARY_DIR}/match-cli-integration
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test_match_cli_integration.cmake)
set_tests_properties(rmc-match-cli-integration PROPERTIES TIMEOUT 120)

# M001-SBL-A — Read-only measurement over production flow and selectors.
add_executable(rmc-sporting-balance sporting_balance.c)
target_link_libraries(rmc-sporting-balance PRIVATE rmc-match-flow-selector rmc-career)
rmc_warnings(rmc-sporting-balance)

# Same translation unit, isolated reducer/oracle and resource-prefix checks.
# No test switches or failure injection are exposed by the public executable.
add_executable(rmc-sporting-balance-selfcheck sporting_balance.c)
target_compile_definitions(rmc-sporting-balance-selfcheck PRIVATE RMC_SBL_SELF_CHECK)
target_link_libraries(rmc-sporting-balance-selfcheck PRIVATE rmc-match-flow-selector rmc-career)
rmc_warnings(rmc-sporting-balance-selfcheck)
add_test(NAME rmc-sporting-balance-smoke
    COMMAND ${CMAKE_COMMAND}
        -DLAB=$<TARGET_FILE:rmc-sporting-balance>
        -DCHECK=$<TARGET_FILE:rmc-sporting-balance-selfcheck>
        -DWORK=${CMAKE_CURRENT_BINARY_DIR}/sporting-balance-smoke
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test_sporting_balance.cmake)
set_tests_properties(rmc-sporting-balance-smoke PROPERTIES TIMEOUT 120)
