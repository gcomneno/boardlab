#include "action_menu3d.h"
#include "input3d.h"

#include <assert.h>
#include <stdio.h>

static void catalog_qualification(void)
{
    const Chrono3DActionEntry *entries = chrono_3d_actions();
    assert(entries != NULL);

    for (int i = 0; i < CHRONO_3D_ACTION_COUNT; ++i) {
        assert(entries[i].label != NULL);
        assert(entries[i].label[0] != '\0');

        for (int j = i + 1; j < CHRONO_3D_ACTION_COUNT; ++j)
            assert(entries[i].id != entries[j].id);
    }

    assert(CHRONO_3D_ACTION_COUNT == 26);
    /* The catalog must be complete regardless of presentation order. */
    for (int expected = 0; expected < CHRONO_3D_ACTION_COUNT;
         ++expected) {
        bool found = false;
        for (int i = 0; i < CHRONO_3D_ACTION_COUNT; ++i) {
            if (entries[i].id == (ChronoAction)expected) {
                found = true;
                break;
            }
        }
        assert(found);
    }
    puts("CATALOG_26_GATE=PASS");
}

static void act(ChronoState *s, ChronoAction action)
{
    const Chrono3DActionEntry *entries = chrono_3d_actions();
    bool found = false;

    for (int i = 0; i < CHRONO_3D_ACTION_COUNT; ++i) {
        if (entries[i].id == action) {
            found = true;
            break;
        }
    }

    assert(found);
    assert(chrono_3d_action_status(s, action) ==
           CHRONO_3D_ACTION_AVAILABLE);
    assert(chrono_3d_apply(s, action) == CHRONO_OK);
}

static ChronoTrace end_turn(ChronoState *s, unsigned turn)
{
    ChronoTrace trace = {0};

    assert(s->turn == turn);
    assert(s->active_player ==
           (turn % 2 ? CHRONO_PLAYER_A : CHRONO_PLAYER_B));
    assert(chrono_3d_end_turn(s, &trace) == CHRONO_OK);
    return trace;
}

static void bridge_route(void)
{
    ChronoState s;
    chrono_init(&s);

    assert(chrono_3d_action_status(
        &s, CHRONO_ACTION_INSPECT_RUINS) ==
        CHRONO_3D_ACTION_OTHER_PLAYER);
    assert(chrono_3d_action_status(
        &s, CHRONO_ACTION_APPROVE_BRIDGE) ==
        CHRONO_3D_ACTION_BLOCKED);

    act(&s, CHRONO_ACTION_MOVE_ARCHIVE);
    assert(chrono_3d_action_status(
        &s, CHRONO_ACTION_MOVE_ARCHIVE) ==
        CHRONO_3D_ACTION_DONE);
    act(&s, CHRONO_ACTION_READ_PLANS);

    assert(s.action_points == 0);
    assert(chrono_3d_action_status(
        &s, CHRONO_ACTION_WAIT) ==
        CHRONO_3D_ACTION_BLOCKED);
    end_turn(&s, 1);

    act(&s, CHRONO_ACTION_INSPECT_RUINS);
    act(&s, CHRONO_ACTION_IDENTIFY_DEFECT);
    end_turn(&s, 2);

    act(&s, CHRONO_ACTION_SELECT_BRIDGE);
    act(&s, CHRONO_ACTION_APPROVE_BRIDGE);
    {
        /* Isolate the temporal prerequisite from the AP limit. */
        ChronoState premature = s;
        premature.action_points = 1;
        assert(chrono_apply(&premature, CHRONO_ACTION_VERIFY_PLAN)
               == CHRONO_PREREQUISITE_MISSING);
        assert(!premature.project_verified);
    }
    assert(!s.world.bridge_exists);

    ChronoTrace trace = end_turn(&s, 3);
    assert(trace.changed);
    assert(!trace.before.bridge_exists);
    assert(trace.after.bridge_exists);
    assert(s.world.bridge_exists);

    act(&s, CHRONO_ACTION_CROSS_BRIDGE);
    act(&s, CHRONO_ACTION_REACH_TOWER);
    end_turn(&s, 4);

    {
        ChronoState premature = s;
        premature.world.capsule_recovered = true;
        assert(!chrono_is_victory(&premature));
    }
    act(&s, CHRONO_ACTION_VERIFY_PLAN);
    assert(s.project_verified);
    end_turn(&s, 5);

    act(&s, CHRONO_ACTION_ENTER_TOWER);
    act(&s, CHRONO_ACTION_RECOVER_CAPSULE);
    end_turn(&s, 6);

    assert(s.finished && s.victory);
    assert(s.turn == 6);
    assert(s.world.capsule_recovered);
    assert(!s.world.tide_register_acquired);
    puts("BRIDGE_UI_ADAPTER_VICTORY_T6=PASS");
}

static void duck_route(void)
{
    ChronoState s;
    chrono_init(&s);

    act(&s, CHRONO_ACTION_MOVE_ARCHIVE);
    act(&s, CHRONO_ACTION_READ_PLANS);
    end_turn(&s, 1);

    act(&s, CHRONO_ACTION_INSPECT_RUINS);
    act(&s, CHRONO_ACTION_MEASURE_RIVER);
    end_turn(&s, 2);

    act(&s, CHRONO_ACTION_SELECT_DUCK);
    act(&s, CHRONO_ACTION_APPROVE_DUCK);

    assert(!s.world.duck_exists);
    ChronoTrace trace = end_turn(&s, 3);
    assert(trace.changed);
    assert(s.world.duck_exists);

    act(&s, CHRONO_ACTION_INSPECT_DUCK);
    act(&s, CHRONO_ACTION_PREPARE_DUCK);
    end_turn(&s, 4);
    assert(s.world.duck_prepared);

    assert(chrono_apply(&s, CHRONO_ACTION_VERIFY_INTERVENTION)
           == CHRONO_PREREQUISITE_MISSING);
    act(&s, CHRONO_ACTION_READ_MANUAL);
    act(&s, CHRONO_ACTION_CALIBRATE_DUCK);
    assert(s.calibration_pending);
    assert(!s.world.duck_calibrated);

    trace = end_turn(&s, 5);
    assert(trace.changed);
    assert(trace.cause == CHRONO_ACTION_CALIBRATE_DUCK);
    assert(s.world.duck_calibrated);
    assert(s.world.duck_prepared);

    act(&s, CHRONO_ACTION_START_DUCK);
    act(&s, CHRONO_ACTION_CROSS_RIVER);
    end_turn(&s, 6);
    assert(s.world.tide_register_acquired);

    {
        ChronoState premature = s;
        premature.world.capsule_recovered = true;
        assert(!chrono_is_victory(&premature));
    }
    act(&s, CHRONO_ACTION_VERIFY_INTERVENTION);
    assert(s.intervention_verified);
    end_turn(&s, 7);

    act(&s, CHRONO_ACTION_ENTER_TOWER);
    act(&s, CHRONO_ACTION_RECOVER_CAPSULE);
    end_turn(&s, 8);

    assert(s.finished && s.victory);
    assert(s.turn == 8);
    assert(s.world.capsule_recovered);
    assert(s.world.tide_register_acquired);
    puts("DUCK_UI_ADAPTER_VICTORY_T8=PASS");
    puts("DEFERRED_CALIBRATION_GATE=PASS");
}

static void decoy_qualification(void)
{
    const ChronoAction actions[] = {
        CHRONO_ACTION_QUESTION_PIGEON,
        CHRONO_ACTION_CHECK_HOROSCOPE,
        CHRONO_ACTION_THROW_STONE,
        CHRONO_ACTION_KNOCK_TOWER
    };

    for (unsigned i = 0; i < 4; ++i) {
        ChronoState s;
        chrono_init(&s);

        const ChronoPlayer owner =
            actions[i] == CHRONO_ACTION_CHECK_HOROSCOPE
                ? CHRONO_PLAYER_A : CHRONO_PLAYER_B;
        s.active_player = owner;

        if (actions[i] == CHRONO_ACTION_KNOCK_TOWER) {
            ChronoState before = s;
            assert(chrono_3d_action_status(&s, actions[i]) ==
                   CHRONO_3D_ACTION_BLOCKED);
            assert(chrono_apply(&s, actions[i]) ==
                   CHRONO_PREREQUISITE_MISSING);
            assert(s.action_points == before.action_points);
            assert(s.location_b == before.location_b);
            s.location_b = CHRONO_LOCATION_TOWER;
        }

        assert(chrono_3d_action_for_player(actions[i], owner));
        assert(!chrono_3d_action_for_player(
            actions[i],
            owner == CHRONO_PLAYER_A
                ? CHRONO_PLAYER_B : CHRONO_PLAYER_A
        ));

        assert(chrono_3d_action_status(&s, actions[i]) ==
               CHRONO_3D_ACTION_AVAILABLE);
        const unsigned ap = s.action_points;
        assert(chrono_3d_apply(&s, actions[i]) == CHRONO_OK);
        assert(s.action_points == ap - 1);
        assert(!s.finished);
        assert(!s.victory);

        /* Repeated decoys have no completion flag. */
        if (s.action_points > 0) {
            assert(chrono_3d_action_status(&s, actions[i]) ==
                   CHRONO_3D_ACTION_AVAILABLE);
        }

        /* A player without AP cannot perform another action. */
        s.action_points = 0;
        assert(chrono_3d_action_status(&s, actions[i]) ==
               CHRONO_3D_ACTION_BLOCKED);
        assert(chrono_apply(&s, actions[i]) ==
               CHRONO_NO_ACTION_POINTS);
    }

    puts("DECOY_OWNER_GATE=PASS");
    puts("DECOY_CORE_AP_GATE=PASS");
    puts("DECOY_TOWER_LOCATION_GATE=PASS");
    puts("DECOY_REPEATABILITY_GATE=PASS");
}

static void deadline_route(void)
{
    ChronoState s;
    chrono_init(&s);

    for (unsigned turn = 1; turn <= CHRONO_MAX_TURNS; ++turn) {
        assert(chrono_3d_action_status(
            &s, CHRONO_ACTION_WAIT) ==
            CHRONO_3D_ACTION_AVAILABLE);

        ChronoTrace trace = end_turn(&s, turn);
        assert(!trace.changed);
    }

    assert(s.finished && !s.victory);
    assert(chrono_3d_action_status(
        &s, CHRONO_ACTION_WAIT) ==
        CHRONO_3D_ACTION_BLOCKED);
    assert(chrono_3d_apply(&s, CHRONO_ACTION_WAIT) ==
           CHRONO_MISSION_FINISHED);

    puts("DEADLINE_UI_ADAPTER_DEFEAT_T8=PASS");
}

int main(void)
{
    catalog_qualification();
    bridge_route();
    duck_route();
    decoy_qualification();
    deadline_route();
    puts("CHRONO_3D_FUNCTIONAL_QUALIFICATION=PASS");
    return 0;
}
