#include "match_shot_action.h"
#include "match_clock.h"
#include "match_period_commit.h"

#include <stdlib.h>

static const RmcPlayer *field_player(
    const RmcWorldCore *world, RmcTeamId team, const RmcLineup *lineup,
    RmcPlayerId id, bool goalkeeper)
{
    bool assigned = false;
    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        if (id.value != 0 && lineup->players[i].value == id.value &&
            (lineup->slots[i] == RMC_SLOT_POR) == goalkeeper) {
            assigned = true;
        }
    }
    if (!assigned) {
        return NULL;
    }
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (world->teams[i].id.value != team.value ||
            world->rosters[i].team_id.value != team.value) {
            continue;
        }
        for (size_t j = 0; j < RMC_TEAM_PLAYERS; ++j) {
            const RmcPlayer *p = &world->rosters[i].players[j];
            if (p->id.value == id.value && p->team_id.value == team.value &&
                (p->department == RMC_DEPARTMENT_GOALKEEPER) == goalkeeper) {
                return p;
            }
        }
    }
    return NULL;
}

/* Revalidate the entire prefix with the qualified schema validator, without
 * ever writing caller storage. A second-half log can contain first-half shots:
 * replay that prefix against its first-half clock/period, not the current one.
 * The logger's current-lineup/exclusive-access preconditions still apply.
 */
static bool prefix_valid(
    const RmcWorldCore *world, const RmcMatchState *state,
    const RmcMatchEventLog *log)
{
    if (log->count == 0) {
        return true;
    }
    RmcMatchEvent *storage = malloc(log->count * sizeof(*storage));
    if (storage == NULL) {
        return false;
    }
    RmcMatchEventLog check = {
        .events = storage, .capacity = log->count, .match_id = log->match_id
    };
    bool valid = true;
    if (state->period == RMC_MATCH_SECOND_HALF) {
        const uint32_t first_deadline =
            UINT32_C(2700) + state->first_half_stoppage_seconds;
        size_t first_count = 0;
        while (first_count < log->count &&
               log->events[first_count].at_second <= first_deadline) {
            ++first_count;
        }
        if (first_count != 0) {
            RmcMatchState first = *state;
            first.period = RMC_MATCH_FIRST_HALF;
            first.elapsed_seconds = first_deadline;
            first.second_half_start_second = 0;
            first.second_half_stoppage_seconds = 0;
            first.possession = (RmcMatchPossession){0};
            valid = rmc_match_event_log_append_batch(
                world, &first, &check, log->events, first_count);
        }
    }
    if (valid && check.count < log->count) {
        valid = rmc_match_event_log_append_batch(
            world, state, &check, &log->events[check.count],
            log->count - check.count);
    }
    free(storage);
    return valid;
}

static uint32_t clamp(int32_t value, int32_t low, int32_t high)
{
    return (uint32_t)(value < low ? low : (value > high ? high : value));
}

/* Validated operands are 0..100; the numerator is at most 10000.
 * All floors here precede subsequent sums/divisions exactly as frozen.
 */
static uint32_t effectiveness(const RmcPlayer *p, uint32_t attribute)
{
    return (UINT32_C(80) * attribute + UINT32_C(10) * p->energy +
            UINT32_C(10) * p->morale) / UINT32_C(100);
}

static bool model(
    const RmcPlayer *shooter, const RmcPlayer *defender,
    const RmcPlayer *keeper, RmcTacticalZone source, int32_t context,
    RmcShotResult *r)
{
    static const int32_t bases[3][RMC_LANE_COUNT] = {
        {10, 20, 10}, {30, 45, 30}, {55, 75, 55}
    };
    r->shooter_effectiveness = effectiveness(shooter,
        shooter->attributes[RMC_ATTR_TIRO]);
    r->positioning_effectiveness = effectiveness(shooter,
        shooter->attributes[RMC_ATTR_POSIZIONAMENTO]);
    const uint32_t d_base =
        (UINT32_C(40) * defender->attributes[RMC_ATTR_CONTRASTO] +
         UINT32_C(35) * defender->attributes[RMC_ATTR_MARCATURA] +
         UINT32_C(25) * defender->attributes[RMC_ATTR_POSIZIONAMENTO]) / 100u;
    r->defender_effectiveness = effectiveness(defender, d_base);
    const uint32_t g_base =
        (UINT32_C(65) * keeper->goalkeeper_attributes[RMC_GK_ATTR_RIFLESSI] +
         UINT32_C(35) * keeper->goalkeeper_attributes[RMC_GK_ATTR_POSIZIONAMENTO])
        / 100u;
    r->goalkeeper_effectiveness = effectiveness(keeper, g_base);
    r->holding_effectiveness = effectiveness(keeper,
        keeper->goalkeeper_attributes[RMC_GK_ATTR_PRESA]);

    const int32_t s = (int32_t)r->shooter_effectiveness;
    const int32_t j = (int32_t)r->positioning_effectiveness;
    const int32_t d = (int32_t)r->defender_effectiveness;
    const int32_t gk = (int32_t)r->goalkeeper_effectiveness;
    const int32_t h = (int32_t)r->holding_effectiveness;
    r->chance_quality = clamp(bases[source.sector - 2u][source.lane] +
                             j / 5 - d / 10 + context, 0, 100);
    const int32_t q = (int32_t)r->chance_quality;
    r->on_target_probability = clamp(2000 + 40 * s + 30 * q, 2000, 9000);
    r->woodwork_probability = 300;
    r->wide_probability = 9700u - r->on_target_probability;
    r->block_probability = clamp(500 + 25 * d - 10 * q - 5 * s, 200, 3000);
    if (!rmc_match_logistic_lookup((s + q) / 2 - gk,
                                   &r->base_goal_probability)) {
        return false;
    }
    const uint32_t composure = UINT32_C(1) << RMC_TALENT_COMPOSURE;
    r->applied_talent_mask =
        (shooter->talents.consolidated_mask & composure) != 0 &&
        source.sector == 4 && q >= 60 ? composure : 0;
    r->goal_probability = r->base_goal_probability;
    if (r->applied_talent_mask != 0) {
        r->goal_probability += r->goal_probability / 10u;
        if (r->goal_probability > 9999u) {
            r->goal_probability = 9999;
        }
    }
    r->hold_probability = clamp(2000 + 65 * h - 20 * q - 10 * s, 1000, 8500);
    return true;
}

static bool draw(RmcPcg32 *rng, RmcShotResult *r)
{
    return rmc_pcg32_bounded(rng, RMC_PROBABILITY_SCALE,
                            &r->draws[r->bounded_draw_count++]);
}

bool rmc_match_shot_apply(
    const RmcWorldCore *world, RmcMatchState *state, RmcPcg32 *match_rng,
    RmcMatchEventLog *log, const RmcShotAction *action, RmcShotResult *result)
{
    if (world == NULL || state == NULL || match_rng == NULL || log == NULL ||
        action == NULL || result == NULL ||
        !rmc_match_state_valid(world, state) ||
        state->mode != RMC_MATCH_BALL_IN_PLAY ||
        state->possession.team_id.value == 0 ||
        action->team_id.value != state->possession.team_id.value ||
        action->shooter.value == 0 ||
        action->shooter.value != state->possession.controller.value ||
        action->defender.value == action->shooter.value ||
        action->defender.value == action->goalkeeper.value ||
        state->possession.zone.sector < 2 || state->possession.zone.sector > 4 ||
        action->context < -20 || action->context > 20 ||
        action->duration_seconds == 0 ||
        (match_rng->increment & UINT64_C(1)) == 0 ||
        log->events == NULL || log->capacity > SIZE_MAX / sizeof(log->events[0]) ||
        log->count > log->capacity || log->count > UINT64_MAX ||
        log->match_id.value != state->fixture.id.value) {
        return false;
    }
    RmcSequenceDecision decision;
    if (!rmc_match_sequence_decide(world, state, &decision) ||
        decision != RMC_SEQUENCE_ALLOW_ACTION) {
        return false;
    }
    const bool home = action->team_id.value == state->fixture.home_team.value;
    const RmcTeamId opponent = home ? state->fixture.away_team :
                                           state->fixture.home_team;
    const RmcLineup *attack = home ? &state->home_lineup : &state->away_lineup;
    const RmcLineup *defence = home ? &state->away_lineup : &state->home_lineup;
    const RmcPlayer *shooter = field_player(world, action->team_id,
                                           attack, action->shooter, false);
    const RmcPlayer *defender = field_player(world, opponent,
                                            defence, action->defender, false);
    const RmcPlayer *keeper = field_player(world, opponent,
                                          defence, action->goalkeeper, true);
    if (shooter == NULL || defender == NULL || keeper == NULL ||
        (home ? state->home_goals : state->away_goals) == UINT16_MAX) {
        return false;
    }

    RmcMatchState timed = *state;
    if (!rmc_match_clock_advance(world, &timed, action->duration_seconds) ||
        !rmc_match_sequence_decide(world, &timed, &decision)) {
        return false;
    }
    const bool ending = decision == RMC_SEQUENCE_END_PERIOD;
    const size_t ending_count = !ending ? 0u :
        (state->period == RMC_MATCH_FIRST_HALF ? 1u : 2u);
    const size_t maximum = 3u + ending_count;
    if (maximum > log->capacity - log->count ||
        maximum > UINT64_MAX - log->count ||
        !prefix_valid(world, state, log)) {
        return false;
    }

    RmcShotResult resolved = {0};
    if (!model(shooter, defender, keeper, state->possession.zone,
               action->context, &resolved)) {
        return false;
    }
    RmcPcg32 next_rng = *match_rng;
    if (!draw(&next_rng, &resolved) || !draw(&next_rng, &resolved)) {
        return false;
    }
    resolved.trajectory = resolved.draws[0] < resolved.on_target_probability
        ? RMC_SHOT_TRAJECTORY_ON_TARGET
        : (resolved.draws[0] < resolved.on_target_probability +
                              resolved.woodwork_probability
           ? RMC_SHOT_TRAJECTORY_WOODWORK : RMC_SHOT_TRAJECTORY_WIDE);
    if (resolved.draws[1] < resolved.block_probability) {
        resolved.outcome = RMC_SHOT_BLOCKED;
    } else if (resolved.trajectory == RMC_SHOT_TRAJECTORY_WIDE) {
        resolved.outcome = RMC_SHOT_WIDE;
    } else if (resolved.trajectory == RMC_SHOT_TRAJECTORY_WOODWORK) {
        resolved.outcome = RMC_SHOT_WOODWORK;
    } else {
        if (!draw(&next_rng, &resolved)) {
            return false;
        }
        if (resolved.draws[2] < resolved.goal_probability) {
            resolved.outcome = RMC_SHOT_GOAL;
        } else {
            if (!draw(&next_rng, &resolved)) {
                return false;
            }
            resolved.outcome = resolved.draws[3] < resolved.hold_probability
                ? RMC_SHOT_SAVED_HELD : RMC_SHOT_SAVED_REBOUND;
        }
    }

    RmcMatchState next = timed;
    const bool goal = resolved.outcome == RMC_SHOT_GOAL;
    const bool restart = goal || resolved.outcome == RMC_SHOT_WIDE;
    if (goal) {
        if (home) {
            ++next.home_goals;
        } else {
            ++next.away_goals;
        }
    }
    if (restart) {
        next.possession = (RmcMatchPossession){0};
        /* Keep BALL_IN_PLAY until the closure planner has run. */
    } else if (resolved.outcome == RMC_SHOT_SAVED_HELD) {
        next.possession = (RmcMatchPossession){0};
        next.possession.team_id = opponent;
        next.possession.initiator = action->goalkeeper;
        next.possession.controller = action->goalkeeper;
        next.possession.zone.lane = RMC_LANE_CENTRE;
        next.possession.phase = RMC_PHASE_TRANSITION;
    } else {
        next.possession.controller = (RmcPlayerId){0};
        ++next.possession.action_count;
        if (resolved.outcome != RMC_SHOT_BLOCKED) {
            next.possession.zone = (RmcTacticalZone){4, RMC_LANE_CENTRE};
            next.possession.phase = RMC_PHASE_CHANCE;
        }
    }

    RmcMatchEvent events[5] = {{0}};
    events[0].id = (uint64_t)log->count + UINT64_C(1);
    events[0].match_id = state->fixture.id;
    events[0].at_second = timed.elapsed_seconds;
    events[0].type = RMC_EVENT_ACTION;
    events[0].team_id = action->team_id;
    events[0].primary_actor = action->shooter;
    events[0].secondary_actor = action->goalkeeper;
    events[0].outcome_code = RMC_SHOT_CHANCE;
    events[0].shot = (RmcShotEventPayload){
        .model_version = RMC_SHOT_MODEL_VERSION,
        .chance_quality = resolved.chance_quality,
        .source_zone = state->possession.zone,
        .context = action->context,
        .duration_seconds = action->duration_seconds,
        .defender_id = action->defender,
        .goalkeeper_id = action->goalkeeper,
        .applied_talent_mask = resolved.applied_talent_mask
    };
    events[1] = events[0];
    ++events[1].id;
    events[1].type = RMC_EVENT_SHOT;
    events[1].outcome_code = resolved.outcome;
    events[1].cause_event_id = events[0].id;
    events[1].shot.trajectory = resolved.trajectory;
    if (restart) {
        events[1].shot.restart_kind = goal ? RMC_RESTART_KICKOFF :
                                                   RMC_RESTART_GOAL_KICK;
        events[1].shot.restart_awarded_team = opponent;
        events[1].shot.restart_zone =
            (RmcTacticalZone){goal ? 2 : 0, RMC_LANE_CENTRE};
        events[1].shot.restart_disposition = ending ?
            RMC_RESTART_SUPPRESSED_PERIOD_END : RMC_RESTART_PENDING;
    }
    size_t count = 2;
    if (goal) {
        events[2] = events[1];
        ++events[2].id;
        events[2].type = RMC_EVENT_GOAL;
        events[2].cause_event_id = events[1].id;
        ++count;
    }
    if (ending) {
        RmcMatchPeriodPlan plan;
        if (!rmc_match_period_plan(world, &next, log->count + count, &plan)) {
            return false;
        }
        for (size_t i = 0; i < plan.event_count; ++i) {
            events[count++] = plan.events[i];
        }
        next = plan.next_state;
    } else if (restart) {
        next.mode = RMC_MATCH_RESTART_PENDING;
    }
    if (!rmc_match_state_valid(world, &next)) {
        return false;
    }

    /* Validate against the active timed period, even when next is terminal.
     * This is the first caller-owned write; everything following is infallible.
     */
    if (!rmc_match_event_log_append_batch(world, &timed, log, events, count)) {
        return false;
    }
    *state = next;
    *match_rng = next_rng;
    *result = resolved;
    return true;
}
