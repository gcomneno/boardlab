#ifndef CHRONO_HALL_STORE3D_H
#define CHRONO_HALL_STORE3D_H

#include "hall3d.h"

typedef enum {
    CHRONO_HALL_STORE_OK,
    CHRONO_HALL_STORE_NOT_FOUND,
    CHRONO_HALL_STORE_CORRUPT,
    CHRONO_HALL_STORE_IO_ERROR
} ChronoHallStoreResult;

ChronoHallStoreResult chrono_hall_store_load(
    const char *path, ChronoHall *hall
);

ChronoHallStoreResult chrono_hall_store_save(
    const char *path, const ChronoHall *hall
);

#endif
