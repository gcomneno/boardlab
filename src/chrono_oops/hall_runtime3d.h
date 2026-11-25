#ifndef CHRONO_HALL_RUNTIME3D_H
#define CHRONO_HALL_RUNTIME3D_H

#include "hall_store3d.h"

typedef struct {
    ChronoHall hall;
    char path[1024];
    bool ready;
    bool saved;
    bool save_error;
    ChronoHallRanks last_ranks;
} ChronoHallSession;

void chrono_hall_session_open(ChronoHallSession *session);

bool chrono_hall_session_qualifies(
    ChronoHallSession *session,
    bool hard_mode, ChronoPlan plan, unsigned score
);

bool chrono_hall_session_record(
    ChronoHallSession *session,
    bool hard_mode, ChronoPlan plan,
    const char *initials, unsigned score
);

#endif
