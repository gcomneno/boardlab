#include "action_menu3d.h"

static const Chrono3DActionEntry entries[CHRONO_3D_ACTION_COUNT] = {
    {CHRONO_ACTION_CHECK_HOROSCOPE, "Consulta oroscopo temporale"},
    {CHRONO_ACTION_INSPECT_RUINS, "Esamina resti ponte"},
    {CHRONO_ACTION_SELECT_DUCK, "Seleziona Papera"},
    {CHRONO_ACTION_REACH_TOWER, "Raggiungi torre"},
    {CHRONO_ACTION_READ_PLANS, "Consulta progetti"},
    {CHRONO_ACTION_THROW_STONE, "Lancia sasso nel fiume"},
    {CHRONO_ACTION_VERIFY_PLAN, "Verifica progetto"},
    {CHRONO_ACTION_PREPARE_DUCK, "Prepara Papera"},
    {CHRONO_ACTION_APPROVE_BRIDGE, "Approva ponte"},
    {CHRONO_ACTION_MEASURE_RIVER, "Misura il fiume"},
    {CHRONO_ACTION_ENTER_TOWER, "Entra nella torre"},
    {CHRONO_ACTION_READ_MANUAL, "Leggi manuale"},
    {CHRONO_ACTION_CROSS_BRIDGE, "Attraversa ponte"},
    {CHRONO_ACTION_QUESTION_PIGEON, "Interroga il piccione"},
    {CHRONO_ACTION_MOVE_ARCHIVE, "Raggiungi archivio"},
    {CHRONO_ACTION_CALIBRATE_DUCK, "Tara Papera"},
    {CHRONO_ACTION_SELECT_BRIDGE, "Seleziona ponte"},
    {CHRONO_ACTION_INSPECT_DUCK, "Esamina Papera"},
    {CHRONO_ACTION_VERIFY_INTERVENTION, "Verifica intervento"},
    {CHRONO_ACTION_KNOCK_TOWER, "Bussa alla torre"},
    {CHRONO_ACTION_APPROVE_DUCK, "Approva Papera"},
    {CHRONO_ACTION_START_DUCK, "Avvia Papera"},
    {CHRONO_ACTION_IDENTIFY_DEFECT, "Identifica difetto"},
    {CHRONO_ACTION_CROSS_RIVER, "Attraversa fiume"},
    {CHRONO_ACTION_WAIT, "Attendi"},
    {CHRONO_ACTION_RECOVER_CAPSULE, "Recupera capsula"},
};

const Chrono3DActionEntry *chrono_3d_actions(void)
{
    return entries;
}

bool chrono_3d_action_for_player(
    ChronoAction action, ChronoPlayer player
)
{
    if (action == CHRONO_ACTION_WAIT) return true;

    switch (action) {
    case CHRONO_ACTION_MOVE_ARCHIVE:
    case CHRONO_ACTION_READ_PLANS:
    case CHRONO_ACTION_SELECT_BRIDGE:
    case CHRONO_ACTION_APPROVE_BRIDGE:
    case CHRONO_ACTION_SELECT_DUCK:
    case CHRONO_ACTION_APPROVE_DUCK:
    case CHRONO_ACTION_VERIFY_PLAN:
    case CHRONO_ACTION_READ_MANUAL:
    case CHRONO_ACTION_CALIBRATE_DUCK:
    case CHRONO_ACTION_VERIFY_INTERVENTION:
    case CHRONO_ACTION_CHECK_HOROSCOPE:
        return player == CHRONO_PLAYER_A;

    case CHRONO_ACTION_INSPECT_RUINS:
    case CHRONO_ACTION_IDENTIFY_DEFECT:
    case CHRONO_ACTION_MEASURE_RIVER:
    case CHRONO_ACTION_CROSS_BRIDGE:
    case CHRONO_ACTION_REACH_TOWER:
    case CHRONO_ACTION_INSPECT_DUCK:
    case CHRONO_ACTION_PREPARE_DUCK:
    case CHRONO_ACTION_START_DUCK:
    case CHRONO_ACTION_CROSS_RIVER:
    case CHRONO_ACTION_ENTER_TOWER:
    case CHRONO_ACTION_RECOVER_CAPSULE:
    case CHRONO_ACTION_QUESTION_PIGEON:
    case CHRONO_ACTION_THROW_STONE:
    case CHRONO_ACTION_KNOCK_TOWER:
        return player == CHRONO_PLAYER_B;

    default:
        return false;
    }
}

static bool action_done(const ChronoState *s, ChronoAction action)
{
    switch (action) {
    case CHRONO_ACTION_MOVE_ARCHIVE:
        return s->location_a == CHRONO_LOCATION_ARCHIVE;
    case CHRONO_ACTION_READ_PLANS:
        return s->plans_read;
    case CHRONO_ACTION_INSPECT_RUINS:
        return s->ruins_inspected;
    case CHRONO_ACTION_IDENTIFY_DEFECT:
        return s->defect_known;
    case CHRONO_ACTION_MEASURE_RIVER:
        return s->river_measured;
    case CHRONO_ACTION_SELECT_BRIDGE:
        return s->plan_selected &&
               s->selected_plan == CHRONO_PLAN_BRIDGE;
    case CHRONO_ACTION_APPROVE_BRIDGE:
        return s->plan_approved &&
               s->selected_plan == CHRONO_PLAN_BRIDGE;
    case CHRONO_ACTION_SELECT_DUCK:
        return s->plan_selected &&
               s->selected_plan == CHRONO_PLAN_DUCK;
    case CHRONO_ACTION_APPROVE_DUCK:
        return s->plan_approved &&
               s->selected_plan == CHRONO_PLAN_DUCK;
    case CHRONO_ACTION_CROSS_BRIDGE:
        return s->bridge_crossed;
    case CHRONO_ACTION_REACH_TOWER:
        return s->tower_reached;
    case CHRONO_ACTION_VERIFY_PLAN:
        return s->project_verified;
    case CHRONO_ACTION_INSPECT_DUCK:
        return s->duck_inspected;
    case CHRONO_ACTION_PREPARE_DUCK:
        return s->world.duck_prepared;
    case CHRONO_ACTION_READ_MANUAL:
        return s->manual_read;
    case CHRONO_ACTION_CALIBRATE_DUCK:
        return s->world.duck_calibrated ||
               s->calibration_pending;
    case CHRONO_ACTION_START_DUCK:
        return s->world.duck_running;
    case CHRONO_ACTION_CROSS_RIVER:
        return s->river_crossed;
    case CHRONO_ACTION_VERIFY_INTERVENTION:
        return s->intervention_verified;
    case CHRONO_ACTION_ENTER_TOWER:
        return s->tower_entered;
    case CHRONO_ACTION_RECOVER_CAPSULE:
        return s->world.capsule_recovered;
    case CHRONO_ACTION_WAIT:
    default:
        return false;
    }
}

Chrono3DActionStatus chrono_3d_action_status(
    const ChronoState *state, ChronoAction action
)
{
    if (state == NULL) return CHRONO_3D_ACTION_BLOCKED;

    if (!chrono_3d_action_for_player(action, state->active_player))
        return CHRONO_3D_ACTION_OTHER_PLAYER;

    if (action_done(state, action))
        return CHRONO_3D_ACTION_DONE;

    if (state->finished || state->action_points == 0)
        return CHRONO_3D_ACTION_BLOCKED;

    ChronoState probe = *state;
    return chrono_apply(&probe, action) == CHRONO_OK
        ? CHRONO_3D_ACTION_AVAILABLE
        : CHRONO_3D_ACTION_BLOCKED;
}
