#include "chrono.h"

#include <assert.h>
#include <stdio.h>

static void run_route(ChronoPlan plan)
{
    ChronoState state;
    ChronoTrace trace;

    chrono_init(&state);

    assert(state.turn == 1);
    assert(state.active_player == CHRONO_PLAYER_A);
    assert(state.action_points == 2);
    assert(!state.world.bridge_exists);
    assert(!state.world.duck_exists);

    /* Turn 1: A visits the archive. */
    assert(chrono_apply(&state, CHRONO_ACTION_MOVE_ARCHIVE)
           == CHRONO_OK);
    assert(chrono_apply(&state, CHRONO_ACTION_READ_PLANS)
           == CHRONO_OK);
    assert(state.action_points == 0);

    /* An exhausted turn must reject further actions. */
    assert(chrono_apply(&state, CHRONO_ACTION_WAIT)
           == CHRONO_NO_ACTION_POINTS);

    assert(chrono_end_turn(&state, &trace) == CHRONO_OK);
    assert(!trace.changed);
    assert(state.turn == 2);
    assert(state.active_player == CHRONO_PLAYER_B);

    /* Turn 2: B investigates the destroyed bridge. */
    assert(chrono_apply(&state, CHRONO_ACTION_INSPECT_RUINS)
           == CHRONO_OK);

    if (plan == CHRONO_PLAN_BRIDGE) {
        assert(chrono_apply(&state, CHRONO_ACTION_IDENTIFY_DEFECT)
               == CHRONO_OK);
    } else {
        assert(chrono_apply(&state, CHRONO_ACTION_MEASURE_RIVER)
               == CHRONO_OK);
    }

    assert(chrono_end_turn(&state, &trace) == CHRONO_OK);
    assert(state.turn == 3);
    assert(state.active_player == CHRONO_PLAYER_A);

    /* Turn 3: selection is not causal propagation. */
    if (plan == CHRONO_PLAN_BRIDGE) {
        assert(chrono_apply(&state, CHRONO_ACTION_SELECT_BRIDGE)
               == CHRONO_OK);
    } else {
        assert(chrono_apply(&state, CHRONO_ACTION_SELECT_DUCK)
               == CHRONO_OK);
    }

    assert(!state.world.bridge_exists);
    assert(!state.world.duck_exists);

    if (plan == CHRONO_PLAN_BRIDGE) {
        assert(chrono_apply(&state, CHRONO_ACTION_APPROVE_BRIDGE)
               == CHRONO_OK);
    } else {
        assert(chrono_apply(&state, CHRONO_ACTION_APPROVE_DUCK)
               == CHRONO_OK);
    }

    /* Even approval must wait for the turn boundary. */
    assert(!state.world.bridge_exists);
    assert(!state.world.duck_exists);

    assert(chrono_end_turn(&state, &trace) == CHRONO_OK);

    assert(trace.changed);
    assert(trace.turn == 3);
    assert(!trace.before.bridge_exists);
    assert(!trace.before.duck_exists);

    assert(state.world.bridge_exists ==
           (plan == CHRONO_PLAN_BRIDGE));
    assert(state.world.duck_exists ==
           (plan == CHRONO_PLAN_DUCK));

    assert(state.turn == 4);
    assert(state.active_player == CHRONO_PLAYER_B);
    assert(state.action_points == 2);
}


/*
 * End-to-end scenarios exercise only the public engine API.
 *
 * A turn has two action points. Passing WAIT as the second
 * argument below means "leave the remaining point unused";
 * the helper does not execute a WAIT action in that case.
 */
static void play_turn(
    ChronoState *state,
    unsigned turn,
    ChronoAction first,
    ChronoAction second
)
{
    ChronoTrace trace;

    assert(state->turn == turn);
    assert(state->active_player ==
           (turn % 2 ? CHRONO_PLAYER_A : CHRONO_PLAYER_B));

    assert(chrono_apply(state, first) == CHRONO_OK);

    if (second != CHRONO_ACTION_WAIT) {
        assert(chrono_apply(state, second) == CHRONO_OK);
    }

    assert(chrono_end_turn(state, &trace) == CHRONO_OK);
}

/*
 * The bridge route prioritizes speed over optional rewards.
 * The restored bridge is produced by historical approval,
 * not by a scripted victory at turn six.
 */
static void test_complete_bridge_mission(void)
{
    ChronoState state;
    chrono_init(&state);

    play_turn(&state, 1, CHRONO_ACTION_MOVE_ARCHIVE,
                       CHRONO_ACTION_READ_PLANS);
    play_turn(&state, 2, CHRONO_ACTION_INSPECT_RUINS,
                       CHRONO_ACTION_IDENTIFY_DEFECT);
    play_turn(&state, 3, CHRONO_ACTION_SELECT_BRIDGE,
                       CHRONO_ACTION_APPROVE_BRIDGE);
    play_turn(&state, 4, CHRONO_ACTION_CROSS_BRIDGE,
                       CHRONO_ACTION_REACH_TOWER);
    play_turn(&state, 5, CHRONO_ACTION_VERIFY_PLAN,
                       CHRONO_ACTION_WAIT);
    play_turn(&state, 6, CHRONO_ACTION_ENTER_TOWER,
                       CHRONO_ACTION_RECOVER_CAPSULE);

    assert(state.finished);
    assert(state.victory);
    assert(state.turn == 6);
    assert(chrono_is_victory(&state));
    assert(state.world.capsule_recovered);
    assert(!state.world.tide_register_acquired);

    puts("BRIDGE_VICTORY_TURN6=PASS");
}

/*
 * The duck route demonstrates planning with two objectives:
 * recover the capsule and obtain the optional tide register.
 *
 * Historical calibration must not destroy the work already
 * performed by B on the machine in 2025.
 */
static void test_complete_duck_mission(void)
{
    ChronoState state;
    ChronoTrace trace;
    chrono_init(&state);

    play_turn(&state, 1, CHRONO_ACTION_MOVE_ARCHIVE,
                       CHRONO_ACTION_READ_PLANS);
    play_turn(&state, 2, CHRONO_ACTION_INSPECT_RUINS,
                       CHRONO_ACTION_MEASURE_RIVER);
    play_turn(&state, 3, CHRONO_ACTION_SELECT_DUCK,
                       CHRONO_ACTION_APPROVE_DUCK);
    play_turn(&state, 4, CHRONO_ACTION_INSPECT_DUCK,
                       CHRONO_ACTION_PREPARE_DUCK);

    assert(state.world.duck_prepared);

    assert(chrono_apply(&state, CHRONO_ACTION_READ_MANUAL)
           == CHRONO_OK);
    assert(chrono_apply(&state, CHRONO_ACTION_CALIBRATE_DUCK)
           == CHRONO_OK);

    assert(state.calibration_pending);
    assert(!state.world.duck_calibrated);

    assert(chrono_end_turn(&state, &trace) == CHRONO_OK);

    assert(trace.changed);
    assert(trace.turn == 5);
    assert(trace.cause == CHRONO_ACTION_CALIBRATE_DUCK);
    assert(!trace.before.duck_calibrated);
    assert(trace.after.duck_calibrated);

    assert(state.world.duck_calibrated);
    assert(state.world.duck_prepared);
    assert(!state.calibration_pending);

    play_turn(&state, 6, CHRONO_ACTION_START_DUCK,
                       CHRONO_ACTION_CROSS_RIVER);

    assert(state.world.tide_register_acquired);

    play_turn(&state, 7, CHRONO_ACTION_VERIFY_INTERVENTION,
                       CHRONO_ACTION_WAIT);
    play_turn(&state, 8, CHRONO_ACTION_ENTER_TOWER,
                       CHRONO_ACTION_RECOVER_CAPSULE);

    assert(state.finished);
    assert(state.victory);
    assert(state.turn == 8);
    assert(chrono_is_victory(&state));
    assert(state.world.capsule_recovered);
    assert(state.world.tide_register_acquired);

    puts("DUCK_VICTORY_TURN8=PASS");
    puts("DUCK_CALIBRATION_DEFERRED=PASS");
    puts("DUCK_PREPARATION_PRESERVED=PASS");
    puts("TIDE_REGISTER_GATE=PASS");
}

/*
 * A causal trace must identify the historical decision
 * responsible for a change, or report no change at all.
 */
static void test_historical_trace(void)
{
    ChronoState state;
    ChronoTrace trace;

    chrono_init(&state);

    assert(chrono_end_turn(&state, &trace) == CHRONO_OK);
    assert(!trace.changed);
    assert(trace.cause == CHRONO_ACTION_WAIT);

    chrono_init(&state);
    state.selected_plan = CHRONO_PLAN_BRIDGE;
    state.plan_selected = true;
    state.plan_approved = true;

    assert(!state.world.bridge_exists);
    assert(chrono_end_turn(&state, &trace) == CHRONO_OK);

    assert(trace.changed);
    assert(trace.cause == CHRONO_ACTION_APPROVE_BRIDGE);
    assert(!trace.before.bridge_exists);
    assert(trace.after.bridge_exists);

    puts("HISTORICAL_TRACE_GATE=PASS");
}


/*
 * Semantic equality deliberately ignores struct padding.
 *
 * Every gameplay field is compared explicitly. If ChronoState
 * gains a field, this helper must be reviewed as well.
 */
static bool same_state(const ChronoState *a, const ChronoState *b)
{
    return a->active_player == b->active_player &&
           a->turn == b->turn &&
           a->action_points == b->action_points &&
           a->location_a == b->location_a &&
           a->location_b == b->location_b &&
           a->selected_plan == b->selected_plan &&
           a->plans_read == b->plans_read &&
           a->plan_selected == b->plan_selected &&
           a->plan_approved == b->plan_approved &&
           a->ruins_inspected == b->ruins_inspected &&
           a->duck_inspected == b->duck_inspected &&
           a->defect_known == b->defect_known &&
           a->river_measured == b->river_measured &&
           a->manual_read == b->manual_read &&
           a->calibration_pending == b->calibration_pending &&
           a->tower_reached == b->tower_reached &&
           a->tower_entered == b->tower_entered &&
           a->bridge_crossed == b->bridge_crossed &&
           a->river_crossed == b->river_crossed &&
           a->project_verified == b->project_verified &&
           a->intervention_verified == b->intervention_verified &&
           a->finished == b->finished &&
           a->victory == b->victory &&
           a->world.bridge_exists == b->world.bridge_exists &&
           a->world.duck_exists == b->world.duck_exists &&
           a->world.duck_calibrated == b->world.duck_calibrated &&
           a->world.duck_prepared == b->world.duck_prepared &&
           a->world.duck_running == b->world.duck_running &&
           a->world.capsule_recovered == b->world.capsule_recovered &&
           a->world.tide_register_acquired ==
               b->world.tide_register_acquired;
}

/*
 * Rejected actions must not change gameplay state or consume PA.
 * The test covers wrong player, missing prerequisites, invalid
 * action identifiers and exhausted action points.
 */
static void test_invalid_action_atomicity(void)
{
    ChronoState state;
    ChronoState before;

    chrono_init(&state);
    before = state;

    assert(chrono_apply(&state, CHRONO_ACTION_INSPECT_RUINS)
           == CHRONO_WRONG_PLAYER);
    assert(same_state(&state, &before));

    assert(chrono_apply(&state, CHRONO_ACTION_APPROVE_BRIDGE)
           == CHRONO_PREREQUISITE_MISSING);
    assert(same_state(&state, &before));

    assert(chrono_apply(&state, (ChronoAction)999)
           == CHRONO_INVALID_ACTION);
    assert(same_state(&state, &before));

    assert(chrono_apply(&state, CHRONO_ACTION_MOVE_ARCHIVE)
           == CHRONO_OK);
    assert(chrono_apply(&state, CHRONO_ACTION_READ_PLANS)
           == CHRONO_OK);

    before = state;
    assert(chrono_apply(&state, CHRONO_ACTION_WAIT)
           == CHRONO_NO_ACTION_POINTS);
    assert(same_state(&state, &before));

    puts("INVALID_ACTION_ATOMICITY_GATE=PASS");
}

/*
 * Determinism is a property of state transitions:
 * equal initial states plus equal actions must yield equal
 * subsequent states and causal traces.
 *
 * This test covers one representative bridge sequence.
 */
static void test_deterministic_replay(void)
{
    ChronoState a;
    ChronoState b;
    ChronoTrace ta;
    ChronoTrace tb;

    const ChronoAction sequence[] = {
        CHRONO_ACTION_MOVE_ARCHIVE,
        CHRONO_ACTION_READ_PLANS,
        CHRONO_ACTION_INSPECT_RUINS,
        CHRONO_ACTION_IDENTIFY_DEFECT,
        CHRONO_ACTION_SELECT_BRIDGE,
        CHRONO_ACTION_APPROVE_BRIDGE
    };

    chrono_init(&a);
    chrono_init(&b);

    for (unsigned i = 0; i < 6; ++i) {
        assert(chrono_apply(&a, sequence[i]) == CHRONO_OK);
        assert(chrono_apply(&b, sequence[i]) == CHRONO_OK);
        assert(same_state(&a, &b));

        if (i % 2 == 1) {
            assert(chrono_end_turn(&a, &ta) == CHRONO_OK);
            assert(chrono_end_turn(&b, &tb) == CHRONO_OK);

            assert(same_state(&a, &b));
            assert(ta.turn == tb.turn);
            assert(ta.cause == tb.cause);
            assert(ta.changed == tb.changed);
        }
    }

    assert(a.world.bridge_exists);
    assert(b.world.bridge_exists);

    puts("DETERMINISM_GATE=PASS");
}

/*
 * The mission deadline is global, not per player.
 * Once turn eight ends without the capsule, the game is lost
 * and subsequent actions cannot modify the final state.
 */
static void test_mission_deadline(void)
{
    ChronoState state;
    ChronoState before;

    chrono_init(&state);

    for (unsigned i = 0; i < CHRONO_MAX_TURNS; ++i) {
        assert(chrono_end_turn(&state, NULL) == CHRONO_OK);
    }

    assert(state.finished);
    assert(!state.victory);

    before = state;

    assert(chrono_apply(&state, CHRONO_ACTION_WAIT)
           == CHRONO_MISSION_FINISHED);
    assert(chrono_end_turn(&state, NULL)
           == CHRONO_MISSION_FINISHED);
    assert(same_state(&state, &before));

    puts("MISSION_DEADLINE_GATE=PASS");
}

int main(void)
{
    run_route(CHRONO_PLAN_BRIDGE);
    puts("BRIDGE_ROUTE_TURN3=PASS");

    run_route(CHRONO_PLAN_DUCK);
    puts("DUCK_ROUTE_TURN3=PASS");

    test_complete_bridge_mission();
    test_complete_duck_mission();
    test_historical_trace();
    test_invalid_action_atomicity();
    test_deterministic_replay();
    test_mission_deadline();

    puts("CAUSAL_PROPAGATION_GATE=PASS");
    puts("M001_PERMANENT_SUITE_GATE=PASS");
    return 0;
}
