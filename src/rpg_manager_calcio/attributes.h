#ifndef RMC_ATTRIBUTES_H
#define RMC_ATTRIBUTES_H

/*
 * Canonical persistent attribute indices.
 *
 * Contract: M001, section 3.2.
 *
 * Do not reorder existing indices without an explicit
 * versioned compatibility decision.
 */

typedef enum {
    RMC_ATTR_PASSAGGIO = 0,
    RMC_ATTR_CONTROLLO = 1,
    RMC_ATTR_DRIBBLING = 2,
    RMC_ATTR_TIRO = 3,
    RMC_ATTR_CROSS = 4,
    RMC_ATTR_CONTRASTO = 5,

    RMC_ATTR_VELOCITA = 6,
    RMC_ATTR_RESISTENZA = 7,
    RMC_ATTR_FORZA = 8,

    RMC_ATTR_POSIZIONAMENTO = 9,
    RMC_ATTR_VISIONE = 10,
    RMC_ATTR_MARCATURA = 11,

    RMC_ATTR_DISCIPLINA = 12,
    RMC_ATTR_LEADERSHIP = 13,
    RMC_ATTR_DETERMINAZIONE = 14,
    RMC_ATTR_COLLABORAZIONE = 15,

    RMC_ATTR_COUNT = 16
} RmcAttribute;

typedef enum {
    RMC_GK_ATTR_RIFLESSI = 0,
    RMC_GK_ATTR_PRESA = 1,
    RMC_GK_ATTR_USCITE = 2,
    RMC_GK_ATTR_POSIZIONAMENTO = 3,

    RMC_GK_ATTR_COUNT = 4
} RmcGoalkeeperAttribute;

/*
 * ISO C11 compile-time contract checks.
 */
_Static_assert(RMC_ATTR_PASSAGGIO == 0,
               "Attribute index mismatch");
_Static_assert(RMC_ATTR_COLLABORAZIONE == 15,
               "Attribute index mismatch");
_Static_assert(RMC_ATTR_COUNT == 16,
               "General attribute count mismatch");

_Static_assert(RMC_GK_ATTR_RIFLESSI == 0,
               "Goalkeeper attribute index mismatch");
_Static_assert(RMC_GK_ATTR_POSIZIONAMENTO == 3,
               "Goalkeeper attribute index mismatch");
_Static_assert(RMC_GK_ATTR_COUNT == 4,
               "Goalkeeper attribute count mismatch");

#endif
