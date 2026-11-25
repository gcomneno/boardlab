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
    const Chrono3DScene *scene,
    const Model *municipality, const Model *archive,
    const Model *clerk
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

    /* Player A remains positioned by authoritative C11 state. */
    const float player_x =
        scene->location_a == CHRONO_LOCATION_ARCHIVE
            ? x + 2.5f : x - 2.5f;

    DrawModel(
        *clerk,
        (Vector3){player_x, 0.0f, 1.05f},
        1.15f, WHITE
    );
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
    const Model *explorer
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

    /* Only authoritative world events materialize these objects. */
    if (scene->world.bridge_exists) {
        DrawCubeV(
            (Vector3){x, 0.57f, 1.20f},
            (Vector3){5.15f, 0.30f, 1.75f},
            (Color){155, 108, 72, 255}
        );
        for (int i = 0; i < 5; ++i) {
            float px = x - 2.0f + (float)i;
            DrawCubeV(
                (Vector3){px, 0.96f, 0.35f},
                (Vector3){0.14f, 0.76f, 0.14f},
                (Color){111, 73, 53, 255}
            );
        }
    }

    if (scene->world.duck_exists) {
        const Color duck_color = scene->world.duck_running
            ? (Color){255, 223, 66, 255}
            : (Color){225, 175, 72, 255};

        DrawSphere(
            (Vector3){x - 2.5f, 0.52f, 3.0f},
            0.60f, duck_color
        );
        DrawSphere(
            (Vector3){x - 2.18f, 0.99f, 3.0f},
            0.32f, duck_color
        );
        DrawCubeV(
            (Vector3){x - 1.82f, 0.92f, 3.0f},
            (Vector3){0.37f, 0.16f, 0.28f},
            (Color){243, 145, 65, 255}
        );
    }

    /* Player B remains positioned by authoritative C11 state. */
    const float player_x =
        scene->location_b >= CHRONO_LOCATION_EAST_BANK
            ? x + 3.0f : x - 3.5f;

    DrawModel(
        *explorer,
        (Vector3){player_x, 0.0f, 2.6f},
        1.15f, WHITE
    );
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
    InitWindow(1280, 720, "ChronoOops! - Temporal Diorama P1");

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
        "%sassets/3d/tower_2025.gltf",
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

    SetTargetFPS(60);

    Camera3D camera = {
        .position = {18.0f, 25.0f, 32.0f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 25.0f,
        .projection = CAMERA_ORTHOGRAPHIC
    };

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){34, 48, 67, 255});

        BeginMode3D(camera);
        if (lighting_ready) BeginShaderMode(lighting);
        diorama_1985(&scene, &municipality, &archive, &clerk);
        diorama_2025(&scene, &tower, &explorer);
        if (lighting_ready) EndShaderMode();
        EndMode3D();

        DrawText("CHRONOOOPS!  /  TEMPORAL DIORAMA",
                 28, 24, 27, RAYWHITE);
        DrawText("1985  -  Municipality / Archive",
                 65, 95, 19, (Color){246, 218, 159, 255});
        DrawText("2025  -  River / Tower",
                 805, 95, 19, (Color){158, 220, 246, 255});
        DrawText("P1 visual prototype - C11 state snapshot - ESC to close",
                 28, 672, 17, LIGHTGRAY);

        EndDrawing();

        if (screenshot_mode) {
            TakeScreenshot(argv[2]);
            break;
        }
    }

    UnloadModel(explorer);
    UnloadModel(clerk);
    UnloadModel(tower);
    UnloadModel(archive);
    UnloadModel(municipality);
    if (lighting_ready) UnloadShader(lighting);
    CloseWindow();
    return 0;
}
