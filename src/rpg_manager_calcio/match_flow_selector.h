#ifndef RMC_MATCH_FLOW_SELECTOR_H
#define RMC_MATCH_FLOW_SELECTOR_H

#include "match_flow.h"

/* Pure versioned selector: immutable config selects v1 (minimum recipient) or
 * v2 (successor of the team's latest accepted PASS recipient, with wrap).
 * No allocation, RNG or history mutation; writes only destination on success. Instructions
 * require a manager/default submission; select returns WRONG_OPERATION there.
 * The supplied classification must still match current observations.
 */
RmcFlowError rmc_match_flow_select(
    const RmcMatchFlow *flow, const RmcMatchFlowNext *next,
    RmcMatchFlowCommand *destination);

#endif
