#include "streams.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint64_t seed;
    RmcStreamDomain domain;
    uint64_t entity_id;
    uint64_t state;
    uint64_t increment;
    uint32_t draws[3];
} RmcVector;

static const RmcVector vectors[] = {
    {
        UINT64_C(0), RMC_STREAM_WORLD, 0,
        UINT64_C(0xa03e474fa1e191df),
        UINT64_C(0x2dc66dd1e216e9ab),
        {0xde80607c, 0xfc895d7f, 0x9395706d}
    },
    {
        UINT64_C(42), RMC_STREAM_WORLD, 0,
        UINT64_C(0x2cc2b9ce1e4689bd),
        UINT64_C(0xee0f616547bd521f),
        {0xb4c2b2fe, 0x380348b7, 0xa19e8612}
    },
    {
        UINT64_MAX, RMC_STREAM_ATTRIBUTES, 0,
        UINT64_C(0x76797a885452a292),
        UINT64_C(0x8e5baf8125eecd55),
        {0x8b073cb3, 0x34634b1a, 0x199acc1b}
    },
    {
        UINT64_C(42), RMC_STREAM_CALENDAR, 0,
        UINT64_C(0x47161f92efd223bc),
        UINT64_C(0xa2c5fccb152ac2dd),
        {0xede2c1ca, 0x1baae1b0, 0xc2e46b59}
    },
    {
        UINT64_C(42), RMC_STREAM_MATCH, 1,
        UINT64_C(0x23a4f42a949f43d9),
        UINT64_C(0xc43ae722e368bda7),
        {0x5749f987, 0x6bc2ca55, 0x25c7473c}
    },
    {
        UINT64_C(42), RMC_STREAM_MATCH, 2,
        UINT64_C(0xec9b6024595cb48b),
        UINT64_C(0x93dabdb9ae8497cf),
        {0x9b5b0284, 0x39d4d598, 0x849493d9}
    },
    {
        UINT64_C(42), RMC_STREAM_AI, 1,
        UINT64_C(0xc5a23bbbacc4ccee),
        UINT64_C(0xa63ac84c2dfbb36b),
        {0x415a64b4, 0x10009f8a, 0xce218537}
    },
    {
        UINT64_C(42), RMC_STREAM_RPG, 1,
        UINT64_C(0xf8413c3f8535d755),
        UINT64_C(0xcad9241bfa3eb265),
        {0x10408bf2, 0x193c4c55, 0x4135deff}
    }
};

int main(void)
{
    const size_t count = sizeof(vectors) / sizeof(vectors[0]);

    for (size_t i = 0; i < count; ++i) {
        const RmcVector *v = &vectors[i];

        const RmcWorldIdentity identity = {
            v->seed,
            RMC_GENERATOR_VERSION,
            RMC_RULES_VERSION
        };

        RmcPcg32 rng;

        if (!rmc_stream_init(
                &rng, &identity, v->domain, v->entity_id)) {
            fprintf(stderr, "VECTOR_INIT_FAIL=%zu\n", i);
            return 1;
        }

        if (rng.state != v->state ||
            rng.increment != v->increment) {
            fprintf(stderr, "VECTOR_STATE_FAIL=%zu\n", i);
            return 1;
        }

        for (size_t j = 0; j < 3; ++j) {
            if (rmc_pcg32_next(&rng) != v->draws[j]) {
                fprintf(stderr,
                        "VECTOR_DRAW_FAIL=%zu:%zu\n", i, j);
                return 1;
            }
        }
    }

    printf("CANONICAL_STREAM_VECTORS=%zu/8 PASS\n", count);
    return count == 8 ? 0 : 1;
}
