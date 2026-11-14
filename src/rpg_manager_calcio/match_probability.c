#include "match_probability.h"

#include <stddef.h>

/*
 * Canonical generation recipe, version 1:
 *
 * For each integer D in [-100,100], evaluate with
 * sufficiently precise decimal arithmetic:
 *
 *   10000 / (1 + exp(-D / 16))
 *
 * Round to the nearest integer, ties upward.
 *
 * Symmetry P(-D) = 10000 - P(D) is frozen in the
 * resulting table. The table is never regenerated
 * or evaluated with floating-point at runtime.
 *
 * Index i corresponds to D = i - 100.
 */
static const uint16_t logistic_table[RMC_LOGISTIC_TABLE_SIZE] = {
       19,    21,    22,    23,    25,    26,    28,    30,    32,    34,
       36,    38,    41,    43,    46,    49,    52,    56,    59,    63,
       67,    71,    76,    81,    86,    91,    97,   103,   110,   117,
      124,   132,   141,   150,   159,   169,   180,   191,   203,   216,
      230,   244,   260,   276,   293,   311,   331,   351,   373,   396,
      421,   447,   474,   503,   534,   567,   601,   637,   675,   716,
      759,   804,   851,   901,   953,  1009,  1067,  1128,  1192,  1259,
     1330,  1403,  1480,  1561,  1645,  1733,  1824,  1919,  2018,  2121,
     2227,  2337,  2451,  2568,  2689,  2814,  2942,  3074,  3208,  3346,
     3486,  3630,  3775,  3923,  4073,  4225,  4378,  4533,  4688,  4844,
     5000,  5156,  5312,  5467,  5622,  5775,  5927,  6077,  6225,  6370,
     6514,  6654,  6792,  6926,  7058,  7186,  7311,  7432,  7549,  7663,
     7773,  7879,  7982,  8081,  8176,  8267,  8355,  8439,  8520,  8597,
     8670,  8741,  8808,  8872,  8933,  8991,  9047,  9099,  9149,  9196,
     9241,  9284,  9325,  9363,  9399,  9433,  9466,  9497,  9526,  9553,
     9579,  9604,  9627,  9649,  9669,  9689,  9707,  9724,  9740,  9756,
     9770,  9784,  9797,  9809,  9820,  9831,  9841,  9850,  9859,  9868,
     9876,  9883,  9890,  9897,  9903,  9909,  9914,  9919,  9924,  9929,
     9933,  9937,  9941,  9944,  9948,  9951,  9954,  9957,  9959,  9962,
     9964,  9966,  9968,  9970,  9972,  9974,  9975,  9977,  9978,  9979,
     9981
};

_Static_assert(
    sizeof(logistic_table) / sizeof(logistic_table[0]) ==
        RMC_LOGISTIC_TABLE_SIZE,
    "Canonical logistic table must contain 201 entries"
);

bool rmc_match_logistic_lookup(
    int32_t difference,
    uint32_t *probability)
{
    if (probability == NULL ||
        difference < -100 || difference > 100) {
        return false;
    }

    *probability = logistic_table[(size_t)(difference + 100)];
    return true;
}

bool rmc_match_probability(
    int32_t attack,
    int32_t opposition,
    int32_t context,
    uint32_t *probability)
{
    if (probability == NULL ||
        attack < 0 || attack > 100 ||
        opposition < 0 || opposition > 100 ||
        context < -20 || context > 20) {
        return false;
    }

    int32_t difference = attack - opposition + context;

    if (difference < -100) {
        difference = -100;
    } else if (difference > 100) {
        difference = 100;
    }

    return rmc_match_logistic_lookup(difference, probability);
}
