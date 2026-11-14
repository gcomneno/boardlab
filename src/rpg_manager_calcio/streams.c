#include "streams.h"

#include <stddef.h>

/*
 * SplitMix64 finalizer.
 *
 * All operations use explicitly unsigned 64-bit arithmetic.
 * Overflow is defined modulo 2^64.
 */
static uint64_t rmc_mix64(uint64_t value)
{
    value ^= value >> 30u;
    value *= UINT64_C(0xbf58476d1ce4e5b9);

    value ^= value >> 27u;
    value *= UINT64_C(0x94d049bb133111eb);

    return value ^ (value >> 31u);
}

static bool rmc_domain_valid(RmcStreamDomain domain)
{
    return domain >= RMC_STREAM_WORLD &&
           domain <= RMC_STREAM_RPG;
}

bool rmc_stream_init(
    RmcPcg32 *destination,
    const RmcWorldIdentity *identity,
    RmcStreamDomain domain,
    uint64_t entity_id)
{
    if (destination == NULL || identity == NULL) {
        return false;
    }

    if (identity->generator_version != RMC_GENERATOR_VERSION ||
        identity->rules_version != RMC_RULES_VERSION ||
        !rmc_domain_valid(domain)) {
        return false;
    }

    if (domain == RMC_STREAM_MATCH) {
        if (entity_id == 0u) {
            return false;
        }
    } else if (entity_id != 0u &&
               domain != RMC_STREAM_AI &&
               domain != RMC_STREAM_RPG) {
        return false;
    }

    /*
     * Derivation v1, domain-separated.
     *
     * Distinct constants distinguish each input component.
     * Changing constants or mixing order changes the
     * deterministic contract.
     */
    uint64_t value = rmc_mix64(
        identity->world_seed ^
        UINT64_C(0x9e3779b97f4a7c15)
    );

    value = rmc_mix64(
        value ^
        ((uint64_t)identity->generator_version << 32u) ^
        (uint64_t)identity->rules_version
    );

    value = rmc_mix64(
        value ^
        ((uint64_t)(uint32_t)domain *
         UINT64_C(0xd1b54a32d192ed03))
    );

    value = rmc_mix64(
        value ^
        rmc_mix64(entity_id ^
                  UINT64_C(0x8cb92ba72f3d8dd7))
    );

    const uint64_t initstate = rmc_mix64(
        value ^ UINT64_C(0x243f6a8885a308d3)
    );

    const uint64_t initseq = rmc_mix64(
        value ^ UINT64_C(0x13198a2e03707344)
    );

    RmcPcg32 next;
    rmc_pcg32_seed(&next, initstate, initseq);

    *destination = next;
    return true;
}
