#ifndef RMC_MATCH_FLOW_H
#define RMC_MATCH_FLOW_H

#include "match_opening_kickoff.h"
#include "match_restart_action.h"
#include "match_recovery.h"
#include "match_carry_action.h"
#include "match_dribble_action.h"
#include "match_tackle_action.h"
#include "match_cross_action.h"
#include "match_shot_action.h"

#define RMC_MATCH_FLOW_VERSION UINT32_C(1)
#define RMC_MATCH_SELECTOR_VERSION UINT32_C(1)
#define RMC_MATCH_SELECTOR_V1_VERSION UINT32_C(1)
#define RMC_MATCH_SELECTOR_V2_VERSION UINT32_C(2)
#define RMC_FLOW_DEFAULT_COMMAND_CAPACITY 21616u
#define RMC_FLOW_DEFAULT_EVENT_CAPACITY 36008u
#define RMC_FLOW_DEFAULT_OPERATION_LIMIT UINT64_C(21616)

/* Caller owns bounded storage and immutable world for the flow lifetime.
 * All arguments/storage must be accessible, nonoverlapping, exclusively used.
 * Public fields are observations only; never mutate an initialized flow/prefix
 * or execute a shallow copy (which aliases both backing arrays).
 * Every single-operation error preserves flow, ENTIRE storage and result.
 * No bytes of these structures constitute a portable replay encoding.
 */
typedef enum {
    RMC_FLOW_OK = 0,
    RMC_FLOW_INVALID_ARGUMENT,
    RMC_FLOW_INVALID_STATE,
    RMC_FLOW_WRONG_OPERATION,
    RMC_FLOW_STALE_COMMAND,
    RMC_FLOW_RESOURCE_EXHAUSTED,
    RMC_FLOW_OPERATION_REJECTED,
    RMC_FLOW_UNSUPPORTED_VERSION
} RmcFlowError;

typedef enum {
    RMC_FLOW_INSTRUCTION = 0,
    RMC_FLOW_OPENING,
    RMC_FLOW_CLOSE_PERIOD,
    RMC_FLOW_RESET_WINDOW,
    RMC_FLOW_RESTART,
    RMC_FLOW_RECOVERY,
    RMC_FLOW_PASS,
    RMC_FLOW_CARRY,
    RMC_FLOW_DRIBBLE,
    RMC_FLOW_TACKLE,
    RMC_FLOW_CROSS,
    RMC_FLOW_SHOT
} RmcFlowCommandKind;

typedef enum { RMC_FLOW_PATIENT, RMC_FLOW_DIRECT } RmcFlowApproach;
typedef enum { RMC_FLOW_CARRY_FIRST, RMC_FLOW_DRIBBLE_FIRST } RmcFlowProgression;
typedef enum { RMC_FLOW_PRESS_NORMAL, RMC_FLOW_PRESS_HIGH } RmcFlowPressure;

typedef struct {
    RmcFlowApproach approach;
    RmcFlowProgression progression;
    RmcFlowPressure pressure;
} RmcFlowInstructions;

typedef struct {
    RmcTeamId team_id;
    RmcMatchPeriod window; /* NOT_STARTED or HALF_TIME */
    bool keep;
    RmcFlowInstructions instructions; /* ignored for KEEP, journaled as zero */
} RmcFlowInstructionCommand;

typedef struct {
    uint32_t flow_version;
    uint32_t selector_version; /* Explicit v1/v2; immutable after init. */
    RmcTeamId first_opening_team;
    uint64_t operation_limit; /* 0 = default; otherwise 1..default. */
} RmcMatchFlowConfig;

typedef struct {
    uint64_t command_id;             /* next accepted ordinal, starting at 1 */
    uint64_t observed_command_count; /* stale-state guard */
    RmcMatchId match_id;
    RmcMatchPeriod expected_period;
    RmcFlowCommandKind kind;
    union {
        RmcFlowInstructionCommand instruction;
        RmcOpeningKickoffAction opening;
        RmcRestartAction restart;
        RmcRecoveryAction recovery;
        RmcPassAction pass;
        RmcCarryAction carry;
        RmcDribbleAction dribble;
        RmcTackleAction tackle;
        RmcCrossAction cross;
        RmcShotAction shot;
    } data; /* no payload for close/reset; inactive members ignored */
} RmcMatchFlowCommand;

typedef struct {
    RmcMatchFlowCommand *commands;
    size_t count;
    size_t capacity;
} RmcMatchCommandJournal;

typedef struct {
    const RmcWorldCore *world;
    RmcMatchFlowConfig config;
    RmcMatchState state;
    RmcPcg32 match_rng;
    RmcMatchEventLog log;
    RmcMatchCommandJournal journal;
    RmcFlowInstructions home_instructions;
    RmcFlowInstructions away_instructions;
    bool home_window_submitted;
    bool away_window_submitted;
    uint64_t sporting_count;
    uint64_t reset_count;
} RmcMatchFlow;

typedef enum {
    RMC_FLOW_NEED_INSTRUCTION = 0,
    RMC_FLOW_NEED_OPENING,
    RMC_FLOW_NEED_CLOSURE,
    RMC_FLOW_NEED_RESET,
    RMC_FLOW_NEED_RESTART,
    RMC_FLOW_NEED_RECOVERY,
    RMC_FLOW_NEED_SPORTING_ACTION,
    RMC_FLOW_COMPLETE
} RmcFlowNextKind;

typedef struct {
    RmcFlowNextKind kind;
    RmcTeamId team_id; /* pending instruction/opening/award, else zero */
    uint32_t deadline_second; /* zero before start; active/half-time deadline */
    uint32_t remaining_seconds;
    uint64_t observed_command_count;
} RmcMatchFlowNext;

typedef struct {
    RmcMatchPeriod period;
    RmcMatchMode mode;
    RmcMatchTerminalReason terminal_reason;
    uint32_t at_second;
    uint16_t home_goals;
    uint16_t away_goals;
    uint64_t accepted_commands;
    uint64_t sporting_operations;
    size_t event_count;
    bool complete;
    bool needs_instruction;
} RmcMatchFlowStatus;

typedef struct {
    uint64_t accepted_command_id;
    size_t first_event_index; /* old log count, including event-free commands */
    size_t appended_event_count;
    bool entered_half_time;
    RmcMatchFlowStatus status;
} RmcMatchFlowStepResult;

RmcFlowError rmc_match_flow_init(
    const RmcWorldCore *world, const RmcFixture *fixture,
    const RmcLineup *home, const RmcLineup *away,
    const RmcMatchFlowConfig *config,
    RmcMatchEvent *event_storage, size_t event_capacity,
    RmcMatchFlowCommand *command_storage, size_t command_capacity,
    RmcMatchFlow *destination);

RmcFlowError rmc_match_flow_next(
    const RmcMatchFlow *flow, RmcMatchFlowNext *destination);

RmcFlowError rmc_match_flow_apply(
    RmcMatchFlow *flow, const RmcMatchFlowCommand *command,
    RmcMatchFlowStepResult *destination);

RmcFlowError rmc_match_flow_status(
    const RmcMatchFlow *flow, RmcMatchFlowStatus *destination);

/* Replay recorded decisions without invoking selector. Each successful record
 * commits independently. On error, the already accepted prefix is retained.
 * Records may be a prefix/chunk; source must not overlap destination storage.
 * A full replay starts with an independently initialized flow and identical
 * world, fixture, fixed lineups, versions, config and initial MATCH stream.
 */
RmcFlowError rmc_match_flow_replay(
    RmcMatchFlow *flow, const RmcMatchFlowCommand *records, size_t count);

#endif
