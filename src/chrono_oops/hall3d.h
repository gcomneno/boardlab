#ifndef CHRONO_HALL3D_H
#define CHRONO_HALL3D_H

#include <stdbool.h>
#include "chrono.h"

enum {
    CHRONO_HALL_CAPACITY = 10,
    CHRONO_HALL_CATEGORIES = 4
};

typedef struct {
    char initials[4];
    unsigned score;
    unsigned day; /* YYYYMMDD, local calendar date */
} ChronoHallEntry;

typedef struct {
    ChronoHallEntry entries[CHRONO_HALL_CAPACITY];
    unsigned count;
} ChronoHallBoard;

typedef struct {
    unsigned daily_day;
    ChronoHallBoard daily[CHRONO_HALL_CATEGORIES];
    ChronoHallBoard all_time[CHRONO_HALL_CATEGORIES];
} ChronoHall;

typedef struct {
    unsigned daily_rank;    /* 1..10, or 0 if not ranked */
    unsigned all_time_rank; /* 1..10, or 0 if not ranked */
} ChronoHallRanks;

void chrono_hall_init(ChronoHall *hall);

/* New calendar day resets daily boards, never all-time boards. */
void chrono_hall_set_day(ChronoHall *hall, unsigned day);

/* Return 0 on success; invalid input leaves hall unchanged. */
int chrono_hall_record(
    ChronoHall *hall,
    bool hard_mode,
    ChronoPlan plan,
    unsigned day,
    const char *initials,
    unsigned score,
    ChronoHallRanks *ranks
);

#endif
