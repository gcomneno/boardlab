#ifndef RMC_PROGRESS_STATE_H
#define RMC_PROGRESS_STATE_H

#include <stdint.h>

enum {
    RMC_TALENT_COUNT = 8,
    RMC_MAX_CONSOLIDATED_TALENTS = 3,
    RMC_MAX_SEASON_CONSOLIDATIONS = 2,
    RMC_MAX_ATTRIBUTE_SEASON_GAIN = 4,
    RMC_MAX_PLAYER_SEASON_GAIN = 12
};

typedef enum {
    RMC_TALENT_INSPIRED_PASS = 0,
    RMC_TALENT_INTERCEPTOR,
    RMC_TALENT_COMPOSURE,
    RMC_TALENT_FIRST_STEP,
    RMC_TALENT_NATURAL_PLAYMAKER,
    RMC_TALENT_SECOND_WIND,
    RMC_TALENT_CAPTAIN,
    RMC_TALENT_MENTOR
} RmcTalent;

/*
 * Persistent per-attribute training data.
 *
 * General attribute indices: 0..15.
 * Goalkeeper attribute indices: 0..3.
 *
 * Progress thresholds and application rules belong
 * to the RPG engine, not to this data structure.
 */
typedef struct {
    uint32_t general_progress[16];
    uint32_t goalkeeper_progress[4];

    uint8_t general_season_gain[16];
    uint8_t goalkeeper_season_gain[4];

    uint8_t total_season_gain;
} RmcTrainingState;

/*
 * Bit i identifies talent i in the fixed M001 catalog.
 *
 * consolidated_mask: talents already acquired.
 * matured_mask: talents eligible for consolidation.
 * candidate_progress: persistent maturation progress.
 *
 * Season consolidation count is distinct from the
 * total number of consolidated talents.
 */
typedef struct {
    uint8_t consolidated_mask;
    uint8_t matured_mask;
    uint32_t candidate_progress[RMC_TALENT_COUNT];
    uint8_t season_consolidations;
} RmcTalentState;

#endif
