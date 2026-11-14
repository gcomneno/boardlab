#include "calendar.h"
#include "../boardlab_c11/round_robin.h"

#include <stddef.h>
#include <stdint.h>

static bool team_index(
    const RmcWorldCore *world,
    uint64_t id,
    size_t *index)
{
    if (id == 0) {
        return false;
    }

    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (world->teams[i].id.value == id) {
            *index = i;
            return true;
        }
    }

    return false;
}

bool rmc_calendar_valid(
    const RmcWorldCore *world,
    const RmcIdentityState *ids,
    const RmcCalendar *calendar)
{
    if (world == NULL || ids == NULL || calendar == NULL ||
        !rmc_world_core_valid(world) ||
        !rmc_identity_valid(ids)) {
        return false;
    }

    /*
     * An updated snapshot must never move any
     * persistent identity counter backwards.
     */
    for (size_t kind = 0; kind < RMC_ENTITY_COUNT; ++kind) {
        if (ids->counters[kind].next <
            world->ids.counters[kind].next) {
            return false;
        }
    }

    unsigned meetings[RMC_WORLD_TEAMS][RMC_WORLD_TEAMS] = {{0}};
    uint64_t maximum_match_id = 0;

    for (size_t day = 0; day < RMC_CALENDAR_MATCHDAYS; ++day) {
        bool appeared[RMC_WORLD_TEAMS] = {false};

        for (size_t slot = 0;
             slot < RMC_CALENDAR_MATCHES_PER_DAY;
             ++slot) {
            const size_t position =
                day * RMC_CALENDAR_MATCHES_PER_DAY + slot;
            const RmcFixture *fixture =
                &calendar->fixtures[position];

            size_t home = 0;
            size_t away = 0;

            if (fixture->id.value == 0 ||
                fixture->matchday != day + 1 ||
                !team_index(
                    world, fixture->home_team.value, &home) ||
                !team_index(
                    world, fixture->away_team.value, &away) ||
                home == away ||
                appeared[home] ||
                appeared[away]) {
                return false;
            }

            appeared[home] = true;
            appeared[away] = true;

            if (++meetings[home][away] > 1) {
                return false;
            }

            if (fixture->id.value > maximum_match_id) {
                maximum_match_id = fixture->id.value;
            }

            /*
             * MatchId uniqueness is global, not
             * merely local to a matchday.
             */
            for (size_t previous = 0;
                 previous < position;
                 ++previous) {
                if (calendar->fixtures[previous].id.value ==
                    fixture->id.value) {
                    return false;
                }
            }
        }

        for (size_t team = 0; team < RMC_WORLD_TEAMS; ++team) {
            if (!appeared[team]) {
                return false;
            }
        }
    }

    for (size_t a = 0; a < RMC_WORLD_TEAMS; ++a) {
        for (size_t b = a + 1; b < RMC_WORLD_TEAMS; ++b) {
            if (meetings[a][b] != 1 ||
                meetings[b][a] != 1) {
                return false;
            }
        }
    }

    return ids->counters[RMC_ENTITY_MATCH].next >
           maximum_match_id;
}

bool rmc_calendar_generate(
    const RmcWorldCore *world,
    RmcIdentityState *ids,
    RmcCalendar *destination)
{
    if (world == NULL || ids == NULL || destination == NULL ||
        !rmc_world_core_valid(world) ||
        !rmc_identity_valid(ids)) {
        return false;
    }

    /*
     * A supplied persistent state cannot precede
     * the identifiers already stored in the world.
     */
    for (size_t kind = 0; kind < RMC_ENTITY_COUNT; ++kind) {
        if (ids->counters[kind].next <
            world->ids.counters[kind].next) {
            return false;
        }
    }

    RmcPcg32 rng;

    if (!rmc_stream_init(
            &rng, &world->identity,
            RMC_STREAM_CALENDAR, 0)) {
        return false;
    }

    /*
     * The shared scheduler operates on indices.
     * The shuffled permutation maps these indices
     * to the world's persistent team identities.
     */
    size_t teams[RMC_WORLD_TEAMS];

    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        teams[i] = i;
    }

    /*
     * Fisher-Yates: bounded draws avoid modulo bias.
     */
    for (size_t remaining = RMC_WORLD_TEAMS;
         remaining > 1;
         --remaining) {
        uint32_t selected = 0;

        if (!rmc_pcg32_bounded(
                &rng, (uint32_t)remaining, &selected)) {
            return false;
        }

        const size_t last = remaining - 1;
        const size_t temporary = teams[last];

        teams[last] = teams[selected];
        teams[selected] = temporary;
    }

    enum {
        FIRST_LEG_DAYS = RMC_WORLD_TEAMS - 1,
        FIRST_LEG_FIXTURES =
            FIRST_LEG_DAYS * RMC_CALENDAR_MATCHES_PER_DAY
    };

    BoardlabRoundRobinPair pairs[FIRST_LEG_FIXTURES];

    if (!boardlab_round_robin_generate(
            RMC_WORLD_TEAMS,
            pairs,
            FIRST_LEG_FIXTURES)) {
        return false;
    }

    RmcIdentityState next_ids = *ids;
    RmcCalendar next_calendar = {0};

    /*
     * The second leg mirrors the corresponding
     * first-leg fixture with home and away reversed.
     */
    for (size_t day = 0; day < FIRST_LEG_DAYS; ++day) {
        for (size_t slot = 0;
             slot < RMC_CALENDAR_MATCHES_PER_DAY;
             ++slot) {
            const size_t index =
                day * RMC_CALENDAR_MATCHES_PER_DAY + slot;

            const size_t first = teams[pairs[index].first];
            const size_t second = teams[pairs[index].second];

            RmcTeamId home = world->teams[first].id;
            RmcTeamId away = world->teams[second].id;

            /*
             * Deterministic orientation for the
             * first leg; the return leg reverses it.
             */
            if (((day + slot) & 1u) != 0u) {
                const RmcTeamId temporary = home;
                home = away;
                away = temporary;
            }

            uint64_t match_id = 0;

            if (!rmc_identity_next(
                    &next_ids, RMC_ENTITY_MATCH, &match_id)) {
                return false;
            }

            RmcFixture *outbound =
                &next_calendar.fixtures[index];

            outbound->id.value = match_id;
            outbound->home_team = home;
            outbound->away_team = away;
            outbound->matchday = (uint8_t)(day + 1);
        }
    }

    for (size_t day = 0; day < FIRST_LEG_DAYS; ++day) {
        for (size_t slot = 0;
             slot < RMC_CALENDAR_MATCHES_PER_DAY;
             ++slot) {
            const size_t index =
                day * RMC_CALENDAR_MATCHES_PER_DAY + slot;
            const size_t return_index =
                FIRST_LEG_FIXTURES + index;

            const RmcFixture *outbound =
                &next_calendar.fixtures[index];
            RmcFixture *return_fixture =
                &next_calendar.fixtures[return_index];

            uint64_t match_id = 0;

            if (!rmc_identity_next(
                    &next_ids, RMC_ENTITY_MATCH, &match_id)) {
                return false;
            }

            return_fixture->id.value = match_id;
            return_fixture->home_team = outbound->away_team;
            return_fixture->away_team = outbound->home_team;
            return_fixture->matchday =
                (uint8_t)(FIRST_LEG_DAYS + day + 1);
        }
    }

    if (!rmc_calendar_valid(
            world, &next_ids, &next_calendar)) {
        return false;
    }

    /*
     * Atomic publication: no caller-visible state
     * changes before full validation succeeds.
     */
    *destination = next_calendar;
    *ids = next_ids;

    return true;
}
