#include "match_flow_selector.h"

/* State validation already establishes complete authoritative world/lineups.
 * Still resolve both roster ownership and slot AND department for each role. */
static RmcPlayerId participant_after(
    const RmcMatchFlow *f, RmcTeamId team, bool goalkeeper, RmcPlayerId exclude,
    uint64_t after)
{
    RmcPlayerId best = {0};
    const RmcLineup *l = team.value == f->state.fixture.home_team.value ? &f->state.home_lineup :
        team.value == f->state.fixture.away_team.value ? &f->state.away_lineup : NULL;
    if (l == NULL) { return best; }
    for (size_t t = 0; t < RMC_WORLD_TEAMS; ++t) {
        if (f->world->teams[t].id.value != team.value ||
            f->world->rosters[t].team_id.value != team.value) { continue; }
        for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
            if ((l->slots[i] == RMC_SLOT_POR) != goalkeeper) { continue; }
            for (size_t j = 0; j < RMC_TEAM_PLAYERS; ++j) {
                const RmcPlayer *p = &f->world->rosters[t].players[j];
                if (p->id.value != 0 && p->id.value > after && p->id.value == l->players[i].value &&
                    p->id.value != exclude.value && p->team_id.value == team.value &&
                    (p->department == RMC_DEPARTMENT_GOALKEEPER) == goalkeeper &&
                    (best.value == 0 || p->id.value < best.value)) { best = p->id; }
            }
        }
    }
    return best;
}

static RmcPlayerId participant(
    const RmcMatchFlow *f, RmcTeamId team, bool goalkeeper, RmcPlayerId exclude)
{
    return participant_after(f, team, goalkeeper, exclude, 0);
}

static RmcPlayerId pass_recipient(const RmcMatchFlow *f, RmcTeamId team, RmcPlayerId controller)
{
    if (f->config.selector_version == RMC_MATCH_SELECTOR_V2_VERSION) {
        /* Only the accepted prefix is history. Sporting success and current
         * eligibility of the recorded numeric anchor are intentionally irrelevant. */
        for (size_t i = f->journal.count; i > 0; --i) {
            const RmcMatchFlowCommand *c = &f->journal.commands[i - 1];
            if (c->kind != RMC_FLOW_PASS || c->data.pass.team_id.value != team.value) { continue; }
            const RmcPlayerId successor = participant_after(f, team, false, controller,
                c->data.pass.recipient.value);
            if (successor.value != 0) { return successor; }
            break;
        }
    }
    return participant(f, team, false, controller);
}

static uint32_t clipped(uint32_t nominal, uint32_t remaining)
{
    return nominal < remaining ? nominal : remaining;
}

RmcFlowError rmc_match_flow_select(
    const RmcMatchFlow *f, const RmcMatchFlowNext *next, RmcMatchFlowCommand *destination)
{
    if (next == NULL || destination == NULL) { return RMC_FLOW_INVALID_ARGUMENT; }
    RmcMatchFlowNext n;
    RmcFlowError err = rmc_match_flow_next(f, &n);
    if (err != RMC_FLOW_OK) { return err; }
    if (next->observed_command_count != n.observed_command_count) { return RMC_FLOW_STALE_COMMAND; }
    if (next->kind != n.kind || next->team_id.value != n.team_id.value ||
        next->deadline_second != n.deadline_second || next->remaining_seconds != n.remaining_seconds) {
        return RMC_FLOW_WRONG_OPERATION;
    }
    if (n.kind == RMC_FLOW_COMPLETE || n.kind == RMC_FLOW_NEED_INSTRUCTION) {
        return RMC_FLOW_WRONG_OPERATION;
    }
    if (n.observed_command_count == UINT64_MAX) { return RMC_FLOW_RESOURCE_EXHAUSTED; }
    RmcMatchFlowCommand c = {
        .command_id = n.observed_command_count + 1, .observed_command_count = n.observed_command_count,
        .match_id = f->state.fixture.id, .expected_period = f->state.period
    };
    const RmcPlayerId absent = {0};
    if (n.kind == RMC_FLOW_NEED_OPENING) {
        c.kind = RMC_FLOW_OPENING;
        c.data.opening = (RmcOpeningKickoffAction){
            f->state.period == RMC_MATCH_NOT_STARTED ? RMC_MATCH_FIRST_HALF : RMC_MATCH_SECOND_HALF,
            n.team_id, participant(f, n.team_id, false, absent)
        };
        if (c.data.opening.taker.value == 0) { return RMC_FLOW_INVALID_STATE; }
    } else if (n.kind == RMC_FLOW_NEED_CLOSURE) { c.kind = RMC_FLOW_CLOSE_PERIOD; }
    else if (n.kind == RMC_FLOW_NEED_RESET) { c.kind = RMC_FLOW_RESET_WINDOW; }
    else if (n.kind == RMC_FLOW_NEED_RESTART) {
        const RmcMatchEvent *shot = &f->log.events[f->log.count - 1];
        if (shot->type == RMC_EVENT_GOAL) { --shot; }
        c.kind = RMC_FLOW_RESTART;
        c.data.restart = (RmcRestartAction){shot->shot.restart_kind, n.team_id,
            participant(f, n.team_id, shot->shot.restart_kind == RMC_RESTART_GOAL_KICK, absent), shot->id};
        if (c.data.restart.taker.value == 0) { return RMC_FLOW_INVALID_STATE; }
    } else {
        if (n.remaining_seconds == 0) { return RMC_FLOW_INVALID_STATE; }
        const RmcMatchPossession *p = &f->state.possession;
        const RmcTeamId own = p->team_id;
        const RmcTeamId other = own.value == f->state.fixture.home_team.value ?
            f->state.fixture.away_team : f->state.fixture.home_team;
        const RmcPlayerId outfielder = participant(f, own, false, absent);
        const RmcPlayerId defender = participant(f, other, false, absent);
        if (outfielder.value == 0 || defender.value == 0) { return RMC_FLOW_INVALID_STATE; }
        if (n.kind == RMC_FLOW_NEED_RECOVERY) {
            c.kind = RMC_FLOW_RECOVERY;
            c.data.recovery = (RmcRecoveryAction){outfielder, defender, 0, clipped(4, n.remaining_seconds)};
        } else {
            const RmcPlayerId keeper = participant(f, own, true, absent);
            const bool goalkeeper = keeper.value != 0 && keeper.value == p->controller.value;
            /* Require the controller itself to be a genuine eligible role. */
            if (!goalkeeper) {
                bool found = false;
                const RmcLineup *l = own.value == f->state.fixture.home_team.value ?
                    &f->state.home_lineup : &f->state.away_lineup;
                for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
                    if (l->players[i].value != p->controller.value || l->slots[i] == RMC_SLOT_POR) { continue; }
                    for (size_t t = 0; t < RMC_WORLD_TEAMS; ++t) {
                        if (f->world->teams[t].id.value != own.value || f->world->rosters[t].team_id.value != own.value) { continue; }
                        for (size_t j = 0; j < RMC_TEAM_PLAYERS; ++j) {
                            const RmcPlayer *actor = &f->world->rosters[t].players[j];
                            if (actor->id.value == p->controller.value && actor->team_id.value == own.value &&
                                actor->department != RMC_DEPARTMENT_GOALKEEPER) { found = true; }
                        }
                    }
                }
                if (!found) { return RMC_FLOW_INVALID_STATE; }
            }
            const RmcFlowInstructions instructions = own.value == f->state.fixture.home_team.value ?
                f->home_instructions : f->away_instructions;
            const RmcFlowInstructions defending = other.value == f->state.fixture.home_team.value ?
                f->home_instructions : f->away_instructions;
            const uint64_t q = f->sporting_count % 5;
            if (goalkeeper) { c.kind = RMC_FLOW_PASS; }
            else if (q == 2 || (defending.pressure == RMC_FLOW_PRESS_HIGH && q == 1)) {
                c.kind = RMC_FLOW_TACKLE;
                c.data.tackle = (RmcTackleAction){other, p->controller, defender, 0, clipped(5, n.remaining_seconds)};
            } else if (p->zone.sector == 4 ||
                       (q == 4 && instructions.approach == RMC_FLOW_DIRECT && p->zone.sector >= 3)) {
                const RmcPlayerId opposing_keeper = participant(f, other, true, absent);
                if (opposing_keeper.value == 0) { return RMC_FLOW_INVALID_STATE; }
                c.kind = RMC_FLOW_SHOT;
                c.data.shot = (RmcShotAction){own, p->controller, defender, opposing_keeper, 0, clipped(6, n.remaining_seconds)};
            } else if (q == 0) { c.kind = RMC_FLOW_PASS; }
            else if (p->zone.sector < 4) {
                if (instructions.progression == RMC_FLOW_CARRY_FIRST) {
                    c.kind = RMC_FLOW_CARRY;
                    c.data.carry = (RmcCarryAction){own, p->controller, defender, 0, clipped(10, n.remaining_seconds)};
                } else {
                    c.kind = RMC_FLOW_DRIBBLE;
                    c.data.dribble = (RmcDribbleAction){own, p->controller, defender, 0, clipped(7, n.remaining_seconds)};
                }
            } else { c.kind = RMC_FLOW_PASS; }
            if (c.kind == RMC_FLOW_PASS) {
                const RmcPlayerId recipient = pass_recipient(f, own, p->controller);
                if (recipient.value == 0) { return RMC_FLOW_INVALID_STATE; }
                c.data.pass = (RmcPassAction){own, p->controller, recipient, defender, 0, clipped(8, n.remaining_seconds)};
            }
        }
    }
    *destination = c;
    return RMC_FLOW_OK;
}
