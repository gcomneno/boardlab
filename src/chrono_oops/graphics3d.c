#include "action_menu3d.h"
#include "score3d.h"
#include "hall_runtime3d.h"
#include "effects3d.h"
#include "input3d.h"
#include <math.h>
#include <string.h>
#include "raylib.h"
#include "chrono.h"
#include "presentation3d.h"




static void village_tree(float x, float z, float scale)
{
    const Color bark = {111, 74, 49, 255};
    const Color foliage = {62, 119, 76, 255};
    const Color foliage_light = {91, 153, 91, 255};

    /* Raised crown leaves the trunk clearly visible. */
    DrawCylinder(
        (Vector3){x, 0.67f * scale, z},
        0.17f * scale, 0.13f * scale, 1.34f * scale,
        10, bark
    );

    DrawSphere(
        (Vector3){x, 1.88f * scale, z},
        0.67f * scale, foliage
    );

    DrawSphere(
        (Vector3){x - 0.19f * scale, 2.15f * scale,
                  z - 0.13f * scale},
        0.38f * scale, foliage_light
    );
}

static void village_lamp(float x, float z)
{
    DrawCylinder(
        (Vector3){x, 0.9f, z},
        0.055f, 0.055f, 1.8f, 8,
        (Color){58, 60, 67, 255}
    );
    DrawSphere(
        (Vector3){x, 1.92f, z},
        0.20f, (Color){255, 224, 149, 255}
    );
}



static void diorama_1985(
    const Model *municipality, const Model *archive,
    const Model *clerk,
    float character_x, float character_bob,
    float character_yaw
)
{
    const float x = -8.0f;

    /* Layered miniature island: stone foundation and grass. */
    DrawCubeV(
        (Vector3){x, -0.52f, 0.0f},
        (Vector3){12.2f, 0.36f, 10.2f},
        (Color){91, 88, 79, 255}
    );
    DrawCubeV(
        (Vector3){x, -0.28f, 0.0f},
        (Vector3){12.0f, 0.50f, 10.0f},
        (Color){112, 156, 106, 255}
    );

    /* Cobblestone piazza and entrance paths. */
    DrawCubeV(
        (Vector3){x, 0.015f, 1.25f},
        (Vector3){9.7f, 0.055f, 3.3f},
        (Color){190, 176, 153, 255}
    );
    DrawCubeV(
        (Vector3){x, 0.022f, 3.7f},
        (Vector3){1.75f, 0.065f, 2.5f},
        (Color){202, 189, 164, 255}
    );

    /* Soft contact shadows anchor buildings to the piazza. */

    /* External glTF municipality, positioned on the existing piazza. */
    DrawModel(
        *municipality,
        (Vector3){x - 2.5f, 0.0f, -1.4f},
        1.0f,
        WHITE
    );

    /* Dedicated 1985 archive asset. */
    DrawModel(
        *archive,
        (Vector3){x + 2.5f, 0.0f, -1.4f},
        1.0f, WHITE
    );

    /* Street furniture, greenery and a central planter. */
    village_tree(x - 5.0f, 3.45f, 1.05f);
    village_tree(x + 5.0f, 3.40f, 0.90f);
    village_tree(x + 5.2f, -3.3f, 0.85f);
    village_lamp(x - 4.35f, 0.9f);
    village_lamp(x + 4.45f, 0.9f);

    DrawCubeV(
        (Vector3){x, 0.14f, 2.25f},
        (Vector3){1.8f, 0.28f, 1.10f},
        (Color){160, 123, 92, 255}
    );
    DrawCubeV(
        (Vector3){x, 0.30f, 2.25f},
        (Vector3){1.55f, 0.08f, 0.90f},
        (Color){73, 119, 73, 255}
    );
    DrawSphere(
        (Vector3){x, 0.52f, 2.25f},
        0.27f, (Color){91, 151, 87, 255}
    );

    /* Animated display position; destination remains C11-authoritative. */
    DrawModelEx(
        *clerk,
        (Vector3){character_x, character_bob, 1.05f},
        (Vector3){0.0f, 1.0f, 0.0f},
        character_yaw,
        (Vector3){0.75f, 0.75f, 0.75f},
        WHITE
    );
}



/* Physical action props for the 1985 character.
 * The engine decides outcomes; this code only illustrates accepted actions.
 */
static void draw_1985_action_props(
    ChronoAction action, float elapsed, float character_x
)
{
    const float t = fminf(elapsed / 2.4f, 1.0f);
    const float presence = sinf(t * PI);
    const float x = character_x;
    const float z = 1.82f;
    const Color paper = {244, 233, 195, 255};
    const Color ink = {53, 83, 111, 255};
    const Color wood = {126, 84, 56, 255};

    switch (action) {
    case CHRONO_ACTION_CHECK_HOROSCOPE: {
        /* A ridiculous 1985 temporal horoscope apparatus. */
        const float height = 1.65f + 0.13f * presence;
        const Vector3 center = {x, height, z};
        DrawSphere(center, 0.23f, (Color){179, 132, 238, 255});
        DrawSphere(
            (Vector3){x - 0.055f, height + 0.075f, z + 0.17f},
            0.075f, (Color){239, 222, 255, 255}
        );
        for (int i = 0; i < 3; ++i) {
            const float angle = t * 9.0f + (float)i * 2.0944f;
            const Vector3 orbit = {
                x + 0.41f * cosf(angle),
                height + 0.15f * sinf(angle * 1.7f),
                z + 0.32f * sinf(angle)
            };
            DrawSphere(
                orbit, 0.055f,
                (Color){255, 230, 121, 255}
            );
            DrawLine3D(center, orbit, (Color){143, 114, 196, 255});
        }
        DrawCubeV(
            (Vector3){x, 1.24f, z},
            (Vector3){0.34f, 0.07f, 0.34f}, wood
        );
        break;
    }
    case CHRONO_ACTION_SELECT_BRIDGE:
    case CHRONO_ACTION_SELECT_DUCK: {
        /* A blueprint unfolds and displays the chosen miniature. */
        const float unfold = fminf(t / 0.28f, 1.0f);
        DrawCubeV(
            (Vector3){x, 1.28f, z},
            (Vector3){1.05f * unfold, 0.035f, 0.75f},
            (Color){90, 155, 193, 255}
        );
        if (action == CHRONO_ACTION_SELECT_BRIDGE) {
            for (int i = -2; i <= 2; ++i) {
                DrawCubeV(
                    (Vector3){x + (float)i * 0.15f,
                              1.35f + presence * 0.10f, z},
                    (Vector3){0.13f, 0.10f, 0.42f},
                    wood
                );
            }
        } else {
            DrawSphere(
                (Vector3){x, 1.53f + presence * 0.10f, z},
                0.24f, GOLD
            );
            DrawSphere(
                (Vector3){x + 0.18f, 1.70f + presence * 0.10f, z},
                0.14f, GOLD
            );
            DrawCubeV(
                (Vector3){x + 0.32f, 1.67f, z},
                (Vector3){0.17f, 0.07f, 0.14f},
                ORANGE
            );
        }
        break;
    }

    case CHRONO_ACTION_APPROVE_BRIDGE:
    case CHRONO_ACTION_APPROVE_DUCK: {
        /* A heavy approval stamp comes down on the signed sheet. */
        const float strike = fminf(
            fmaxf((t - 0.24f) / 0.24f, 0.0f), 1.0f
        );
        const float rebound = fminf(
            fmaxf((t - 0.56f) / 0.18f, 0.0f), 1.0f
        );
        const float press = strike * (1.0f - rebound);
        DrawCubeV(
            (Vector3){x, 1.18f, z},
            (Vector3){0.94f, 0.035f, 0.69f}, paper
        );
        DrawCylinder(
            (Vector3){x, 2.03f - press * 0.63f, z},
            0.12f, 0.20f, 0.48f, 12, wood
        );
        DrawCylinder(
            (Vector3){x, 1.74f - press * 0.63f, z},
            0.27f, 0.27f, 0.12f, 12, ink
        );
        if (t > 0.49f) {
            DrawCylinder(
                (Vector3){x, 1.21f, z},
                0.22f, 0.22f, 0.012f, 18,
                (Color){181, 59, 54, 255}
            );
        }
        break;
    }

    case CHRONO_ACTION_VERIFY_PLAN:
    case CHRONO_ACTION_VERIFY_INTERVENTION: {
        /* A checklist is inspected with a moving magnifying glass. */
        DrawCubeV(
            (Vector3){x, 1.27f, z},
            (Vector3){0.91f, 0.045f, 0.73f}, paper
        );
        for (int i = 0; i < 3; ++i) {
            const float zz = z - 0.19f + 0.19f * (float)i;
            DrawCubeV(
                (Vector3){x - 0.03f, 1.30f, zz},
                (Vector3){0.50f, 0.009f, 0.025f}, ink
            );
            if (t > 0.25f + 0.18f * (float)i) {
                DrawCubeV(
                    (Vector3){x - 0.36f, 1.31f, zz},
                    (Vector3){0.11f, 0.025f, 0.09f},
                    (Color){70, 163, 107, 255}
                );
            }
        }
        const Vector3 lens = {
            x + 0.24f * sinf(t * 6.0f),
            1.57f,
            z + 0.16f * cosf(t * 6.0f)
        };
        DrawCircle3D(
            lens, 0.23f,
            (Vector3){1.0f, 0.0f, 0.0f},
            90.0f, ink
        );
        DrawLine3D(
            (Vector3){lens.x + 0.17f, lens.y, lens.z},
            (Vector3){lens.x + 0.38f, lens.y - 0.24f, lens.z},
            wood
        );
        break;
    }

    case CHRONO_ACTION_READ_MANUAL: {
        /* An opened book with pages lifting one after another. */
        DrawCubeV(
            (Vector3){x - 0.25f, 1.36f, z},
            (Vector3){0.48f, 0.055f, 0.67f}, paper
        );
        DrawCubeV(
            (Vector3){x + 0.25f, 1.36f, z},
            (Vector3){0.48f, 0.055f, 0.67f}, paper
        );
        DrawCubeV(
            (Vector3){x, 1.32f, z},
            (Vector3){0.035f, 0.09f, 0.70f}, wood
        );
        for (int i = 0; i < 3; ++i) {
            const float lift = fmaxf(
                0.0f, sinf((t * 3.0f - (float)i * 0.55f) * PI)
            );
            DrawCubeV(
                (Vector3){x + 0.22f - 0.31f * lift,
                          1.43f + lift * 0.34f, z},
                (Vector3){0.42f, 0.018f, 0.59f}, paper
            );
        }
        break;
    }

    case CHRONO_ACTION_CALIBRATE_DUCK: {
        /* Turn a calibration dial while the indicator sweeps. */
        DrawCubeV(
            (Vector3){x, 1.33f, z},
            (Vector3){0.85f, 0.38f, 0.58f},
            (Color){86, 111, 124, 255}
        );
        DrawCylinder(
            (Vector3){x, 1.59f, z},
            0.30f, 0.30f, 0.035f, 24, paper
        );
        const float angle = -1.2f + 2.4f * t;
        DrawLine3D(
            (Vector3){x, 1.62f, z},
            (Vector3){x + 0.21f * sinf(angle),
                      1.64f, z + 0.21f * cosf(angle)},
            RED
        );
        DrawSphere(
            (Vector3){x + 0.34f, 1.61f + 0.05f * sinf(t * 8.0f), z},
            0.11f, GOLD
        );
        break;
    }

    default:
        break;
    }
}

static void river_reeds(float x, float z)
{
    for (int i = 0; i < 3; ++i) {
        const float dx = (float)i * 0.17f;
        DrawCylinder(
            (Vector3){x + dx, 0.32f, z},
            0.035f, 0.025f, 0.65f, 6,
            (Color){79, 132, 74, 255}
        );
        DrawSphere(
            (Vector3){x + dx, 0.70f, z},
            0.075f, (Color){126, 102, 64, 255}
        );
    }
}



static void diorama_2025(
    const Chrono3DScene *scene, const Model *tower,
    const Model *explorer, float bridge_progress,
    float inspect_progress, ChronoAction action,
    float motion_progress, ChronoLocation previous_location
)
{
    const float x = 8.0f;

    /* Stone-edged floating island and grassy banks. */
    DrawCubeV(
        (Vector3){x, -0.54f, 0.0f},
        (Vector3){12.2f, 0.36f, 10.2f},
        (Color){82, 89, 91, 255}
    );
    DrawCubeV(
        (Vector3){x, -0.28f, 0.0f},
        (Vector3){12.0f, 0.50f, 10.0f},
        (Color){112, 153, 105, 255}
    );

    /* Meandering river: triangulated surface, not a rectangular slab. */
    for (int i = 0; i < 16; ++i) {
        const float z0 = -4.8f + (float)i * 0.6f;
        const float z1 = z0 + 0.6f;

        const float s0 = sinf(z0 * 0.72f) * 0.24f;
        const float s1 = sinf(z1 * 0.72f) * 0.24f;

        const float w0 = 1.43f + 0.11f * cosf(z0 * 0.85f);
        const float w1 = 1.43f + 0.11f * cosf(z1 * 0.85f);

        const Vector3 a = {x + s0 - w0, 0.085f, z0};
        const Vector3 b = {x + s0 + w0, 0.085f, z0};
        const Vector3 c = {x + s1 - w1, 0.085f, z1};
        const Vector3 d = {x + s1 + w1, 0.085f, z1};

        const Color water = (i % 3 == 0)
            ? (Color){62, 151, 190, 255}
            : (Color){72, 162, 195, 255};

        DrawTriangle3D(a, c, b, water);
        DrawTriangle3D(b, c, d, water);

        /* Narrow shoreline strips follow the same changing banks. */
        const Vector3 al = {a.x - 0.19f, 0.092f, z0};
        const Vector3 cl = {c.x - 0.19f, 0.092f, z1};
        const Vector3 br = {b.x + 0.19f, 0.092f, z0};
        const Vector3 dr = {d.x + 0.19f, 0.092f, z1};

        const Color shore = {168, 174, 124, 255};
        DrawTriangle3D(al, cl, a, shore);
        DrawTriangle3D(a, cl, c, shore);
        DrawTriangle3D(b, d, br, shore);
        DrawTriangle3D(br, d, dr, shore);
    }

    /* Subtle surface highlights. */
    for (int i = 0; i < 9; ++i) {
        const float z = -3.9f + (float)i * 0.95f;
        const float center = x + sinf(z * 0.72f) * 0.24f;

        DrawLine3D(
            (Vector3){center - 0.55f, 0.103f, z},
            (Vector3){center + 0.15f, 0.103f, z},
            (Color){151, 212, 218, 255}
        );
    }

    /* Paths approaching the water and the tower. */
    DrawCubeV(
        (Vector3){x - 3.6f, 0.025f, 1.25f},
        (Vector3){2.75f, 0.055f, 2.0f},
        (Color){194, 177, 143, 255}
    );
    DrawCubeV(
        (Vector3){x + 3.6f, 0.025f, 1.25f},
        (Vector3){2.7f, 0.055f, 2.0f},
        (Color){187, 177, 159, 255}
    );


    /* Dedicated 2025 tower asset. */
    DrawModel(
        *tower,
        (Vector3){x + 3.75f, 0.0f, -1.5f},
        1.0f, WHITE
    );

    /* Reeds, rocks, trees and shoreline vegetation. */
    for (int i = 0; i < 4; ++i) {
        float z = -3.2f + (float)i * 2.1f;
        river_reeds(x - 1.8f, z);
        river_reeds(x + 1.7f, z + 0.5f);
    }

    village_tree(x - 4.9f, -3.25f, 0.85f);
    village_tree(x - 4.85f, 3.75f, 0.72f);
    village_tree(x + 5.0f, 3.65f, 0.77f);

    DrawSphere(
        (Vector3){x - 2.05f, 0.13f, -1.5f},
        0.34f, (Color){123, 130, 126, 255}
    );
    DrawSphere(
        (Vector3){x + 2.08f, 0.12f, 3.6f},
        0.25f, (Color){146, 144, 133, 255}
    );

    /* The 1985 Duck project replaces the old ruins in 2025.
       Before approval, both broken abutments remain visible. */
    const Color old_stone = {125, 119, 106, 255};
    const Color broken_stone = {157, 148, 128, 255};

    if (!scene->world.duck_exists) {
    for (int side = 0; side < 2; ++side) {
        const float bank_x = side == 0 ? 5.92f : 10.08f;
        const float z = -0.65f;

        DrawCubeV(
            (Vector3){bank_x, 0.24f, z},
            (Vector3){0.96f, 0.46f, 1.25f}, old_stone
        );
        DrawCubeV(
            (Vector3){bank_x - 0.22f, 0.69f, z - 0.24f},
            (Vector3){0.39f, 0.48f, 0.48f}, broken_stone
        );
        DrawCubeV(
            (Vector3){bank_x + 0.25f, 0.53f, z + 0.28f},
            (Vector3){0.34f, 0.23f, 0.38f}, old_stone
        );

        /* Scattered masonry along each bank. */
        DrawCubeV(
            (Vector3){bank_x - 0.72f, 0.12f, z + 0.63f},
            (Vector3){0.37f, 0.20f, 0.31f}, broken_stone
        );
        DrawCubeV(
            (Vector3){bank_x + 0.54f, 0.10f, z - 0.66f},
            (Vector3){0.30f, 0.16f, 0.44f}, old_stone
        );
    }
    }

    /* Only authoritative world events materialize these objects. */
    if (scene->world.bridge_exists) {
        const float rise = (1.0f - bridge_progress) * 1.2f;
        DrawCubeV(
            (Vector3){x, 0.57f - rise, -0.65f},
            (Vector3){5.15f, 0.30f, 1.75f},
            (Color){155, 108, 72, 255}
        );
        for (int i = 0; i < 5; ++i) {
            float px = x - 2.0f + (float)i;
            DrawCubeV(
                (Vector3){px, 0.96f - rise, -1.50f},
                (Vector3){0.14f, 0.76f, 0.14f},
                (Color){111, 73, 53, 255}
            );
        }
    }

    if (scene->world.duck_exists) {
        /*
         * The 1985 experimental duck survives as a commemorative
         * machine on the 2025 lawn. Its world-state remains C11-owned.
         */
        /*
         * The duck itself navigates during CROSS_RIVER.
         * After arrival it remains docked on the eastern bank.
         */
        float dx = x - 2.5f;
        const float dz = -0.65f;
        const bool running = scene->world.duck_running;
        if (running &&
            scene->location_b >= CHRONO_LOCATION_EAST_BANK) {
            dx = 10.5f;
            if (action == CHRONO_ACTION_CROSS_RIVER &&
                motion_progress >= 0.0f) {
                /* Same normalized timeline used by the passenger. */
                const float walk = fminf(fmaxf(
                    (motion_progress - 0.12f) / 0.76f,
                    0.0f), 1.0f);
                const float eased =
                    walk * walk * (3.0f - 2.0f * walk);
                float sail = fminf(fmaxf(
                    (eased - 0.22f) / 0.53f,
                    0.0f), 1.0f);
                sail = sail * sail * (3.0f - 2.0f * sail);
                dx = 5.5f + 5.0f * sail;
            }
        }
        const Color plumage = running
            ? (Color){255, 229, 88, 255}
            : (Color){232, 184, 76, 255};
        const Color wing = running
            ? (Color){255, 244, 161, 255}
            : (Color){247, 211, 117, 255};
        const Color metal = {85, 107, 117, 255};
        const Color bill = {248, 140, 55, 255};
        const Color dark = {29, 42, 53, 255};

        /* Small museum-style plinth and two pontoon supports. */
        DrawCubeV(
            (Vector3){dx, 0.09f, dz},
            (Vector3){1.83f, 0.17f, 1.40f}, metal
        );
        DrawCubeV(
            (Vector3){dx, 0.19f, dz + 0.61f},
            (Vector3){0.92f, 0.16f, 0.08f},
            (Color){183, 160, 103, 255}
        );
        for (int side = -1; side <= 1; side += 2) {
            DrawCubeV(
                (Vector3){dx, 0.29f, dz + (float)side * 0.36f},
                (Vector3){1.30f, 0.17f, 0.21f},
                (Color){124, 143, 149, 255}
            );
        }

        /* Distinct duck silhouette: breast, raised head and tail. */
        DrawSphere((Vector3){dx, 0.76f, dz}, 0.54f, plumage);
        DrawSphere(
            (Vector3){dx + 0.30f, 1.23f, dz},
            0.32f, plumage
        );
        DrawSphere(
            (Vector3){dx - 0.48f, 0.91f, dz},
            0.24f, wing
        );
        DrawSphere(
            (Vector3){dx - 0.63f, 1.08f, dz},
            0.17f, wing
        );

        /* Both wings remain readable from the isometric camera. */
        for (int side = -1; side <= 1; side += 2) {
            float wz = dz + (float)side * 0.39f;
            DrawSphere(
                (Vector3){dx - 0.12f, 0.78f, wz},
                0.29f, wing
            );
            DrawLine3D(
                (Vector3){dx - 0.38f, 0.80f, wz},
                (Vector3){dx + 0.10f, 0.70f, wz},
                metal
            );
        }

        /* Orange beak points toward the river crossing. */
        DrawCubeV(
            (Vector3){dx + 0.66f, 1.18f, dz},
            (Vector3){0.46f, 0.13f, 0.37f}, bill
        );
        DrawCubeV(
            (Vector3){dx + 0.58f, 1.09f, dz},
            (Vector3){0.31f, 0.055f, 0.31f},
            (Color){184, 91, 40, 255}
        );

        /* High-contrast eyes on both sides of the head. */
        for (int side = -1; side <= 1; side += 2) {
            float ez = dz + (float)side * 0.255f;
            DrawSphere(
                (Vector3){dx + 0.41f, 1.34f, ez},
                0.105f, RAYWHITE
            );
            DrawSphere(
                (Vector3){dx + 0.45f, 1.34f,
                          ez + (float)side * 0.082f},
                0.055f, dark
            );
        }

        /* Visible retro-industrial fittings on the pedestal. */
        DrawCubeV(
            (Vector3){dx - 0.42f, 0.18f, dz + 0.666f},
            (Vector3){0.09f, 0.07f, 0.02f}, dark
        );
        DrawCubeV(
            (Vector3){dx + 0.42f, 0.18f, dz + 0.666f},
            (Vector3){0.09f, 0.07f, 0.02f},
            running ? LIME : dark
        );
    }

    /* Canonical 2025 locations, aligned with the physical bridge. */
    const Vector3 west_bank = {4.5f, 0.0f, 2.6f};
    const Vector3 bridge_center = {8.0f, 0.72f, -0.65f};
    const Vector3 east_bank = {11.0f, 0.0f, 2.6f};
    const Vector3 tower_approach = {11.75f, 0.0f, -0.35f};
    const Vector3 tower_interior = {11.55f, 0.0f, -1.20f};

    Vector3 player_position = west_bank;
    if (scene->location_b == CHRONO_LOCATION_BRIDGE) {
        player_position = bridge_center;
    } else if (scene->location_b == CHRONO_LOCATION_TOWER) {
        player_position = scene->tower_entered
            ? tower_interior : tower_approach;
    } else if (scene->location_b >= CHRONO_LOCATION_EAST_BANK) {
        player_position = east_bank;
    }

    const float player_x = player_position.x;
    float explorer_x = player_position.x;
    float explorer_z = player_position.z;
    float explorer_height =
        scene->location_b == CHRONO_LOCATION_BRIDGE
            ? bridge_center.y : 0.0f;
    float explorer_pitch = 0.0f;
    float explorer_yaw = 0.0f;
    float explorer_bob = 0.0f;

    if (motion_progress >= 0.0f) {
        const float t = motion_progress;
        if (action == CHRONO_ACTION_CROSS_BRIDGE ||
            action == CHRONO_ACTION_CROSS_RIVER ||
            action == CHRONO_ACTION_REACH_TOWER) {
            const float walk = fminf(
                fmaxf((t - 0.12f) / 0.76f, 0.0f), 1.0f
            );
            const float eased = walk * walk * (3.0f - 2.0f * walk);

            if (action == CHRONO_ACTION_CROSS_BRIDGE) {
                /* Approach the western bridge entrance before crossing. */
                const float approach = fminf(eased / 0.38f, 1.0f);
                const float crossing = fmaxf(
                    (eased - 0.38f) / 0.62f, 0.0f
                );
                explorer_x = west_bank.x +
                    (5.45f - west_bank.x) * approach +
                    (bridge_center.x - 5.45f) * crossing;
                explorer_z = west_bank.z +
                    (bridge_center.z - west_bank.z) * approach;
                explorer_height = bridge_center.y * crossing;
            } else if (action == CHRONO_ACTION_REACH_TOWER) {
                /* Finish crossing the deck, then approach the tower. */
                const float crossing = fminf(eased / 0.72f, 1.0f);
                const float approach = fmaxf(
                    (eased - 0.72f) / 0.28f, 0.0f
                );
                explorer_x = bridge_center.x +
                    (10.55f - bridge_center.x) * crossing +
                    (tower_approach.x - 10.55f) * approach;
                explorer_z = bridge_center.z +
                    (tower_approach.z - bridge_center.z) * approach;
                explorer_height =
                    bridge_center.y * (1.0f - approach);
            } else {
                /* Board, ride the duck across, then disembark. */
                float board = fminf(eased / 0.22f, 1.0f);
                float sail = (eased - 0.22f) / 0.53f;
                float disembark = (eased - 0.75f) / 0.25f;

                board = fminf(fmaxf(board, 0.0f), 1.0f);
                sail = fminf(fmaxf(sail, 0.0f), 1.0f);
                disembark =
                    fminf(fmaxf(disembark, 0.0f), 1.0f);

                sail = sail * sail * (3.0f - 2.0f * sail);
                disembark = disembark * disembark *
                    (3.0f - 2.0f * disembark);

                const float duck_x = 5.5f + 5.0f * sail;
                const float duck_z = -0.65f;

                explorer_x = west_bank.x +
                    (duck_x - west_bank.x) * board;
                explorer_z = west_bank.z +
                    (duck_z - west_bank.z) * board;
                explorer_height = 1.05f * board;

                explorer_x +=
                    (east_bank.x - explorer_x) * disembark;
                explorer_z +=
                    (east_bank.z - explorer_z) * disembark;
                explorer_height *= 1.0f - disembark;
            }

            const float facing = fminf(t / 0.12f, 1.0f);
            const float settle = fminf(
                fmaxf((t - 0.88f) / 0.12f, 0.0f), 1.0f
            );
            explorer_yaw = 90.0f * facing * (1.0f - settle);
            explorer_bob = 0.085f *
                fabsf(sinf(walk * 18.0f * PI)) * sinf(walk * PI);
        } else if (action == CHRONO_ACTION_MEASURE_RIVER) {
            /* Walk to the western bank, measure, then step back. */
            float approach = fminf(t / 0.34f, 1.0f);
            approach = approach * approach *
                (3.0f - 2.0f * approach);

            float retreat = fminf(
                fmaxf((t - 0.76f) / 0.24f, 0.0f), 1.0f
            );
            retreat = retreat * retreat *
                (3.0f - 2.0f * retreat);

            const float proximity = approach * (1.0f - retreat);
            explorer_x = player_x + 1.65f * proximity;
            explorer_yaw = 90.0f * proximity;
            explorer_bob = 0.065f *
                fabsf(sinf(t * 16.0f * PI)) *
                (1.0f - proximity);
        } else if (action == CHRONO_ACTION_ENTER_TOWER) {
            /* Approach the door, then enter the tower. */
            const Vector3 start =
                previous_location == CHRONO_LOCATION_EAST_BANK
                    ? east_bank : tower_approach;
            float approach = fminf(
                fmaxf(t / 0.50f, 0.0f), 1.0f
            );
            float enter = fminf(
                fmaxf((t - 0.50f) / 0.50f, 0.0f), 1.0f
            );
            approach = approach * approach *
                (3.0f - 2.0f * approach);
            enter = enter * enter *
                (3.0f - 2.0f * enter);
            explorer_x = start.x +
                (tower_approach.x - start.x) * approach +
                (tower_interior.x - tower_approach.x) * enter;
            explorer_z = start.z +
                (tower_approach.z - start.z) * approach +
                (tower_interior.z - tower_approach.z) * enter;
            explorer_yaw = -35.0f * sinf(t * PI);
        }
    }

    if (inspect_progress >= 0.0f &&
        inspect_progress < 1.0f) {
        const float t = inspect_progress;

        /* Approach the ruins, inspect them, then return. */
        float approach = fminf(
            fmaxf((t - 0.08f) / 0.38f, 0.0f), 1.0f
        );
        approach = approach * approach *
            (3.0f - 2.0f * approach);

        float retreat = fminf(
            fmaxf((t - 0.77f) / 0.23f, 0.0f), 1.0f
        );
        retreat = retreat * retreat *
            (3.0f - 2.0f * retreat);

        const float proximity = approach * (1.0f - retreat);

        explorer_x = player_x + 1.45f * proximity;
        explorer_z = 2.6f - 3.15f * proximity;

        float bend = fminf(
            fmaxf((t - 0.46f) / 0.13f, 0.0f), 1.0f
        );
        bend = bend * bend * (3.0f - 2.0f * bend);

        float straighten = fminf(
            fmaxf((t - 0.69f) / 0.08f, 0.0f), 1.0f
        );
        straighten = straighten * straighten *
            (3.0f - 2.0f * straighten);

        explorer_pitch = 28.0f * bend * (1.0f - straighten);
    }

    /* Locomotion turns the model; inspection retains its pitch. */
    const bool turning = fabsf(explorer_yaw) > 0.01f;
    DrawModelEx(
        *explorer,
        (Vector3){
            explorer_x, explorer_height + explorer_bob, explorer_z
        },
        turning
            ? (Vector3){0.0f, 1.0f, 0.0f}
            : (Vector3){1.0f, 0.0f, 0.0f},
        turning ? explorer_yaw : explorer_pitch,
        (Vector3){0.75f, 0.75f, 0.75f},
        WHITE
    );
}



/* Accepted actions in the 2025 scene use visible, physical props. */
static void draw_2025_action_props(
    ChronoAction action, float elapsed, const Chrono3DScene *scene
)
{
    const float t = fminf(elapsed / 2.4f, 1.0f);
    const float pulse = sinf(t * PI);
    const Color steel = {91, 112, 124, 255};
    const Color gold = {241, 196, 72, 255};
    const float west = 4.5f;

    switch (action) {
    case CHRONO_ACTION_QUESTION_PIGEON: {
        /* The witness is a pigeon, not a reliable historian. */
        const float nod = 0.11f * sinf(t * 8.0f * PI);
        const Color feathers = {145, 158, 169, 255};
        const Vector3 perch = {west + 0.55f, 0.97f, 2.0f};
        DrawCubeV(
            (Vector3){perch.x, 0.64f, perch.z},
            (Vector3){0.55f, 0.22f, 0.42f}, steel
        );
        DrawSphere(perch, 0.25f, feathers);
        DrawSphere(
            (Vector3){perch.x + 0.20f, 1.17f + nod, perch.z},
            0.16f, feathers
        );
        DrawCubeV(
            (Vector3){perch.x + 0.38f, 1.13f + nod, perch.z},
            (Vector3){0.17f, 0.075f, 0.13f}, gold
        );
        DrawSphere(
            (Vector3){perch.x + 0.26f, 1.23f + nod,
                      perch.z + 0.12f},
            0.038f, BLACK
        );
        DrawSphere(
            (Vector3){perch.x - 0.12f, 0.98f, perch.z + 0.17f},
            0.14f, (Color){112, 130, 149, 255}
        );
        /* A floating question mark follows the interrogation. */
        DrawSphere(
            (Vector3){perch.x + 0.44f,
                      1.77f + 0.09f * pulse, perch.z},
            0.09f, gold
        );
        DrawSphere(
            (Vector3){perch.x + 0.44f,
                      1.52f + 0.09f * pulse, perch.z},
            0.040f, gold
        );
        break;
    }
    case CHRONO_ACTION_THROW_STONE: {
        /* A small stone arcs from the west bank into the river. */
        const float flight = fminf(t / 0.62f, 1.0f);
        const float arc = 0.90f * sinf(flight * PI);
        const Vector3 stone = {
            5.00f + 2.85f * flight,
            0.16f + arc,
            1.65f
        };
        if (t < 0.62f) {
            DrawSphere(
                stone, 0.105f,
                (Color){125, 132, 139, 255}
            );
        } else {
            for (int i = 0; i < 3; ++i) {
                const float ripple = (t - 0.62f) * 2.0f
                    - (float)i * 0.15f;
                if (ripple > 0.0f) {
                    DrawCircle3D(
                        (Vector3){7.85f, 0.12f, 1.65f},
                        0.15f + ripple,
                        (Vector3){1.0f, 0.0f, 0.0f},
                        90.0f,
                        (Color){190, 231, 239, 255}
                    );
                }
            }
        }
        break;
    }
    case CHRONO_ACTION_KNOCK_TOWER: {
        /* Three insistent knocks on the tower's old door. */
        const float knock = fmaxf(0.0f, sinf(t * 6.0f * PI));
        const Vector3 door = {11.70f, 1.05f, -0.48f};
        DrawSphere(
            (Vector3){door.x, door.y + 0.36f,
                      door.z + 0.16f + 0.20f * knock},
            0.13f, gold
        );
        DrawCylinder(
            (Vector3){door.x, door.y + 0.20f,
                      door.z + 0.09f},
            0.035f, 0.035f, 0.22f, 10, steel
        );
        for (int i = 0; i < 3; ++i) {
            const float wave = t * 3.0f - (float)i * 0.55f;
            if (wave > 0.0f && wave < 1.0f) {
                DrawSphereWires(
                    (Vector3){door.x, door.y + 0.36f, door.z},
                    0.15f + wave * 0.48f, 8, 12,
                    (Color){255, 208, 110, 255}
                );
            }
        }
        break;
    }
    case CHRONO_ACTION_IDENTIFY_DEFECT: {
        /* Defective stone: magnifier and highlighted crack. */
        const Vector3 center = {5.9f, 0.48f, -0.65f};
        DrawLine3D(center,
            (Vector3){6.20f, 0.67f, -0.91f}, RED);
        DrawLine3D(
            (Vector3){6.20f, 0.67f, -0.91f},
            (Vector3){6.32f, 0.40f, -1.04f}, RED);
        DrawCircle3D(
            (Vector3){6.0f, 1.28f + 0.19f * pulse, -0.72f},
            0.38f, (Vector3){1, 0, 0}, 75.0f, gold);
        break;
    }
    case CHRONO_ACTION_MEASURE_RIVER: {
        /* Handheld laser meter aimed across the river. */
        /* Player B stands at X=4.5, Z=2.6 on the west bank. */
        const float measurement_z = 2.6f;
        const float river_center =
            8.0f + sinf(measurement_z * 0.72f) * 0.24f;
        const float river_half_width =
            1.43f + 0.11f * cosf(measurement_z * 0.85f);

        const Vector3 meter = {6.34f, 1.20f, measurement_z};
        const Vector3 target = {
            river_center + river_half_width + 0.20f,
            meter.y,
            measurement_z
        };
        const bool measuring = t >= 0.40f && t <= 0.67f;
        const bool holding_meter = t >= 0.34f && t <= 0.76f;

        /* The instrument appears only after reaching the bank. */
        if (!holding_meter) break;

        /* Dark handheld instrument and illuminated display. */
        DrawCubeV(
            meter,
            (Vector3){0.34f, 0.19f, 0.16f},
            (Color){39, 48, 56, 255}
        );
        DrawCubeV(
            (Vector3){meter.x, meter.y + 0.105f, meter.z},
            (Vector3){0.19f, 0.018f, 0.11f},
            measuring
                ? (Color){103, 238, 169, 255}
                : (Color){58, 104, 91, 255}
        );

        /* Brief, thin measurement beam: instrument to far bank. */
        if (measuring) {
            const Vector3 emitter = {
                meter.x + 0.18f, meter.y, meter.z
            };
            DrawLine3D(
                emitter, target,
                (Color){255, 79, 66, 220}
            );
            DrawSphere(
                target, 0.055f,
                (Color){255, 100, 75, 255}
            );
        }

        /* Display confirms the measurement without giving a hint. */
        if (t > 0.67f) {
            DrawCubeV(
                (Vector3){meter.x, meter.y + 0.105f, meter.z},
                (Vector3){0.19f, 0.018f, 0.11f},
                (Color){103, 238, 169, 255}
            );
        }
        break;
    }
    case CHRONO_ACTION_INSPECT_DUCK:
    case CHRONO_ACTION_PREPARE_DUCK: {
        if (!scene->world.duck_exists) break;

        /* Side-mounted service bay on the near pontoon support. */
        const float dx = 5.5f;
        const float dz = -0.65f;
        const float side_z = dz + 0.75f;
        const float progress = fminf(fmaxf(
            (t - 0.18f) / 0.62f, 0.0f), 1.0f);
        const float ease = progress * progress *
                           (3.0f - 2.0f * progress);
        const Color housing = {55, 70, 78, 255};
        const Color latch = {174, 183, 186, 255};
        const Color locked = {216, 102, 65, 255};
        const Color unlocked = {83, 217, 139, 255};

        /* Service housing physically attached to the pedestal. */
        DrawCubeV(
            (Vector3){dx - 0.34f, 0.38f, side_z},
            (Vector3){0.47f, 0.36f, 0.13f}, housing
        );

        if (action == CHRONO_ACTION_INSPECT_DUCK) {
            /* Open the hinged cover; reveal the safety-lock pin. */
            const float opening = sinf(progress * PI);
            DrawCubeV(
                (Vector3){
                    dx - 0.34f,
                    0.38f + 0.26f * opening,
                    side_z + 0.09f * opening
                },
                (Vector3){0.49f, 0.055f, 0.17f},
                latch
            );
            DrawSphere(
                (Vector3){
                    dx - 0.34f, 0.40f,
                    side_z + 0.085f
                },
                0.075f, locked
            );
            DrawLine3D(
                (Vector3){dx - 0.54f, 0.41f, side_z},
                (Vector3){
                    dx - 0.54f,
                    0.41f + 0.26f * opening,
                    side_z + 0.09f * opening
                },
                latch
            );
        } else {
            /* Release the lock using a captive side lever. */
            const float handle_x = dx - 0.34f + 0.15f * ease;
            DrawCubeV(
                (Vector3){dx - 0.34f, 0.43f, side_z + 0.10f},
                (Vector3){0.30f, 0.055f, 0.07f}, latch
            );
            DrawCubeV(
                (Vector3){handle_x, 0.47f, side_z + 0.15f},
                (Vector3){0.11f, 0.13f, 0.09f}, gold
            );
            DrawSphere(
                (Vector3){dx - 0.34f, 0.33f, side_z + 0.09f},
                0.068f, progress > 0.72f ? unlocked : locked
            );

            /* Both pontoon retaining braces extend in place. */
            for (int side = -1; side <= 1; side += 2) {
                const float pontoon_z = dz + (float)side * 0.36f;
                DrawCubeV(
                    (Vector3){
                        dx + 0.41f + 0.17f * ease,
                        0.30f, pontoon_z
                    },
                    (Vector3){
                        0.13f + 0.19f * ease,
                        0.09f, 0.12f
                    },
                    latch
                );
            }
        }
        break;
    }
    case CHRONO_ACTION_START_DUCK: {
        if (!scene->world.duck_exists) break;

        /* Starter is fixed to the duck's near-side service panel. */
        const float dx = 5.5f;
        const float dz = -0.65f;
        const float panel_z = dz + 0.75f;
        const float activation = fminf(fmaxf(
            (t - 0.18f) / 0.24f, 0.0f), 1.0f);
        const float spin = fminf(fmaxf(
            (t - 0.40f) / 0.28f, 0.0f), 1.0f);
        const float angle = t * (4.0f + spin * 42.0f);
        const Color casing = {53, 67, 77, 255};
        const Color indicator = spin > 0.20f
            ? (Color){97, 241, 139, 255}
            : (Color){178, 74, 57, 255};

        DrawCubeV(
            (Vector3){dx + 0.30f, 0.40f, panel_z},
            (Vector3){0.36f, 0.29f, 0.12f}, casing
        );

        /* The captive starter handle is pushed down. */
        DrawCubeV(
            (Vector3){
                dx + 0.23f,
                0.62f - 0.13f * activation,
                panel_z + 0.075f
            },
            (Vector3){0.09f, 0.21f, 0.08f}, steel
        );
        DrawSphere(
            (Vector3){
                dx + 0.23f,
                0.73f - 0.13f * activation,
                panel_z + 0.075f
            },
            0.10f, gold
        );
        DrawSphere(
            (Vector3){dx + 0.40f, 0.45f, panel_z + 0.075f},
            0.065f, indicator
        );

        /* Stern-mounted propeller accelerates after ignition. */
        const Vector3 hub = {dx - 0.96f, 0.42f, dz};
        DrawCylinderEx(
            (Vector3){dx - 0.72f, 0.42f, dz},
            hub, 0.075f, 0.075f, 10, casing
        );
        DrawSphere(hub, 0.095f, steel);
        for (int blade = 0; blade < 3; ++blade) {
            const float a = angle + (float)blade * 2.0f * PI / 3.0f;
            DrawLine3D(
                hub,
                (Vector3){
                    hub.x,
                    hub.y + 0.29f * cosf(a),
                    hub.z + 0.29f * sinf(a)
                },
                (Color){212, 225, 229, 255}
            );
            DrawSphere(
                (Vector3){
                    hub.x,
                    hub.y + 0.29f * cosf(a),
                    hub.z + 0.29f * sinf(a)
                },
                0.055f, steel
            );
        }
        break;
    }
    case CHRONO_ACTION_ENTER_TOWER: {
        /* The tower's entrance opens as the explorer approaches. */
        const float open = sinf(t * PI);
        DrawCubeV(
            (Vector3){11.70f + 0.48f * open, 0.93f, -0.62f},
            (Vector3){0.90f, 1.85f, 0.14f},
            (Color){118, 82, 56, 255});
        DrawSphere(
            (Vector3){12.02f + 0.48f * open, 1.00f, -0.48f},
            0.075f, gold);
        break;
    }
    case CHRONO_ACTION_RECOVER_CAPSULE: {
        /* A metal capsule rises from a protective container. */
        const float lift = fminf(
            fmaxf((t - 0.20f) / 0.65f, 0.0f), 1.0f);
        DrawCubeV(
            (Vector3){11.55f, 0.32f, -0.78f},
            (Vector3){0.86f, 0.48f, 0.76f}, steel);
        DrawCylinder(
            (Vector3){11.55f, 0.66f + lift * 1.20f, -0.78f},
            0.20f, 0.20f, 0.58f, 16, gold);
        DrawSphere(
            (Vector3){11.55f, 0.98f + lift * 1.20f, -0.78f},
            0.20f, gold);
        break;
    }
    case CHRONO_ACTION_CROSS_BRIDGE:
    case CHRONO_ACTION_CROSS_RIVER:
    case CHRONO_ACTION_REACH_TOWER:
        /* Locomotion is rendered by the explorer model. */
        (void)west;
        break;
    default:
        break;
    }
}

/* Per-vertex normals provide simple, consistent directional lighting. */
static const char *diorama_vertex_shader =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec3 vertexNormal;\n"
    "in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "out vec3 fragNormal;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragNormal = vertexNormal;\n"
    "    fragColor = vertexColor;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *diorama_fragment_shader =
    "#version 330\n"
    "in vec3 fragNormal;\n"
    "in vec4 fragColor;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec3 normal = normalize(fragNormal);\n"
    "    vec3 sun = normalize(vec3(-0.55, 0.88, 0.48));\n"
    "    float diffuse = max(dot(normal, sun), 0.0);\n"
    "    float illumination = 0.56 + 0.44 * diffuse;\n"
    "    finalColor = vec4(fragColor.rgb * illumination, fragColor.a);\n"
    "}\n";

int main(int argc, char **argv)
{
    const bool screenshot_mode = argc == 3 &&
        strcmp(argv[1], "--screenshot") == 0;
    if (argc != 1 && !screenshot_mode) return 2;
    ChronoState state;
    Chrono3DScene scene;

    chrono_init(&state);

    if (!chrono_3d_scene_from_state(&state, &scene)) {
        return 1;
    }

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "ChronoOops! - Temporal Diorama P2");

    if (!IsWindowReady()) {
        return 1;
    }

    Shader lighting = LoadShaderFromMemory(
        diorama_vertex_shader, diorama_fragment_shader
    );
    const bool lighting_ready = lighting.id != 0;
    const char *model_path = TextFormat(
        "%sassets/3d/municipality_1985.gltf",
        GetApplicationDirectory()
    );
    Model municipality = LoadModel(model_path);
    if (municipality.meshCount == 0) {
        TraceLog(LOG_ERROR, "Municipality glTF could not be loaded");
        if (lighting_ready) UnloadShader(lighting);
        CloseWindow();
        return 1;
    }

    Model archive = LoadModel(TextFormat(
        "%sassets/3d/archive_1985.gltf",
        GetApplicationDirectory()
    ));
    if (archive.meshCount == 0) {
        TraceLog(LOG_ERROR, "Archive glTF could not be loaded");
        UnloadModel(municipality);
        if (lighting_ready) UnloadShader(lighting);
        CloseWindow();
        return 1;
    }

    Model tower = LoadModel(TextFormat(
        "%sassets/3d/tower_hydraulic_2025.gltf",
        GetApplicationDirectory()
    ));
    if (tower.meshCount == 0) {
        TraceLog(LOG_ERROR, "Tower glTF could not be loaded");
        UnloadModel(archive);
        UnloadModel(municipality);
        if (lighting_ready) UnloadShader(lighting);
        CloseWindow();
        return 1;
    }

    Model clerk = LoadModel(TextFormat(
        "%sassets/3d/clerk_1985.gltf",
        GetApplicationDirectory()
    ));
    if (clerk.meshCount == 0) {
        TraceLog(LOG_ERROR, "Clerk glTF could not be loaded");
        UnloadModel(tower);
        UnloadModel(archive);
        UnloadModel(municipality);
        if (lighting_ready) UnloadShader(lighting);
        CloseWindow();
        return 1;
    }

    Model explorer = LoadModel(TextFormat(
        "%sassets/3d/explorer_2025.gltf",
        GetApplicationDirectory()
    ));
    if (explorer.meshCount == 0) {
        TraceLog(LOG_ERROR, "Explorer glTF could not be loaded");
        UnloadModel(clerk);
        UnloadModel(tower);
        UnloadModel(archive);
        UnloadModel(municipality);
        if (lighting_ready) UnloadShader(lighting);
        CloseWindow();
        return 1;
    }

    const char *font_dir = GetApplicationDirectory();
    const char *regular_asset = TextFormat(
        "%sassets/3d/fonts/DejaVuSans.ttf", font_dir
    );
    const char *regular_path = FileExists(regular_asset)
        ? regular_asset
        : "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";

    Font regular = FileExists(regular_path)
        ? LoadFontEx(regular_path, 40, NULL, 0)
        : GetFontDefault();
    bool regular_owned = regular.texture.id != GetFontDefault().texture.id
        && regular.texture.id != 0;
    if (!regular_owned) regular = GetFontDefault();

    const char *bold_asset = TextFormat(
        "%sassets/3d/fonts/DejaVuSans-Bold.ttf", font_dir
    );
    const char *bold_path = FileExists(bold_asset)
        ? bold_asset
        : "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf";

    Font heading = FileExists(bold_path)
        ? LoadFontEx(bold_path, 48, NULL, 0)
        : GetFontDefault();
    bool heading_owned = heading.texture.id != GetFontDefault().texture.id
        && heading.texture.id != 0;
    if (!heading_owned) heading = GetFontDefault();

    SetTargetFPS(60);

    Camera3D camera = {
        .position = {18.0f, 25.0f, 32.0f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 25.0f,
        .projection = CAMERA_ORTHOGRAPHIC
    };

    const char *feedback = "Pronto";
    static const char *english_action_labels[CHRONO_3D_ACTION_COUNT] = {
        "Wait",
        "Go to archive",
        "Read plans",
        "Inspect bridge ruins",
        "Identify defect",
        "Measure river",
        "Select bridge",
        "Approve bridge",
        "Select Duck",
        "Approve Duck",
        "Cross bridge",
        "Reach tower",
        "Verify plan",
        "Inspect Duck",
        "Prepare Duck",
        "Read manual",
        "Calibrate Duck",
        "Start Duck",
        "Cross river",
        "Verify intervention",
        "Enter tower",
        "Recover capsule",
        "Question the pigeon",
        "Check temporal horoscope",
        "Throw stone into river",
        "Knock on tower door",
    };
    bool menu_open = false;
    bool choosing_language = !screenshot_mode;
    bool choosing_difficulty = !screenshot_mode;
    bool english = false;
    bool hard_mode = false;
    unsigned decoys_used = 0;
    bool result_processed = false;
    bool initials_pending = false;
    bool hall_visible = false;
    char arcade_initials[4] = {0};
    unsigned initials_length = 0;
    ChronoHallSession hall_session;
    chrono_hall_session_open(&hall_session);
    int selected_action = 0;
    Chrono3DEffects active_effects = {0};
    float bridge_elapsed = 1.2f;
    float character_elapsed = 2.4f;
    ChronoAction character_action = CHRONO_ACTION_WAIT;
    ChronoPlayer character_player = CHRONO_PLAYER_A;
    ChronoLocation previous_location_b = CHRONO_LOCATION_WEST_BANK;

    while (!WindowShouldClose()) {
        if (state.finished && !result_processed) {
            result_processed = true;
            unsigned final_score = chrono_score3d(
                        state.victory, hard_mode,
                        state.selected_plan, state.turn,
                        decoys_used,
                        state.world.tide_register_acquired
                    );
            initials_pending = !screenshot_mode &&
                chrono_hall_session_qualifies(
                    &hall_session, hard_mode,
                    state.selected_plan, final_score
                );
            initials_length = 0;
            memset(arcade_initials, 0, sizeof(arcade_initials));
        }

        if (initials_pending) {
            int codepoint;
            while ((codepoint = GetCharPressed()) != 0) {
                if (initials_length < 3 &&
                    ((codepoint >= 'A' && codepoint <= 'Z') ||
                     (codepoint >= 'a' && codepoint <= 'z'))) {
                    if (codepoint >= 'a')
                        codepoint -= 'a' - 'A';
                    arcade_initials[initials_length++] =
                        (char)codepoint;
                    arcade_initials[initials_length] = '\0';
                }
            }
            if (IsKeyPressed(KEY_BACKSPACE) && initials_length > 0) {
                arcade_initials[--initials_length] = '\0';
            }
            if (IsKeyPressed(KEY_ENTER) && initials_length == 3) {
                unsigned score = chrono_score3d(
                        state.victory, hard_mode,
                        state.selected_plan, state.turn,
                        decoys_used,
                        state.world.tide_register_acquired
                    );
                chrono_hall_session_record(
                    &hall_session, hard_mode, state.selected_plan,
                    arcade_initials, score
                );
                initials_pending = false;
            }
        }

        if (state.finished && !initials_pending &&
            !choosing_language && !choosing_difficulty &&
            IsKeyPressed(KEY_H)) {
            hall_visible = !hall_visible;
        }

        if (state.finished && !initials_pending &&
            !choosing_language &&
            !choosing_difficulty && IsKeyPressed(KEY_R)) {
            chrono_init(&state);
            if (!chrono_3d_scene_from_state(&state, &scene)) {
                TraceLog(LOG_ERROR, "P2 restart projection failed");
                break;
            }
            choosing_language = true;
            choosing_difficulty = true;
            english = false;
            hard_mode = false;
            decoys_used = 0;
            result_processed = false;
            initials_pending = false;
            hall_visible = false;
            hall_session.saved = false;
            hall_session.last_ranks = (ChronoHallRanks){0};
            initials_length = 0;
            memset(arcade_initials, 0, sizeof(arcade_initials));
            menu_open = false;
            selected_action = 0;
            active_effects = (Chrono3DEffects){0};
            bridge_elapsed = 1.2f;
            character_elapsed = 2.4f;
            character_action = CHRONO_ACTION_WAIT;
            character_player = CHRONO_PLAYER_A;
            previous_location_b = CHRONO_LOCATION_WEST_BANK;
            feedback = "Pronto";
        }

        if (choosing_language) {
            if (IsKeyPressed(KEY_I)) {
                english = false;
                choosing_language = false;
                feedback = english ? "Ready" : "Pronto";
            } else if (IsKeyPressed(KEY_E)) {
                english = true;
                choosing_language = false;
                feedback = english ? "Ready" : "Pronto";
            }
        } else if (choosing_difficulty) {
            if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
                hard_mode = false;
                choosing_difficulty = false;
            } else if (IsKeyPressed(KEY_TWO) ||
                       IsKeyPressed(KEY_KP_2)) {
                hard_mode = true;
                choosing_difficulty = false;
            }
        }

        if (!choosing_language && !choosing_difficulty &&
            !state.finished && IsKeyPressed(KEY_SPACE)) {
            TraceLog(LOG_INFO,
                "P2_KEY key=SPACE code=%d menu=%d turn=%u ap=%u",
                KEY_SPACE, menu_open ? 1 : 0,
                state.turn, state.action_points);
        }
        if (IsKeyPressed(KEY_ENTER)) {
            TraceLog(LOG_INFO,
                "P2_KEY key=ENTER code=%d menu=%d turn=%u ap=%u",
                KEY_ENTER, menu_open ? 1 : 0,
                state.turn, state.action_points);
        }

        if (bridge_elapsed < 1.2f) {
            bridge_elapsed += GetFrameTime();
            if (bridge_elapsed > 1.2f) bridge_elapsed = 1.2f;
        }

        if (character_elapsed < 2.4f) {
            character_elapsed += GetFrameTime();
            if (character_elapsed > 2.4f) character_elapsed = 2.4f;
        }

        const bool character_animating = character_elapsed < 2.4f;
        const bool animating =
            bridge_elapsed < 1.2f || character_animating;
        ChronoResult input_result = CHRONO_OK;
        bool submitted = false;
        const bool automatic_turn_end =
            !animating && state.action_points == 0;

        if (!choosing_language && !choosing_difficulty &&
            !state.finished && !animating &&
            IsKeyPressed(KEY_TAB)) {
            menu_open = !menu_open;
        }

        if (!choosing_language && !choosing_difficulty &&
            !state.finished && !animating && menu_open) {
            if (IsKeyPressed(KEY_UP) && selected_action > 0) {
                selected_action--;
            }
            if (IsKeyPressed(KEY_DOWN) &&
                selected_action < CHRONO_3D_ACTION_COUNT - 1) {
                selected_action++;
            }
            if (IsKeyPressed(KEY_LEFT) && selected_action >= 13) {
                selected_action -= 13;
            }
            if (IsKeyPressed(KEY_RIGHT) && selected_action < 13) {
                selected_action += 13;
            }
            if (IsKeyPressed(KEY_ENTER)) {
                const Chrono3DActionEntry *actions = chrono_3d_actions();
                ChronoAction action = actions[selected_action].id;
                const bool is_decoy =
                    action == CHRONO_ACTION_WAIT ||
                    action == CHRONO_ACTION_QUESTION_PIGEON ||
                    action == CHRONO_ACTION_CHECK_HOROSCOPE ||
                    action == CHRONO_ACTION_THROW_STONE ||
                    action == CHRONO_ACTION_KNOCK_TOWER;
                Chrono3DActionStatus status =
                    chrono_3d_action_status(&state, action);
                TraceLog(
                    LOG_INFO,
                    "P2_ACTION_SELECT turn=%u ap=%u action=%d status=%d",
                    state.turn, state.action_points,
                    (int)action, (int)status
                );

                if (status == CHRONO_3D_ACTION_AVAILABLE &&
                    !hard_mode && is_decoy) {
                    /* Easy mode: a harmless decoy, without spending a point. */
                    decoys_used++;
                    character_action = action;
                    character_player = state.active_player;
                    character_elapsed = 0.0f;
                    feedback = english
    ? "Absurd detour: no Action points spent"
    : "Diversivo assurdo: nessun Punto azione consumato";
                    menu_open = false;
                } else if (status == CHRONO_3D_ACTION_AVAILABLE) {
                    previous_location_b = state.location_b;
                    input_result = chrono_3d_apply(&state, action);
                    TraceLog(
                        LOG_INFO,
                        "P2_ACTION_APPLY turn=%u action=%d result=%d "
                        "ap_after=%u location_b=%d duck_running=%d "
                        "river_crossed=%d",
                        state.turn, (int)action, (int)input_result,
                        state.action_points, (int)state.location_b,
                        state.world.duck_running ? 1 : 0,
                        state.river_crossed ? 1 : 0
                    );
                    if (input_result == CHRONO_OK) {
                        if (is_decoy)
                            decoys_used++;
                        character_action = action;
                        character_player = state.active_player;
                        character_elapsed = 0.0f;
                    }
                    submitted = true;
                } else {
                    feedback = hard_mode
                        ? (english ? "Nothing happens" : "Non succede nulla")
                        : status == CHRONO_3D_ACTION_DONE
                            ? (english ? "Already completed" : "Azione gia' completata")
                            : status == CHRONO_3D_ACTION_OTHER_PLAYER
                                ? (english ? "Other character's action" : "Azione dell'altro personaggio")
                                : (english ? "Action blocked" : "Azione bloccata");
                }
            }
        }

        if (!choosing_language && !choosing_difficulty &&
            !state.finished && !animating &&
            (automatic_turn_end || IsKeyPressed(KEY_SPACE))) {
            ChronoTrace trace = {0};
            input_result = chrono_3d_end_turn(&state, &trace);
            submitted = true;

            if (input_result == CHRONO_OK) {
                selected_action = 0;
                Chrono3DEffects next_effects = {0};

                if (!chrono_3d_effects_from_trace(
                        input_result, &trace, &next_effects)) {
                    TraceLog(LOG_ERROR, "P2 causal projection failed");
                    break;
                }

                active_effects = next_effects;
                TraceLog(
                    LOG_INFO,
                    "P2_CAUSAL turn=%u approved=%d changed=%d "
                    "bridge_before=%d bridge_after=%d flags=0x%X",
                    trace.turn,
                    state.plan_approved ? 1 : 0,
                    trace.changed ? 1 : 0,
                    trace.before.bridge_exists ? 1 : 0,
                    trace.after.bridge_exists ? 1 : 0,
                    (unsigned)next_effects.flags
                );
                if ((next_effects.flags & CHRONO_3D_EFFECT_BRIDGE) &&
                    !next_effects.before.bridge_exists &&
                    next_effects.after.bridge_exists) {
                    bridge_elapsed = 0.0f;
                    menu_open = false;
                    feedback = english
    ? "Temporal effect: bridge construction"
    : "Effetto temporale: costruzione del ponte";
                }
            }
        }

        if (submitted) {
            switch (input_result) {
            case CHRONO_OK:
                feedback = automatic_turn_end
                    ? (english
                        ? "Turn completed automatically"
                        : "Turno concluso automaticamente")
                    : (english ? "Action accepted" : "Azione eseguita");
                break;
            case CHRONO_WRONG_PLAYER:
                feedback = english ? "Wrong character" : "Personaggio errato";
                break;
            case CHRONO_NO_ACTION_POINTS:
                feedback = english
    ? "No Action points left - press SPACE"
    : "Punti azione esauriti - premi SPAZIO";
                break;
            case CHRONO_PREREQUISITE_MISSING:
                feedback = english
    ? "Prerequisite missing"
    : "Prerequisito mancante";
                break;
            case CHRONO_MISSION_FINISHED:
                feedback = english ? "Mission finished" : "Missione conclusa";
                break;
            default:
                feedback = english ? "Invalid action" : "Azione non valida";
                break;
            }

            if (input_result == CHRONO_OK) {
                menu_open = false;
            }

            if (input_result == CHRONO_OK &&
                !chrono_3d_scene_from_state(&state, &scene)) {
                TraceLog(LOG_ERROR, "P2 scene projection failed");
                break;
            }
        }

        BeginDrawing();
        ClearBackground((Color){34, 48, 67, 255});

        BeginMode3D(camera);
        if (lighting_ready) BeginShaderMode(lighting);
        float clerk_x = scene.location_a == CHRONO_LOCATION_ARCHIVE
            ? -5.5f : -10.5f;
        float clerk_bob = 0.0f;
        float clerk_yaw = 0.0f;

        if (character_animating &&
            character_action == CHRONO_ACTION_MOVE_ARCHIVE &&
            character_player == CHRONO_PLAYER_A) {
            const float t = character_elapsed / 2.4f;

            /* Turn toward the archive before walking. */
            float turn_in = fminf(t / 0.16f, 1.0f);
            turn_in = turn_in * turn_in *
                (3.0f - 2.0f * turn_in);

            /* Face forward again after reaching the archive. */
            float turn_out = fminf(
                fmaxf((t - 0.83f) / 0.17f, 0.0f), 1.0f
            );
            turn_out = turn_out * turn_out *
                (3.0f - 2.0f * turn_out);

            clerk_yaw = 90.0f * turn_in * (1.0f - turn_out);

            const float walk = fminf(
                fmaxf((t - 0.16f) / 0.67f, 0.0f), 1.0f
            );
            const float eased = walk * walk *
                (3.0f - 2.0f * walk);

            clerk_x = -10.5f + 5.0f * eased;
            clerk_bob =
                0.075f * fabsf(sinf(walk * 14.0f * PI)) *
                sinf(walk * PI);
        }

        diorama_1985(
            &municipality, &archive, &clerk,
            clerk_x, clerk_bob, clerk_yaw
        );
        const float bridge_progress =
            (active_effects.flags & CHRONO_3D_EFFECT_BRIDGE) &&
            bridge_elapsed < 1.2f
                ? bridge_elapsed / 1.2f : 1.0f;
        const float inspect_progress =
            character_animating &&
            character_action == CHRONO_ACTION_INSPECT_RUINS &&
            character_player == CHRONO_PLAYER_B
                ? character_elapsed / 2.4f : -1.0f;
        const float motion_progress =
            character_animating &&
            character_player == CHRONO_PLAYER_B
                ? character_elapsed / 2.4f : -1.0f;

        diorama_2025(
            &scene, &tower, &explorer,
            bridge_progress, inspect_progress,
            character_action, motion_progress, previous_location_b
        );
        if (lighting_ready) EndShaderMode();

        if (character_animating &&
            character_player == CHRONO_PLAYER_B) {
            draw_2025_action_props(
                character_action, character_elapsed, &scene
            );
        }

        if (character_animating &&
            character_action == CHRONO_ACTION_READ_PLANS) {
            const float t = character_elapsed / 2.4f;
            const float envelope = sinf(t * PI);
            const float base_x = -5.5f;

            /* A folder opens and three pages turn in sequence. */
            DrawCubeV(
                (Vector3){base_x, 1.02f + envelope * 0.25f, 1.45f},
                (Vector3){0.82f, 0.065f, 0.57f},
                (Color){126, 82, 48, 255}
            );

            for (int page = 0; page < 3; ++page) {
                const float phase = t * 3.0f - (float)page * 0.55f;
                const float lift = fmaxf(0.0f, sinf(phase * PI));
                const float offset = (float)page * 0.055f;

                DrawCubeV(
                    (Vector3){
                        base_x + offset,
                        1.11f + envelope * 0.25f + lift * 0.42f,
                        1.45f - lift * 0.24f
                    },
                    (Vector3){0.72f, 0.025f, 0.48f},
                    (Color){247, 235, 193, 255}
                );
            }
        }

        if (character_animating &&
            character_player == CHRONO_PLAYER_A) {
            draw_1985_action_props(
                character_action, character_elapsed, clerk_x
            );
        }

        EndMode3D();

        if (character_animating &&
            character_action == CHRONO_ACTION_WAIT) {
            float t = character_elapsed / 2.4f;
            float x = character_player == CHRONO_PLAYER_A
                ? clerk_x : (scene.location_b >= CHRONO_LOCATION_EAST_BANK
                    ? 11.0f : 4.5f);
            float z = character_player == CHRONO_PLAYER_A
                ? 1.05f : 2.6f;
            Vector3 above = {
                x, 2.0f + 0.12f * sinf(t * 3.0f * PI), z
            };
            Vector2 pos = GetWorldToScreen(above, camera);

            DrawRectangleRounded(
                (Rectangle){pos.x - 39.0f, pos.y - 27.0f,
                            103.0f, 45.0f},
                0.38f, 8, (Color){249, 245, 224, 245}
            );
            DrawTextEx(
                heading, "ZzZzZ...",
                (Vector2){pos.x - 28.0f, pos.y - 17.0f},
                17.0f, 0.3f, (Color){57, 75, 108, 255}
            );
        }

        DrawTextEx(
            heading, english ? "ChronoOops! / Temporal Diorama" : "ChronoOops! / Diorama temporale",
            (Vector2){28.0f, 24.0f}, 29.0f, 0.5f, RAYWHITE
        );
        DrawTextEx(
            regular, english ? "1985 - Municipality / Archive" : "1985 - Municipio / Archivio",
            (Vector2){65.0f, 95.0f}, 20.0f, 0.4f,
            (Color){246, 218, 159, 255}
        );
        DrawTextEx(
            regular, english ? "2025 - River / Tower" : "2025 - Fiume / Torre",
            (Vector2){805.0f, 95.0f}, 20.0f, 0.4f,
            (Color){158, 220, 246, 255}
        );
        DrawTextEx(
            regular, english
                ? "TAB menu - ARROWS select - ENTER act - SPACE end turn - ESC close"
                : "TAB menu - FRECCE scegli - INVIO agisci - SPAZIO fine turno - ESC chiudi",
            (Vector2){28.0f, 672.0f}, 17.0f, 0.3f, LIGHTGRAY
        );

        DrawTextEx(regular, TextFormat(
            english
                ? "Turn %u | Character %s | Action points: %u/2"
                : "Turno %u | Personaggio %s | Punti azione: %u/2",
            scene.turn,
            scene.active_player == CHRONO_PLAYER_A ? "1985" : "2025",
            scene.action_points
        ), (Vector2){28.0f, 610.0f}, 20.0f, 0.4f, RAYWHITE);

        DrawTextEx(
            regular,
            english
                ? "TAB: Action menu | ENTER: Apply selected action"
                : "TAB: Menu azioni | INVIO: Esegui azione selezionata",
            (Vector2){28.0f, 635.0f}, 18.0f, 0.3f, RAYWHITE
        );
        DrawTextEx(
            regular, feedback,
            (Vector2){830.0f, 635.0f}, 18.0f, 0.3f, YELLOW
        );

        if (menu_open && !animating) {
            const Chrono3DActionEntry *actions = chrono_3d_actions();
            DrawRectangle(150, 125, 980, 460,
                          (Color){20, 30, 44, 246});
            DrawRectangleLines(150, 125, 980, 460,
                               (Color){114, 154, 187, 255});

            DrawTextEx(
                heading, english ? "Mission 001 - Actions" : "Missione 001 - Azioni",
                (Vector2){172.0f, 137.0f},
                23.0f, 0.3f, RAYWHITE
            );

            for (int i = 0; i < CHRONO_3D_ACTION_COUNT; ++i) {
                int column = i / 13;
                int row = i % 13;
                float x = 172.0f + (float)column * 485.0f;
                float y = 184.0f + (float)row * 29.0f;

                Chrono3DActionStatus action_status =
                    chrono_3d_action_status(&state, actions[i].id);

                if (i == selected_action) {
                    DrawRectangle(
                        (int)x - 5, (int)y - 4, 464, 30,
                        (Color){56, 83, 108, 255}
                    );
                }

                Color action_color = hard_mode
                    ? (Color){228, 229, 224, 255}
                    : action_status == CHRONO_3D_ACTION_AVAILABLE
                        ? (Color){174, 237, 175, 255}
                        : action_status == CHRONO_3D_ACTION_DONE
                            ? (Color){150, 206, 230, 255}
                            : (Color){151, 160, 172, 255};

                const char *marker = hard_mode ? ""
                    : action_status == CHRONO_3D_ACTION_AVAILABLE
                        ? (english ? "READY" : "PRONTA")
                        : action_status == CHRONO_3D_ACTION_DONE
                            ? (english ? "DONE" : "FATTA")
                            : action_status == CHRONO_3D_ACTION_OTHER_PLAYER
                                ? (english ? "OTHER" : "ALTRO")
                        : (english ? "LOCKED" : "BLOCCATA");

                DrawTextEx(
                    regular,
                    english ? english_action_labels[i] : actions[i].label,
                    (Vector2){x, y}, 17.0f, 0.2f, action_color
                );
                DrawTextEx(
                    regular, marker,
                    (Vector2){x + 375.0f, y + 2.0f},
                    13.0f, 0.1f, action_color
                );
            }
        }

        if (choosing_difficulty) {
            DrawRectangle(
                200, 165, 880, 365, (Color){19, 29, 43, 251}
            );
            DrawRectangleLines(
                200, 165, 880, 365, (Color){157, 185, 211, 255}
            );
            DrawTextEx(
                heading, "ChronoOops!",
                (Vector2){250.0f, 202.0f},
                39.0f, 0.5f, RAYWHITE
            );
            DrawTextEx(
                regular, english ? "Choose difficulty" : "Scegli la difficolta'",
                (Vector2){250.0f, 265.0f},
                27.0f, 0.3f, RAYWHITE
            );
            DrawTextEx(
                regular, english ? "1 - EASY: blocked actions indicated" : "1 - FACILE: azioni bloccate segnalate",
                (Vector2){250.0f, 337.0f},
                21.0f, 0.3f, (Color){174, 237, 175, 255}
            );
            DrawTextEx(
                regular, english ? "2 - HARD: no assistance" : "2 - DIFFICILE: nessun aiuto",
                (Vector2){250.0f, 389.0f},
                21.0f, 0.3f, (Color){255, 198, 133, 255}
            );
            DrawTextEx(
                regular, english ? "8 turns - 2 Action points per turn" : "8 turni - 2 Punti azione per turno",
                (Vector2){250.0f, 459.0f},
                18.0f, 0.3f, (Color){179, 194, 210, 255}
            );
        }

        if (choosing_language) {
            DrawRectangle(200, 165, 880, 365,
                          (Color){19, 29, 43, 251});
            DrawRectangleLines(200, 165, 880, 365,
                               (Color){157, 185, 211, 255});
            DrawTextEx(heading, "ChronoOops!",
                       (Vector2){250.0f, 202.0f},
                       39.0f, 0.5f, RAYWHITE);
            DrawTextEx(regular, "Lingua / Language",
                       (Vector2){250.0f, 270.0f},
                       27.0f, 0.3f, RAYWHITE);
            DrawTextEx(regular, "I - Italiano",
                       (Vector2){250.0f, 345.0f},
                       23.0f, 0.3f, RAYWHITE);
            DrawTextEx(regular, "E - English",
                       (Vector2){250.0f, 405.0f},
                       23.0f, 0.3f, RAYWHITE);
        }

        if (state.finished && !choosing_language &&
            !choosing_difficulty && !animating) {
            const bool won = state.victory;
            const Color accent = won
                ? (Color){174, 237, 175, 255}
                : (Color){255, 198, 133, 255};

            DrawRectangle(
                180, 170, 920, 360, (Color){19, 29, 43, 250}
            );
            DrawRectangleLines(180, 170, 920, 360, accent);

            DrawTextEx(
                heading,
                won
                    ? (english ? "MISSION COMPLETED"
                               : "MISSIONE COMPLETATA")
                    : (english ? "MISSION FAILED"
                               : "MISSIONE FALLITA"),
                (Vector2){225.0f, 208.0f},
                32.0f, 0.4f, accent
            );

            DrawTextEx(
                regular,
                won
                    ? (english ? "The time capsule has been recovered."
                               : "La capsula temporale e' stata recuperata.")
                    : (english ? "The temporal paradox remains unresolved."
                               : "Il paradosso temporale resta irrisolto."),
                (Vector2){225.0f, 295.0f},
                22.0f, 0.3f, RAYWHITE
            );

            DrawTextEx(
                regular,
                won
                    ? (english ? "The timeline can finally breathe again."
                               : "La linea temporale puo' finalmente respirare.")
                    : (english ? "Time moves on. The pigeon is doing fine."
                               : "Il tempo passa. Il piccione sta benissimo."),
                (Vector2){225.0f, 342.0f},
                20.0f, 0.3f, LIGHTGRAY
            );

            DrawTextEx(
                regular,
                TextFormat(
                    english
                        ? "FINAL SCORE: %u"
                        : "PUNTEGGIO FINALE: %u",
                    chrono_score3d(
                        state.victory, hard_mode,
                        state.selected_plan, state.turn,
                        decoys_used,
                        state.world.tide_register_acquired
                    )
                ),
                (Vector2){225.0f, 394.0f},
                25.0f, 0.3f, accent
            );

            DrawTextEx(
                regular,
                initials_pending
                     ? (english
                         ? "Enter 3 initials - BACKSPACE / ENTER"
                         : "Inserisci 3 iniziali - BACKSPACE / INVIO")
                     : (english
                         ? "Press R to start a new game"
                         : "Premi R per iniziare una nuova partita"),
                (Vector2){225.0f, 446.0f},
                21.0f, 0.3f, accent
            );
        }

        if (initials_pending) {
            DrawRectangle(
                455, 530, 370, 125, (Color){19, 29, 43, 250}
            );
            DrawRectangleLines(
                455, 530, 370, 125, (Color){255, 214, 107, 255}
            );
            DrawTextEx(
                heading, "INSERT YOUR INITIALS",
                (Vector2){476.0f, 545.0f},
                20.0f, 0.3f, (Color){255, 214, 107, 255}
            );
            DrawTextEx(
                heading,
                TextFormat(
                    "%c %c %c",
                    initials_length > 0 ? arcade_initials[0] : '_',
                    initials_length > 1 ? arcade_initials[1] : '_',
                    initials_length > 2 ? arcade_initials[2] : '_'
                ),
                (Vector2){555.0f, 586.0f},
                32.0f, 1.0f, RAYWHITE
            );
        }

        if (state.finished && !choosing_language &&
            !choosing_difficulty && !animating) {
            if (!initials_pending) {
                const char *record_message =
                    !hall_session.ready
                        ? (english ? "Records unavailable"
                                   : "Archivio record non disponibile")
                    : hall_session.save_error
                        ? (english ? "Record save failed"
                                   : "Salvataggio record fallito")
                    : hall_session.saved
                        ? TextFormat(
                            english ? "Daily #%u  |  All-time #%u"
                                    : "Giornaliera #%u  |  Assoluta #%u",
                            hall_session.last_ranks.daily_rank,
                            hall_session.last_ranks.all_time_rank
                          )
                        : (english ? "No new record" : "Nessun nuovo record");

                DrawTextEx(
                    regular, record_message,
                    (Vector2){225.0f, 483.0f},
                    17.0f, 0.2f, LIGHTGRAY
                );
                DrawTextEx(
                    regular,
                    english ? "H: Hall of Fame"
                            : "H: Classifiche Hall of Fame",
                    (Vector2){715.0f, 483.0f},
                    17.0f, 0.2f, YELLOW
                );
            }

            if (hall_visible && !initials_pending) {
                unsigned category =
                    (hard_mode ? 2u : 0u) +
                    (state.selected_plan == CHRONO_PLAN_DUCK ? 1u : 0u);

                DrawRectangle(
                    90, 65, 1100, 590,
                    (Color){14, 23, 37, 253}
                );
                DrawRectangleLines(
                    90, 65, 1100, 590,
                    (Color){255, 214, 107, 255}
                );
                DrawTextEx(
                    heading, "CHRONOOOPS! HALL OF FAME",
                    (Vector2){190.0f, 85.0f},
                    31.0f, 0.3f,
                    (Color){255, 214, 107, 255}
                );

                DrawTextEx(
                    regular,
                    TextFormat(
                        "%s  |  %s",
                        hard_mode
                            ? (english ? "HARD" : "DIFFICILE")
                            : (english ? "EASY" : "FACILE"),
                        state.selected_plan == CHRONO_PLAN_DUCK
                            ? (english ? "DUCK" : "PAPERA")
                            : "PONTE / BRIDGE"
                    ),
                    (Vector2){350.0f, 137.0f},
                    20.0f, 0.3f, RAYWHITE
                );

                for (unsigned board_type = 0;
                     board_type < 2; ++board_type) {
                    const ChronoHallBoard *board =
                        board_type == 0
                            ? &hall_session.hall.daily[category]
                            : &hall_session.hall.all_time[category];

                    float x = board_type == 0 ? 150.0f : 680.0f;
                    DrawTextEx(
                        heading,
                        board_type == 0
                            ? (english ? "DAILY TOP 10"
                                       : "TOP 10 GIORNALIERA")
                            : (english ? "ALL-TIME TOP 10"
                                       : "TOP 10 ASSOLUTA"),
                        (Vector2){x, 185.0f},
                        22.0f, 0.2f,
                        (Color){255, 214, 107, 255}
                    );

                    for (unsigned i = 0;
                         i < board->count; ++i) {
                        const ChronoHallEntry *entry =
                            &board->entries[i];

                        DrawTextEx(
                            regular,
                            TextFormat(
                                "%02u   %s       %u",
                                i + 1u,
                                entry->initials,
                                entry->score
                            ),
                            (Vector2){
                                x, 237.0f + (float)i * 31.0f
                            },
                            20.0f, 0.3f, RAYWHITE
                        );
                    }

                    if (board->count == 0) {
                        DrawTextEx(
                            regular,
                            english ? "No records yet"
                                    : "Nessun record",
                            (Vector2){x, 245.0f},
                            20.0f, 0.3f, LIGHTGRAY
                        );
                    }
                }

                DrawTextEx(
                    regular,
                    english
                        ? "H: Close Hall of Fame   |   R: New game"
                        : "H: Chiudi classifiche   |   R: Nuova partita",
                    (Vector2){280.0f, 602.0f},
                    19.0f, 0.3f,
                    (Color){255, 214, 107, 255}
                );
            }
        }

        EndDrawing();

        if (screenshot_mode) {
            TakeScreenshot(argv[2]);
            break;
        }
    }

    if (heading_owned) UnloadFont(heading);
    if (regular_owned) UnloadFont(regular);
    UnloadModel(explorer);
    UnloadModel(clerk);
    UnloadModel(tower);
    UnloadModel(archive);
    UnloadModel(municipality);
    if (lighting_ready) UnloadShader(lighting);
    CloseWindow();
    return 0;
}
