#ifndef RMC_MATCH_FLOW_SELECTOR_H
#define RMC_MATCH_FLOW_SELECTOR_H

#include "match_flow.h"

/* Pure Selector v1: no RNG, writes only destination on success. Instructions
 * require a manager/default submission; select returns WRONG_OPERATION there.
 * The supplied classification must still match current observations.
 */
RmcFlowError rmc_match_flow_select(
    const RmcMatchFlow *flow, const RmcMatchFlowNext *next,
    RmcMatchFlowCommand *destination);

#endif
