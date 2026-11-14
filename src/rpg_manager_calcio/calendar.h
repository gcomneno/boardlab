#ifndef RMC_CALENDAR_H
#define RMC_CALENDAR_H

#include "world.h"

#include <stdbool.h>
#include <stdint.h>

enum {
    RMC_CALENDAR_MATCHDAYS = 14,
    RMC_CALENDAR_MATCHES_PER_DAY = RMC_WORLD_TEAMS / 2,
    RMC_CALENDAR_FIXTURES =
        RMC_CALENDAR_MATCHDAYS * RMC_CALENDAR_MATCHES_PER_DAY
};

/*
 * One scheduled match.
 *
 * matchday is 1-based: [1, 14].
 * Team identities are persistent, not array indices.
 *
 * Home and away are explicit and distinct.
 */
typedef struct {
    RmcMatchId id;
    RmcTeamId home_team;
    RmcTeamId away_team;
    uint8_t matchday;
} RmcFixture;

/*
 * Canonical order:
 * - Ascending matchday.
 * - Four fixtures per matchday.
 *
 * This structure contains scheduling data only.
 * Results and match state belong to later components.
 */
typedef struct {
    RmcFixture fixtures[RMC_CALENDAR_FIXTURES];
} RmcCalendar;

/*
 * Generate a deterministic double round-robin calendar.
 *
 * Uses the shared BoardLab C11 round-robin algorithm
 * and the game's CALENDAR random stream.
 *
 * Requirements:
 * - Fourteen matchdays, four fixtures per matchday.
 * - Every team plays exactly once per matchday.
 * - Each pair meets twice, reversing home/away.
 * - MatchIds are unique and persistent.
 *
 * On failure, both destination and IDs remain unchanged.
 *
 * The caller supplies a mutable copy of the persistent
 * identity state; the generated calendar is committed
 * together with its updated ID counters.
 */
bool rmc_calendar_generate(
    const RmcWorldCore *world,
    RmcIdentityState *ids,
    RmcCalendar *destination
);

/*
 * Validate fixture identities, participation,
 * round structure, pair coverage and home/away.
 *
 * The persistent ID state must be consistent with
 * every MatchId appearing in the calendar.
 */
bool rmc_calendar_valid(
    const RmcWorldCore *world,
    const RmcIdentityState *ids,
    const RmcCalendar *calendar
);

#endif
