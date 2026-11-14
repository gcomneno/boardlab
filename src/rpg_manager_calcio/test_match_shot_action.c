#include "match_shot_action.h"
#include "match_clock.h"
#include "match_period_commit.h"
#include "match_transitions.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

_Static_assert(RMC_SHOT_ACTION_VERSION == 1, "Action version");
_Static_assert(RMC_SHOT_MODEL_VERSION == 1, "Model version");
_Static_assert(RMC_EVENT_SCHEMA_VERSION == 2, "Event version");
_Static_assert(RMC_SHOT_CHANCE == 500 && RMC_SHOT_GOAL == 501 &&
               RMC_SHOT_SAVED_HELD == 502 && RMC_SHOT_SAVED_REBOUND == 503 &&
               RMC_SHOT_BLOCKED == 504 && RMC_SHOT_WIDE == 505 &&
               RMC_SHOT_WOODWORK == 506, "Frozen outcomes");
_Static_assert(RMC_RULES_VERSION == 1 && RMC_GENERATOR_VERSION == 1 &&
               RMC_LOGISTIC_TABLE_VERSION == 1, "Frozen infrastructure");

enum { STORAGE = 32 };
static RmcWorldCore world;
static RmcWorldCore original_world;
static RmcMatchState initial;

typedef struct {
    RmcMatchState state;
    RmcPcg32 rng;
    RmcMatchEvent storage[STORAGE];
    RmcMatchEventLog log;
    RmcShotResult result;
} Run;

static void setup(void)
{
    const RmcWorldIdentity identity = {
        .world_seed = UINT64_C(20261114),
        .generator_version = RMC_GENERATOR_VERSION,
        .rules_version = RMC_RULES_VERSION
    };
    assert(rmc_world_core_generate(&identity, &world));
    const RmcFixture fixture = {
        .id = {1}, .home_team = world.teams[0].id,
        .away_team = world.teams[1].id, .matchday = 1
    };
    RmcLineup home, away;
    assert(rmc_roster_lineup(&world.rosters[0], RMC_FORMATION_442, &home));
    assert(rmc_roster_lineup(&world.rosters[1], RMC_FORMATION_433, &away));
    assert(rmc_match_state_init(&world, &fixture, &home, &away, &initial));
    original_world = world;
}

static RmcPcg32 stream(void)
{
    RmcPcg32 rng;
    assert(rmc_stream_init(&rng, &original_world.identity, RMC_STREAM_MATCH,
                           initial.fixture.id.value));
    return rng;
}

static RmcPlayer *player(RmcPlayerId id)
{
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        for (size_t j = 0; j < RMC_TEAM_PLAYERS; ++j) {
            if (world.rosters[i].players[j].id.value == id.value) {
                return &world.rosters[i].players[j];
            }
        }
    }
    assert(false);
    return NULL;
}

static void possession(RmcMatchState *state, bool home)
{
    const RmcLineup *lineup = home ? &state->home_lineup : &state->away_lineup;
    state->mode = RMC_MATCH_BALL_IN_PLAY;
    state->possession = (RmcMatchPossession){0};
    state->possession.team_id = home ? state->fixture.home_team :
                                           state->fixture.away_team;
    state->possession.initiator = lineup->players[2];
    state->possession.controller = lineup->players[1];
    state->possession.zone = (RmcTacticalZone){3, RMC_LANE_CENTRE};
    state->possession.phase = RMC_PHASE_ATTACKING_DEVELOPMENT;
    state->possession.action_count = 7;
    state->possession.elapsed_seconds = 20;
}

static RmcMatchState active(bool home)
{
    RmcMatchState state = initial;
    assert(rmc_match_transition_apply(&world, &state, RMC_TRANSITION_START, 0));
    state.mode = RMC_MATCH_BALL_IN_PLAY;
    assert(rmc_match_clock_advance(&world, &state, 100));
    possession(&state, home);
    state.home_goals = 2;
    state.away_goals = 3;
    assert(rmc_match_state_valid(&world, &state));
    return state;
}

static RmcShotAction action_for(const RmcMatchState *state)
{
    const bool home = state->possession.team_id.value ==
                      state->fixture.home_team.value;
    const RmcLineup *opponent = home ? &state->away_lineup : &state->home_lineup;
    const RmcShotAction action = {
        .team_id = state->possession.team_id,
        .shooter = state->possession.controller,
        .defender = opponent->players[1],
        .goalkeeper = opponent->players[0],
        .context = 0, .duration_seconds = 5
    };
    return action;
}

static void init_run(Run *run, const RmcMatchState *state, RmcPcg32 rng)
{
    memset(run, 0xa5, sizeof(*run));
    run->state = *state;
    run->rng = rng;
    assert(rmc_match_event_log_init(&run->state, run->storage, STORAGE, &run->log));
}

static bool apply(Run *run, const RmcShotAction *action)
{
    return rmc_match_shot_apply(&world, &run->state, &run->rng, &run->log,
                               action, &run->result);
}

static void reject(Run *run, const RmcShotAction *action)
{
    /* Capture object representations with memcpy: this check includes padding
     * and every unused storage slot, without assuming portable serialization.
     */
    unsigned char before[sizeof(*run)];
    memcpy(before, run, sizeof(*run));
    assert(!apply(run, action));
    assert(memcmp(before, run, sizeof(*run)) == 0);
}

/* Independent branch oracle uses the actual bounded PCG32 API, never a
 * fixed raw-output count or a modulo-only substitute for rejection sampling.
 */
static RmcShotEventOutcome predict(
    const RmcShotResult *model, RmcPcg32 *rng, uint32_t draws[4],
    uint32_t *count, RmcShotTrajectory *trajectory)
{
    memset(draws, 0, 4 * sizeof(draws[0]));
    assert(rmc_pcg32_bounded(rng, 10000, &draws[0]));
    assert(rmc_pcg32_bounded(rng, 10000, &draws[1]));
    *count = 2;
    if (draws[0] >= model->on_target_probability + model->woodwork_probability) {
        *trajectory = RMC_SHOT_TRAJECTORY_WIDE;
    } else if (draws[0] >= model->on_target_probability) {
        *trajectory = RMC_SHOT_TRAJECTORY_WOODWORK;
    } else {
        *trajectory = RMC_SHOT_TRAJECTORY_ON_TARGET;
    }
    if (draws[1] < model->block_probability) {
        return RMC_SHOT_BLOCKED;
    }
    if (*trajectory == RMC_SHOT_TRAJECTORY_WIDE) {
        return RMC_SHOT_WIDE;
    }
    if (*trajectory == RMC_SHOT_TRAJECTORY_WOODWORK) {
        return RMC_SHOT_WOODWORK;
    }
    assert(rmc_pcg32_bounded(rng, 10000, &draws[2]));
    *count = 3;
    if (draws[2] < model->goal_probability) {
        return RMC_SHOT_GOAL;
    }
    assert(rmc_pcg32_bounded(rng, 10000, &draws[3]));
    *count = 4;
    return draws[3] < model->hold_probability ? RMC_SHOT_SAVED_HELD :
                                               RMC_SHOT_SAVED_REBOUND;
}

static void normalized(const RmcShotResult *r)
{
    const uint64_t n = 10000;
    assert(r->shooter_effectiveness <= 100 && r->positioning_effectiveness <= 100);
    assert(r->defender_effectiveness <= 100 && r->goalkeeper_effectiveness <= 100);
    assert(r->holding_effectiveness <= 100 && r->chance_quality <= 100);
    assert(r->on_target_probability >= 2000 && r->on_target_probability <= 9000);
    assert(r->woodwork_probability == 300);
    assert(r->on_target_probability + r->woodwork_probability +
           r->wide_probability == n);
    assert(r->block_probability >= 200 && r->block_probability <= 3000);
    assert(r->goal_probability <= 9999);
    assert(r->hold_probability >= 1000 && r->hold_probability <= 8500);
    assert(r->block_probability + (n - r->block_probability) == n);
    assert(r->goal_probability + (n - r->goal_probability) == n);
    assert(r->hold_probability + (n - r->hold_probability) == n);
    const uint64_t t = r->on_target_probability, b = r->block_probability;
    const uint64_t g = r->goal_probability, h = r->hold_probability;
    /* Exact unconditional six-outcome numerator over N^4, no rounding. */
    const uint64_t sum = b * n * n * n +
        (uint64_t)r->wide_probability * (n - b) * n * n +
        (uint64_t)r->woodwork_probability * (n - b) * n * n +
        t * (n - b) * g * n + t * (n - b) * (n - g) * h +
        t * (n - b) * (n - g) * (n - h);
    assert(sum == n * n * n * n);
}

static void same_payload(const RmcShotEventPayload *a, const RmcShotEventPayload *b)
{
    assert(a->model_version == b->model_version);
    assert(a->chance_quality == b->chance_quality);
    assert(a->source_zone.sector == b->source_zone.sector);
    assert(a->source_zone.lane == b->source_zone.lane);
    assert(a->context == b->context && a->duration_seconds == b->duration_seconds);
    assert(a->defender_id.value == b->defender_id.value);
    assert(a->goalkeeper_id.value == b->goalkeeper_id.value);
    assert(a->applied_talent_mask == b->applied_talent_mask);
}

static void same_event(const RmcMatchEvent *a, const RmcMatchEvent *b)
{
    assert(a->id == b->id && a->match_id.value == b->match_id.value);
    assert(a->at_second == b->at_second && a->type == b->type);
    assert(a->team_id.value == b->team_id.value);
    assert(a->primary_actor.value == b->primary_actor.value);
    assert(a->secondary_actor.value == b->secondary_actor.value);
    assert(a->outcome_code == b->outcome_code && a->cause_event_id == b->cause_event_id);
    same_payload(&a->shot, &b->shot);
    assert(a->shot.trajectory == b->shot.trajectory);
    assert(a->shot.restart_kind == b->shot.restart_kind);
    assert(a->shot.restart_awarded_team.value == b->shot.restart_awarded_team.value);
    assert(a->shot.restart_zone.sector == b->shot.restart_zone.sector);
    assert(a->shot.restart_zone.lane == b->shot.restart_zone.lane);
    assert(a->shot.restart_disposition == b->shot.restart_disposition);
}

static void same_state(const RmcMatchState *a, const RmcMatchState *b)
{
    assert(a->fixture.id.value == b->fixture.id.value);
    assert(a->period == b->period && a->mode == b->mode);
    assert(a->terminal_reason == b->terminal_reason);
    assert(a->elapsed_seconds == b->elapsed_seconds);
    assert(a->second_half_start_second == b->second_half_start_second);
    assert(a->first_half_stoppage_seconds == b->first_half_stoppage_seconds);
    assert(a->second_half_stoppage_seconds == b->second_half_stoppage_seconds);
    assert(a->home_goals == b->home_goals && a->away_goals == b->away_goals);
    assert(a->possession.team_id.value == b->possession.team_id.value);
    assert(a->possession.initiator.value == b->possession.initiator.value);
    assert(a->possession.controller.value == b->possession.controller.value);
    assert(a->possession.zone.sector == b->possession.zone.sector);
    assert(a->possession.zone.lane == b->possession.zone.lane);
    assert(a->possession.phase == b->possession.phase);
    assert(a->possession.action_count == b->possession.action_count);
    assert(a->possession.elapsed_seconds == b->possession.elapsed_seconds);
    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        assert(a->home_lineup.players[i].value == b->home_lineup.players[i].value);
        assert(a->away_lineup.players[i].value == b->away_lineup.players[i].value);
        assert(a->home_lineup.slots[i] == b->home_lineup.slots[i]);
        assert(a->away_lineup.slots[i] == b->away_lineup.slots[i]);
    }
}

/* Checks a genuine production resolution against the branch oracle, complete
 * sporting effects and payload/causal chain, then replays it with the same seed.
 */
static RmcShotResult check(
    const RmcMatchState *before, const RmcShotAction *action, RmcPcg32 seed)
{
    Run run, replay;
    init_run(&run, before, seed);
    init_run(&replay, before, seed);
    assert(apply(&run, action));
    const RmcShotResult *r = &run.result;
    normalized(r);
    uint32_t draws[4], count;
    RmcShotTrajectory trajectory;
    RmcPcg32 expected = seed;
    const RmcShotEventOutcome outcome = predict(r, &expected, draws, &count, &trajectory);
    assert(r->outcome == outcome && r->trajectory == trajectory);
    assert(r->bounded_draw_count == count);
    for (size_t i = 0; i < 4; ++i) {
        assert(r->draws[i] == draws[i]);
    }
    assert(run.rng.state == expected.state && run.rng.increment == expected.increment);
    assert(run.state.elapsed_seconds == before->elapsed_seconds + action->duration_seconds);
    assert(rmc_match_state_valid(&world, &run.state));

    const bool home = action->team_id.value == before->fixture.home_team.value;
    const RmcTeamId opponent = home ? before->fixture.away_team : before->fixture.home_team;
    const bool goal = outcome == RMC_SHOT_GOAL;
    const bool restart = goal || outcome == RMC_SHOT_WIDE;
    assert(run.state.home_goals == before->home_goals + (goal && home ? 1u : 0u));
    assert(run.state.away_goals == before->away_goals + (goal && !home ? 1u : 0u));
    const uint64_t deadline = before->period == RMC_MATCH_FIRST_HALF
        ? UINT64_C(2700) + before->first_half_stoppage_seconds
        : (uint64_t)before->second_half_start_second + 2700u +
          before->second_half_stoppage_seconds;
    const bool ending = run.state.elapsed_seconds == deadline;
    const size_t chain = goal ? 3u : 2u;
    const size_t ending_count = !ending ? 0u :
        (before->period == RMC_MATCH_FIRST_HALF ? 1u : 2u);
    assert(run.log.count == chain + ending_count);
    const RmcMatchEvent *chance = &run.storage[0];
    const RmcMatchEvent *shot = &run.storage[1];
    assert(chance->type == RMC_EVENT_ACTION && chance->outcome_code == RMC_SHOT_CHANCE);
    assert(chance->cause_event_id == 0);
    assert(chance->shot.trajectory == RMC_SHOT_TRAJECTORY_NONE);
    assert(chance->shot.restart_kind == RMC_RESTART_NONE);
    assert(chance->shot.restart_awarded_team.value == 0);
    assert(chance->shot.restart_zone.sector == 0 && chance->shot.restart_zone.lane == RMC_LANE_LEFT);
    assert(chance->shot.restart_disposition == RMC_RESTART_DISPOSITION_NONE);
    assert(shot->type == RMC_EVENT_SHOT && shot->outcome_code == (uint32_t)outcome);
    assert(shot->cause_event_id == chance->id && shot->shot.trajectory == trajectory);
    for (size_t i = 0; i < chain; ++i) {
        const RmcMatchEvent *event = &run.storage[i];
        assert(event->id == i + 1u && event->match_id.value == before->fixture.id.value);
        assert(event->at_second == run.state.elapsed_seconds);
        assert(event->team_id.value == action->team_id.value);
        assert(event->primary_actor.value == action->shooter.value);
        assert(event->secondary_actor.value == action->goalkeeper.value);
        assert(event->shot.model_version == 1 && event->shot.chance_quality == r->chance_quality);
        assert(event->shot.source_zone.sector == before->possession.zone.sector);
        assert(event->shot.source_zone.lane == before->possession.zone.lane);
        assert(event->shot.context == action->context);
        assert(event->shot.duration_seconds == action->duration_seconds);
        assert(event->shot.defender_id.value == action->defender.value);
        assert(event->shot.goalkeeper_id.value == action->goalkeeper.value);
        assert(event->shot.applied_talent_mask == r->applied_talent_mask);
        same_payload(&event->shot, &chance->shot);
    }
    if (goal) {
        assert(run.storage[2].type == RMC_EVENT_GOAL);
        assert(run.storage[2].outcome_code == RMC_SHOT_GOAL);
        assert(run.storage[2].cause_event_id == shot->id);
        assert(run.storage[2].shot.trajectory == shot->shot.trajectory);
        assert(run.storage[2].shot.restart_kind == shot->shot.restart_kind);
        assert(run.storage[2].shot.restart_disposition == shot->shot.restart_disposition);
    }
    if (restart) {
        assert(shot->shot.restart_kind == (goal ? RMC_RESTART_KICKOFF : RMC_RESTART_GOAL_KICK));
        assert(shot->shot.restart_awarded_team.value == opponent.value);
        assert(shot->shot.restart_zone.sector == (goal ? 2 : 0));
        assert(shot->shot.restart_zone.lane == RMC_LANE_CENTRE);
        assert(shot->shot.restart_disposition ==
               (ending ? RMC_RESTART_SUPPRESSED_PERIOD_END : RMC_RESTART_PENDING));
    } else {
        assert(shot->shot.restart_kind == RMC_RESTART_NONE);
        assert(shot->shot.restart_awarded_team.value == 0);
        assert(shot->shot.restart_zone.sector == 0 && shot->shot.restart_zone.lane == RMC_LANE_LEFT);
        assert(shot->shot.restart_disposition == RMC_RESTART_DISPOSITION_NONE);
    }

    if (ending || restart) {
        const RmcMatchPossession *pos = &run.state.possession;
        assert(pos->team_id.value == 0 && pos->initiator.value == 0 && pos->controller.value == 0);
        assert(pos->action_count == 0 && pos->elapsed_seconds == 0);
        assert(pos->zone.sector == 0 && pos->zone.lane == RMC_LANE_LEFT);
        assert(pos->phase == RMC_PHASE_BUILDUP);
        assert(run.state.mode == (ending ? RMC_MATCH_PERIOD_ENDED : RMC_MATCH_RESTART_PENDING));
    } else if (outcome == RMC_SHOT_SAVED_HELD) {
        assert(run.state.mode == RMC_MATCH_BALL_IN_PLAY);
        assert(run.state.possession.team_id.value == opponent.value);
        assert(run.state.possession.initiator.value == action->goalkeeper.value);
        assert(run.state.possession.controller.value == action->goalkeeper.value);
        assert(run.state.possession.zone.sector == 0 && run.state.possession.zone.lane == RMC_LANE_CENTRE);
        assert(run.state.possession.phase == RMC_PHASE_TRANSITION);
        assert(run.state.possession.action_count == 0 && run.state.possession.elapsed_seconds == 0);
    } else {
        assert(run.state.mode == RMC_MATCH_BALL_IN_PLAY);
        assert(run.state.possession.team_id.value == action->team_id.value);
        assert(run.state.possession.initiator.value == before->possession.initiator.value);
        assert(run.state.possession.controller.value == 0);
        assert(run.state.possession.action_count == before->possession.action_count + 1u);
        assert(run.state.possession.elapsed_seconds == before->possession.elapsed_seconds + action->duration_seconds);
        assert(run.state.possession.zone.sector == (outcome == RMC_SHOT_BLOCKED ? before->possession.zone.sector : 4));
        assert(run.state.possession.zone.lane == (outcome == RMC_SHOT_BLOCKED ? before->possession.zone.lane : RMC_LANE_CENTRE));
        assert(run.state.possession.phase == (outcome == RMC_SHOT_BLOCKED ? before->possession.phase : RMC_PHASE_CHANCE));
    }
    if (ending) {
        assert(run.storage[chain].type == RMC_EVENT_PERIOD_END);
        assert(run.storage[chain].id == chain + 1u && run.storage[chain].cause_event_id == 0);
        assert(run.storage[chain].shot.model_version == 0);
        if (before->period == RMC_MATCH_FIRST_HALF) {
            assert(run.state.period == RMC_MATCH_HALF_TIME);
            assert(run.state.second_half_start_second == deadline);
        } else {
            assert(run.state.period == RMC_MATCH_FINISHED);
            assert(run.state.terminal_reason == RMC_MATCH_TERMINAL_COMPLETED);
            assert(run.storage[chain + 1u].type == RMC_EVENT_MATCH_END);
            assert(run.storage[chain + 1u].cause_event_id == run.storage[chain].id);
            assert(run.storage[chain + 1u].shot.model_version == 0);
        }
        for (size_t i = chain; i < run.log.count; ++i) {
            assert(run.storage[i].at_second == deadline);
        }
    }
    /* No implicit restart, recovery or second scoring is permitted. */
    reject(&run, action);
    assert(apply(&replay, action));
    same_state(&run.state, &replay.state);
    assert(run.rng.state == replay.rng.state && run.rng.increment == replay.rng.increment);
    assert(run.log.count == replay.log.count);
    for (size_t i = 0; i < run.log.count; ++i) {
        same_event(&run.storage[i], &replay.storage[i]);
    }
    /* Result consists solely of integer/enum fields; compare each field. */
    assert(r->outcome == replay.result.outcome && r->trajectory == replay.result.trajectory);
    assert(r->shooter_effectiveness == replay.result.shooter_effectiveness);
    assert(r->positioning_effectiveness == replay.result.positioning_effectiveness);
    assert(r->defender_effectiveness == replay.result.defender_effectiveness);
    assert(r->goalkeeper_effectiveness == replay.result.goalkeeper_effectiveness);
    assert(r->holding_effectiveness == replay.result.holding_effectiveness);
    assert(r->chance_quality == replay.result.chance_quality);
    assert(r->on_target_probability == replay.result.on_target_probability);
    assert(r->woodwork_probability == replay.result.woodwork_probability);
    assert(r->wide_probability == replay.result.wide_probability);
    assert(r->block_probability == replay.result.block_probability);
    assert(r->base_goal_probability == replay.result.base_goal_probability);
    assert(r->goal_probability == replay.result.goal_probability);
    assert(r->hold_probability == replay.result.hold_probability);
    assert(r->applied_talent_mask == replay.result.applied_talent_mask);
    assert(r->bounded_draw_count == replay.result.bounded_draw_count);
    for (size_t i = 0; i < 4; ++i) {
        assert(r->draws[i] == replay.result.draws[i]);
    }
    return run.result;
}

static RmcPcg32 seed_for(
    const RmcMatchState *state, const RmcShotAction *action,
    RmcShotEventOutcome desired, RmcShotTrajectory latent)
{
    Run sample;
    RmcPcg32 seed = stream();
    init_run(&sample, state, seed);
    assert(apply(&sample, action));
    for (unsigned i = 0; i < 1000000; ++i) {
        RmcPcg32 expected = seed;
        uint32_t draws[4], count;
        RmcShotTrajectory trajectory;
        if (predict(&sample.result, &expected, draws, &count, &trajectory) == desired &&
            (latent == RMC_SHOT_TRAJECTORY_NONE || trajectory == latent)) {
            return seed;
        }
        (void)rmc_pcg32_next(&seed);
    }
    assert(false);
    return seed;
}

static void set_values(RmcPlayerId id, uint8_t attribute, uint8_t energy, uint8_t morale)
{
    RmcPlayer *p = player(id);
    memset(p->attributes, attribute, sizeof(p->attributes));
    if (p->department == RMC_DEPARTMENT_GOALKEEPER) {
        memset(p->goalkeeper_attributes, attribute, sizeof(p->goalkeeper_attributes));
    }
    p->energy = energy;
    p->morale = morale;
    p->talents = (RmcTalentState){0};
}

static void test_formulas(void)
{
    RmcMatchState state = active(true);
    RmcShotAction action = action_for(&state);
    RmcPlayer *s = player(action.shooter), *d = player(action.defender);
    RmcPlayer *k = player(action.goalkeeper);
    s->attributes[RMC_ATTR_TIRO] = 73;
    s->attributes[RMC_ATTR_POSIZIONAMENTO] = 61;
    s->energy = 47; s->morale = 83;
    d->attributes[RMC_ATTR_CONTRASTO] = 57;
    d->attributes[RMC_ATTR_MARCATURA] = 68;
    d->attributes[RMC_ATTR_POSIZIONAMENTO] = 79;
    d->energy = 43; d->morale = 98;
    k->goalkeeper_attributes[RMC_GK_ATTR_RIFLESSI] = 81;
    k->goalkeeper_attributes[RMC_GK_ATTR_POSIZIONAMENTO] = 26;
    k->goalkeeper_attributes[RMC_GK_ATTR_PRESA] = 49;
    k->energy = 55; k->morale = 94;
    action.context = -7;
    RmcShotResult r = check(&state, &action, stream());
    /* Fixed vector discriminates staged D/G floors and floor((S+Q)/2). */
    assert(r.shooter_effectiveness == 71 && r.positioning_effectiveness == 61);
    assert(r.defender_effectiveness == 66 && r.goalkeeper_effectiveness == 63);
    assert(r.holding_effectiveness == 54 && r.chance_quality == 44);
    assert(r.on_target_probability == 6160 && r.wide_probability == 3540);
    assert(r.block_probability == 1355 && r.base_goal_probability == 4073);
    assert(r.goal_probability == 4073 && r.hold_probability == 3920);

    /* The qualified roster domain is [1,100]. At attribute=1, E can still be
     * zero; no test bypasses production validation to fabricate this bound.
     */
    set_values(action.shooter, 1, 0, 0);
    set_values(action.defender, 100, 100, 100);
    set_values(action.goalkeeper, 100, 100, 100);
    state.possession.zone = (RmcTacticalZone){2, RMC_LANE_LEFT};
    action.context = -20;
    r = check(&state, &action, stream());
    assert(r.shooter_effectiveness == 0 && r.positioning_effectiveness == 0);
    assert(r.defender_effectiveness == 100 && r.goalkeeper_effectiveness == 100);
    assert(r.holding_effectiveness == 100 && r.chance_quality == 0);
    assert(r.on_target_probability == 2000 && r.wide_probability == 7700);
    assert(r.block_probability == 3000 && r.base_goal_probability == 19);
    assert(r.hold_probability == 8500);
    set_values(action.shooter, 100, 100, 100);
    set_values(action.defender, 1, 0, 0);
    set_values(action.goalkeeper, 1, 0, 0);
    state.possession.zone = (RmcTacticalZone){4, RMC_LANE_CENTRE};
    action.context = 20;
    r = check(&state, &action, stream());
    assert(r.shooter_effectiveness == 100 && r.positioning_effectiveness == 100);
    assert(r.defender_effectiveness == 0 && r.goalkeeper_effectiveness == 0);
    assert(r.holding_effectiveness == 0 && r.chance_quality == 100);
    assert(r.on_target_probability == 9000 && r.wide_probability == 700);
    assert(r.block_probability == 200 && r.goal_probability == 9981);
    assert(r.hold_probability == 1000);
    world = original_world;
}

static void test_zones_and_context(void)
{
    static const uint32_t bases[3][3] = {{10,20,10}, {30,45,30}, {55,75,55}};
    RmcMatchState state = active(true);
    RmcShotAction action = action_for(&state);
    set_values(action.shooter, 50, 50, 50);
    set_values(action.defender, 50, 50, 50);
    set_values(action.goalkeeper, 50, 50, 50);
    for (uint8_t sector = 2; sector <= 4; ++sector) {
        for (unsigned lane = 0; lane < RMC_LANE_COUNT; ++lane) {
            state.possession.zone = (RmcTacticalZone){sector, (RmcTacticalLane)lane};
            for (int32_t context = -20; context <= 20; context += 20) {
                action.context = context;
                const RmcShotResult r = check(&state, &action, stream());
                int32_t expected = (int32_t)bases[sector - 2][lane] + 5 + context;
                if (expected < 0) { expected = 0; }
                if (expected > 100) { expected = 100; }
                assert(r.chance_quality == (uint32_t)expected);
                assert(r.on_target_probability == 4000u + 30u * r.chance_quality);
            }
        }
    }
    world = original_world;
}

static void test_composure(void)
{
    RmcMatchState state = active(true);
    RmcShotAction action = action_for(&state);
    set_values(action.shooter, 50, 50, 50);
    set_values(action.defender, 50, 50, 50);
    set_values(action.goalkeeper, 50, 50, 50);
    RmcPlayer *s = player(action.shooter);
    const uint32_t mask = UINT32_C(1) << RMC_TALENT_COMPOSURE;
    state.possession.zone = (RmcTacticalZone){4, RMC_LANE_LEFT};
    s->talents.matured_mask = (uint8_t)mask;
    s->talents.candidate_progress[RMC_TALENT_COMPOSURE] = 1000;
    RmcShotResult r = check(&state, &action, stream());
    assert(r.chance_quality == 60 && r.applied_talent_mask == 0);
    assert(r.goal_probability == r.base_goal_probability);
    s->talents.consolidated_mask = (uint8_t)mask;
    r = check(&state, &action, stream());
    assert(r.applied_talent_mask == mask);
    assert(r.base_goal_probability == 5775 && r.goal_probability == 6352);
    action.context = -1;
    r = check(&state, &action, stream());
    assert(r.chance_quality == 59 && r.applied_talent_mask == 0);
    action.context = 20;
    state.possession.zone = (RmcTacticalZone){3, RMC_LANE_CENTRE};
    r = check(&state, &action, stream());
    assert(r.chance_quality == 70 && r.applied_talent_mask == 0);
    s->talents.consolidated_mask = UINT8_C(1) << RMC_TALENT_INSPIRED_PASS;
    state.possession.zone.sector = 4;
    r = check(&state, &action, stream());
    assert(r.applied_talent_mask == 0);
    set_values(action.shooter, 100, 100, 100);
    set_values(action.defender, 1, 0, 0);
    set_values(action.goalkeeper, 1, 0, 0);
    s->talents.consolidated_mask = (uint8_t)mask;
    r = check(&state, &action, stream());
    assert(r.base_goal_probability == 9981 && r.goal_probability == 9999);
    world = original_world;
}

static void test_outcomes_and_draws(void)
{
    for (unsigned home = 0; home < 2; ++home) {
        RmcMatchState state = active(home != 0);
        RmcShotAction action = action_for(&state);
        for (unsigned code = RMC_SHOT_GOAL; code <= RMC_SHOT_WOODWORK; ++code) {
            RmcPcg32 seed = seed_for(&state, &action, (RmcShotEventOutcome)code,
                                    RMC_SHOT_TRAJECTORY_NONE);
            RmcShotResult r = check(&state, &action, seed);
            assert((unsigned)r.outcome == code);
            assert(r.bounded_draw_count == (code == RMC_SHOT_GOAL ? 3u :
                   (code == RMC_SHOT_SAVED_HELD || code == RMC_SHOT_SAVED_REBOUND ? 4u : 2u)));
        }
        /* Defensive opposition is sampled even for latent WIDE/WOODWORK. */
        for (unsigned t = RMC_SHOT_TRAJECTORY_ON_TARGET; t <= RMC_SHOT_TRAJECTORY_WOODWORK; ++t) {
            RmcPcg32 seed = seed_for(&state, &action, RMC_SHOT_BLOCKED, (RmcShotTrajectory)t);
            RmcShotResult r = check(&state, &action, seed);
            assert(r.outcome == RMC_SHOT_BLOCKED && (unsigned)r.trajectory == t);
        }
        /* A valid odd increment with state zero guarantees a rejected raw
         * output (zero < 7296). The successful bounded draw count stays 2..4.
         */
        RmcPcg32 seed = stream();
        seed.state = 0;
        RmcShotResult r = check(&state, &action, seed);
        RmcPcg32 raw = seed;
        for (uint32_t i = 0; i < r.bounded_draw_count; ++i) {
            (void)rmc_pcg32_next(&raw);
        }
        RmcPcg32 bounded = seed;
        for (uint32_t i = 0; i < r.bounded_draw_count; ++i) {
            uint32_t value;
            assert(rmc_pcg32_bounded(&bounded, 10000, &value));
        }
        assert(raw.state != bounded.state);
    }
}

static void test_interval_edges(void)
{
    RmcMatchState state = active(true);
    RmcShotAction action = action_for(&state);
    Run sample;
    init_run(&sample, &state, stream());
    assert(apply(&sample, &action));
    const RmcShotResult *m = &sample.result;
    /* Search actual PCG32 states for both sides of every half-open boundary.
     * Only successful production calls can satisfy an edge assertion.
     */
    const uint32_t target[10] = {
        m->on_target_probability - 1u, m->on_target_probability,
        m->on_target_probability + 299u, m->on_target_probability + 300u,
        m->block_probability - 1u, m->block_probability,
        m->goal_probability - 1u, m->goal_probability,
        m->hold_probability - 1u, m->hold_probability
    };
    bool found[10] = {false};
    unsigned remaining = 10;
    RmcPcg32 seed = stream();
    for (unsigned attempt = 0; attempt < 2000000 && remaining != 0; ++attempt) {
        RmcPcg32 expected = seed;
        uint32_t draws[4], count;
        RmcShotTrajectory trajectory;
        const RmcShotEventOutcome outcome = predict(m, &expected, draws, &count, &trajectory);
        for (unsigned edge = 0; edge < 10; ++edge) {
            const unsigned stage = edge < 4 ? 0u : (edge < 6 ? 1u : (edge < 8 ? 2u : 3u));
            if (!found[edge] && count > stage && draws[stage] == target[edge] &&
                (edge >= 4 || outcome != RMC_SHOT_BLOCKED)) {
                const RmcShotResult r = check(&state, &action, seed);
                assert(r.draws[stage] == target[edge]);
                if (edge < 4) {
                    assert(r.trajectory == (edge == 0 ? RMC_SHOT_TRAJECTORY_ON_TARGET :
                        (edge == 3 ? RMC_SHOT_TRAJECTORY_WIDE : RMC_SHOT_TRAJECTORY_WOODWORK)));
                } else if (edge < 6) {
                    assert((r.outcome == RMC_SHOT_BLOCKED) == (edge == 4));
                } else if (edge < 8) {
                    assert((r.outcome == RMC_SHOT_GOAL) == (edge == 6));
                } else {
                    assert(r.outcome == (edge == 8 ? RMC_SHOT_SAVED_HELD : RMC_SHOT_SAVED_REBOUND));
                }
                found[edge] = true;
                --remaining;
            }
        }
        (void)rmc_pcg32_next(&seed);
    }
    assert(remaining == 0);
}

static RmcMatchState near_deadline(bool second, bool home)
{
    RmcMatchState state = active(home);
    state.first_half_stoppage_seconds = 60;
    assert(rmc_match_clock_advance(&world, &state, 2760u - state.elapsed_seconds));
    if (second) {
        assert(rmc_match_transition_apply(&world, &state, RMC_TRANSITION_HALF_TIME, 2760));
        assert(rmc_match_transition_apply(&world, &state, RMC_TRANSITION_SECOND_HALF, 2760));
        state.second_half_stoppage_seconds = 90;
        state.mode = RMC_MATCH_BALL_IN_PLAY;
        assert(rmc_match_clock_advance(&world, &state, 2785));
    } else {
        /* Set up a legal starting clock five seconds before the boundary. */
        state.elapsed_seconds -= 5;
    }
    possession(&state, home);
    state.possession.action_count = 31;
    assert(rmc_match_state_valid(&world, &state));
    return state;
}

static void test_deadlines_and_window(void)
{
    for (unsigned second = 0; second < 2; ++second) {
        for (unsigned home = 0; home < 2; ++home) {
            RmcMatchState state = near_deadline(second != 0, home != 0);
            RmcShotAction action = action_for(&state);
            for (unsigned code = RMC_SHOT_GOAL; code <= RMC_SHOT_WOODWORK; ++code) {
                RmcPcg32 seed = seed_for(&state, &action, (RmcShotEventOutcome)code,
                                        RMC_SHOT_TRAJECTORY_NONE);
                assert((unsigned)check(&state, &action, seed).outcome == code);
                Run run;
                init_run(&run, &state, seed);
                const size_t maximum = second ? 5u : 4u;
                run.log.capacity = maximum - 1u;
                reject(&run, &action); /* reserve GOAL even for non-goal seeds */
                run.log.capacity = maximum;
                assert(apply(&run, &action));
                if (code == RMC_SHOT_GOAL) { assert(run.log.count == maximum); }
            }
            Run run;
            init_run(&run, &state, stream());
            ++action.duration_seconds;
            reject(&run, &action); /* never truncate a crossing shot */
            --action.duration_seconds;
            run.state.possession.action_count = 32;
            reject(&run, &action);
            run.state.elapsed_seconds += 5;
            reject(&run, &action); /* END_PERIOD outranks the window reset */
        }
    }
    RmcMatchState state = active(true);
    state.possession.action_count = 31;
    state.possession.zone = (RmcTacticalZone){2, RMC_LANE_RIGHT};
    state.possession.phase = RMC_PHASE_PROGRESSION;
    RmcShotAction action = action_for(&state);
    for (unsigned code = RMC_SHOT_SAVED_REBOUND; code <= RMC_SHOT_WOODWORK; ++code) {
        if (code == RMC_SHOT_WIDE) { continue; }
        RmcPcg32 seed = seed_for(&state, &action, (RmcShotEventOutcome)code,
                                RMC_SHOT_TRAJECTORY_NONE);
        (void)check(&state, &action, seed);
        Run run;
        init_run(&run, &state, seed);
        assert(apply(&run, &action));
        assert(run.state.possession.action_count == 32);
        RmcSequenceDecision decision;
        assert(rmc_match_sequence_decide(&world, &run.state, &decision));
        assert(decision == RMC_SEQUENCE_RESET_ACTION_WINDOW);
        reject(&run, &action);
        assert(rmc_match_sequence_apply(&world, &run.state, decision));
        assert(run.state.possession.action_count == 0 && run.state.possession.controller.value == 0);
        assert(run.log.count == 2); /* reset adds no event or restart */
    }
}

static void test_rejections(void)
{
    RmcMatchState state = active(true);
    const RmcShotAction legal = action_for(&state);
    for (unsigned which = 0; which < 42; ++which) {
        Run run;
        init_run(&run, &state, stream());
        RmcShotAction action = legal;
        switch (which) {
        case 0: action.team_id.value = 0; break;
        case 1: action.team_id = state.fixture.away_team; break;
        case 2: action.shooter.value = 0; break;
        case 3: action.shooter = state.home_lineup.players[2]; break;
        case 4: action.defender.value = 0; break;
        case 5: action.defender = action.shooter; break;
        case 6: action.defender = state.home_lineup.players[2]; break;
        case 7: action.defender = action.goalkeeper; break;
        case 8: action.goalkeeper.value = 0; break;
        case 9: action.goalkeeper = state.home_lineup.players[0]; break;
        case 10: action.goalkeeper = state.away_lineup.players[2]; break;
        case 11: action.context = -21; break;
        case 12: action.context = 21; break;
        case 13: action.duration_seconds = 0; break;
        case 14: action.duration_seconds = UINT32_MAX; break;
        case 15: run.state.possession.zone.sector = 0; break;
        case 16: run.state.possession.zone.sector = 1; break;
        case 17: run.state.possession.zone.sector = 5; break;
        case 18: run.state.possession.zone.lane = RMC_LANE_COUNT; break;
        case 19: run.state.possession.zone.lane = (RmcTacticalLane)-1; break;
        case 20: run.state.possession.action_count = 32; break;
        case 21: run.state.possession.action_count = UINT32_MAX; break;
        case 22: run.state.possession.controller.value = 0; break;
        case 23: run.state.possession.initiator.value = 0; break;
        case 24: run.state.possession = (RmcMatchPossession){0}; break;
        case 25: run.state.mode = RMC_MATCH_RESTART_PENDING; break;
        case 26: run.state.period = RMC_MATCH_NOT_STARTED; break;
        case 27: run.state.period = RMC_MATCH_FINISHED; break;
        case 28: run.rng.increment &= ~UINT64_C(1); break;
        case 29: run.state.home_goals = UINT16_MAX; break;
        case 30: run.state.elapsed_seconds = UINT32_MAX; break;
        case 31: run.state.possession.elapsed_seconds = UINT32_MAX; break;
        case 32: run.log.match_id.value = 0; break;
        case 33: run.log.capacity = 2; break;
        case 34: run.log.count = run.log.capacity + 1u; break;
        case 35: run.log.capacity = SIZE_MAX; break;
        case 36: run.log.count = SIZE_MAX; break;
        case 37: run.log.events = NULL; break;
        case 38: run.state.fixture.id.value = 0; break;
        case 39: action.defender.value = UINT64_MAX; break;
        case 40: action.goalkeeper.value = UINT64_MAX; break;
        case 41:
            action.shooter = state.home_lineup.players[0];
            run.state.possession.controller = action.shooter;
            break;
        default: assert(false);
        }
        reject(&run, &action);
    }
    /* Bench actors, including the other roster goalkeeper, are ineligible. */
    for (size_t team = 0; team < 2; ++team) {
        const RmcLineup *lineup = team == 0 ? &state.home_lineup : &state.away_lineup;
        for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
            const RmcPlayer *p = &world.rosters[team].players[i];
            bool on_field = false;
            for (size_t j = 0; j < RMC_LINEUP_PLAYERS; ++j) {
                on_field = on_field || p->id.value == lineup->players[j].value;
            }
            if (!on_field) {
                Run run;
                init_run(&run, &state, stream());
                RmcShotAction action = legal;
                if (team == 0) {
                    action.shooter = p->id;
                    run.state.possession.controller = p->id;
                } else if (p->department == RMC_DEPARTMENT_GOALKEEPER) {
                    action.goalkeeper = p->id;
                } else {
                    action.defender = p->id;
                }
                reject(&run, &action);
            }
        }
    }
    /* Invalid authoritative world/model support, membership and numerics. */
    for (unsigned which = 0; which < 9; ++which) {
        world = original_world;
        RmcPlayer *s = player(legal.shooter);
        switch (which) {
        case 0: s->energy = 101; break;
        case 1: s->morale = 101; break;
        case 2: s->attributes[RMC_ATTR_TIRO] = 101; break;
        case 3: s->team_id = state.fixture.away_team; break;
        case 4: s->department = RMC_DEPARTMENT_GOALKEEPER; break;
        case 5: world.identity.rules_version = 2; break;
        case 6: world.identity.generator_version = 2; break;
        case 7: player(legal.goalkeeper)->goalkeeper_attributes[RMC_GK_ATTR_PRESA] = 101; break;
        case 8: s->attributes[RMC_ATTR_TIRO] = 0; break; /* pre-existing domain mismatch */
        default: assert(false);
        }
        Run run;
        init_run(&run, &state, stream());
        reject(&run, &legal);
    }
    world = original_world;
    Run run;
    init_run(&run, &state, stream());
    unsigned char before[sizeof(run)];
    memcpy(before, &run, sizeof(run));
    for (unsigned absent = 0; absent < 6; ++absent) {
        assert(!rmc_match_shot_apply(absent == 0 ? NULL : &world,
            absent == 1 ? NULL : &run.state, absent == 2 ? NULL : &run.rng,
            absent == 3 ? NULL : &run.log, absent == 4 ? NULL : &legal,
            absent == 5 ? NULL : &run.result));
        assert(memcmp(before, &run, sizeof(run)) == 0);
    }
}

static void test_prefix(void)
{
    RmcMatchState state = active(true);
    RmcShotAction action = action_for(&state);
    for (unsigned corrupt = 0; corrupt < 14; ++corrupt) {
        Run run;
        init_run(&run, &state, stream());
        RmcMatchEvent prefix[2] = {{0}};
        for (size_t i = 0; i < 2; ++i) {
            prefix[i].id = i + 1u;
            prefix[i].match_id = state.fixture.id;
            prefix[i].at_second = (uint32_t)i + 1u;
            prefix[i].type = RMC_EVENT_ACTION;
            prefix[i].team_id = action.team_id;
            prefix[i].primary_actor = action.shooter;
        }
        prefix[1].cause_event_id = 1;
        assert(rmc_match_event_log_append_batch(&world, &state, &run.log, prefix, 2));
        switch (corrupt) {
        case 0: run.storage[0].id = 9; break;
        case 1: run.storage[0].match_id.value = 0; break;
        case 2: run.storage[0].type = RMC_EVENT_TYPE_COUNT; break;
        case 3: run.storage[0].at_second = state.elapsed_seconds + 1u; break;
        case 4: run.storage[0].at_second = 3; break;
        case 5: run.storage[0].cause_event_id = 1; break;
        case 6: run.storage[0].team_id.value = UINT64_MAX; break;
        case 7: run.storage[0].primary_actor.value = UINT64_MAX; break;
        case 8: run.storage[1].secondary_actor.value = UINT64_MAX; break;
        case 9: run.storage[1].cause_event_id = 3; break;
        case 10: run.storage[0].shot.model_version = 2; break;
        case 11: run.storage[0].shot.chance_quality = 1; break;
        case 12: run.storage[0].outcome_code = RMC_SHOT_CHANCE; break;
        case 13: run.storage[1].cause_event_id = UINT64_MAX; break;
        default: assert(false);
        }
        reject(&run, &action);
    }
    /* A prior schema-v2 shot is revalidated, not trusted as just an envelope. */
    RmcPcg32 seed = seed_for(&state, &action, RMC_SHOT_SAVED_HELD, RMC_SHOT_TRAJECTORY_NONE);
    for (unsigned corrupt = 0; corrupt < 5; ++corrupt) {
        Run run;
        init_run(&run, &state, seed);
        assert(apply(&run, &action));
        possession(&run.state, true); /* explicit later caller decision */
        switch (corrupt) {
        case 0: run.storage[1].cause_event_id = 0; break;
        case 1: run.storage[1].shot.chance_quality = 101; break;
        case 2: run.storage[1].shot.trajectory = RMC_SHOT_TRAJECTORY_WIDE; break;
        case 3: run.storage[0].shot.applied_talent_mask = UINT32_MAX; break;
        case 4: run.storage[1].shot.goalkeeper_id = action.defender; break;
        default: assert(false);
        }
        reject(&run, &action);
    }
    /* Nonempty valid prefix, exact maximum capacity and untouched tail. */
    Run run;
    init_run(&run, &state, seed);
    assert(apply(&run, &action));
    RmcMatchEvent saved[2];
    memcpy(saved, run.storage, sizeof(saved));
    possession(&run.state, true);
    run.log.capacity = 4; /* two free cannot reserve the possible goal */
    reject(&run, &action);
    run.log.capacity = 5;
    assert(apply(&run, &action));
    assert(memcmp(saved, run.storage, sizeof(saved)) == 0);
    assert(run.storage[2].id == 3 && run.storage[3].cause_event_id == 3);
    for (size_t i = run.log.count * sizeof(run.storage[0]); i < sizeof(run.storage); ++i) {
        assert(((const unsigned char *)run.storage)[i] == 0xa5);
    }
}

static void test_replay_across_halves(void)
{
    RmcMatchState state = active(true);
    RmcShotAction action = action_for(&state);
    const RmcPcg32 seed = seed_for(&state, &action, RMC_SHOT_GOAL, RMC_SHOT_TRAJECTORY_NONE);
    Run a, b;
    init_run(&a, &state, seed);
    init_run(&b, &state, seed);
    Run *runs[] = {&a, &b};
    for (size_t i = 0; i < 2; ++i) {
        Run *run = runs[i];
        assert(apply(run, &action));
        const RmcMatchEvent old_goal = run->storage[2];
        /* Explicit fixtures for future decisions; no recovery/restart engine
         * is simulated or added to production Shot v1.
         */
        possession(&run->state, false);
        assert(rmc_match_clock_advance(&world, &run->state, 2700u - run->state.elapsed_seconds));
        assert(rmc_match_period_commit(&world, &run->state, &run->log));
        assert(rmc_match_transition_apply(&world, &run->state, RMC_TRANSITION_SECOND_HALF, 2700));
        possession(&run->state, false);
        RmcShotAction second = action_for(&run->state);
        assert(apply(run, &second)); /* validates the first-half shot prefix */
        same_event(&old_goal, &run->storage[2]);
        assert(run->state.home_goals == state.home_goals + 1u);
    }
    same_state(&a.state, &b.state);
    assert(a.rng.state == b.rng.state && a.rng.increment == b.rng.increment);
    assert(a.log.count == b.log.count);
    for (size_t i = 0; i < a.log.count; ++i) {
        same_event(&a.storage[i], &b.storage[i]);
    }
}

int main(void)
{
    setup();
    test_formulas();
    test_zones_and_context();
    test_composure();
    test_outcomes_and_draws();
    test_interval_edges();
    test_deadlines_and_window();
    test_rejections();
    test_prefix();
    test_replay_across_halves();
    puts("SHOT_SIX_OUTCOMES_AND_SPORTING_EFFECTS=PASS");
    puts("SHOT_FROZEN_FORMULAS_BOUNDS_NORMALIZATION_AND_COMPOSURE=PASS");
    puts("SHOT_BOUNDED_PCG32_ORDER_SKIPS_REJECTION_AND_INTERVAL_EDGES=PASS");
    puts("SHOT_CAUSAL_CHAIN_QUALITY_AND_EXACTLY_ONCE_SCORE=PASS");
    puts("SHOT_DEADLINES_WINDOW_AND_SUPPRESSED_RESTARTS=PASS");
    puts("SHOT_VALIDATION_PREFIX_CAPACITY_AND_BYTE_ATOMICITY=PASS");
    puts("SHOT_DETERMINISTIC_REPLAY_ACROSS_HALVES=PASS");
    return 0;
}
