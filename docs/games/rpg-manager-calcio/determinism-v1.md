# RPG Manager Calcio — Determinism Specification v1

**Milestone:** M001-A1
**Normative parent:** M001 Engineering Contract v1.0
**GeneratorVersion:** 1
**RulesVersion:** 1
**Status:** implementation baseline, qualified by CTest

## 1. General-purpose PRNG

BoardLab C11 owns the canonical implementation of
PCG-XSH-RR 64/32.

- State: unsigned 64-bit integer.
- Multiplier: 6364136223846793005.
- Increment: `(initseq << 1) | 1`, modulo 2^64.
- Initialization: zero state, set increment, advance once,
  add initstate, advance again.
- Output: XSH-RR rotation of the previous state.
- Bounded draws: rejection sampling without modulo bias.
- Invalid bounded requests do not advance the state.

Canonical implementation:
`src/boardlab_c11/rng.c`.

Canonical general-purpose test:
`boardlab-c11-rng`.

## 2. RPG Manager Calcio stream identity

A stream is derived from:

- WorldSeed: unsigned 64-bit integer.
- GeneratorVersion: unsigned 32-bit integer.
- RulesVersion: unsigned 32-bit integer.
- Domain: integer 1 through 6.
- EntityId: unsigned 64-bit integer.

The domain assignments are:

| Domain | Value |
|---|---:|
| WORLD | 1 |
| ATTRIBUTES | 2 |
| CALENDAR | 3 |
| MATCH | 4 |
| AI | 5 |
| RPG | 6 |

MATCH requires a nonzero entity identifier.

WORLD, ATTRIBUTES and CALENDAR require EntityId zero.

AI and RPG permit entity-specific identifiers, including
zero for their global stream.

## 3. Derivation v1

All arithmetic is unsigned and modulo 2^64.

The SplitMix64 finalizer is:

1. `x ^= x >> 30`
2. `x *= 0xbf58476d1ce4e5b9`
3. `x ^= x >> 27`
4. `x *= 0x94d049bb133111eb`
5. `x ^= x >> 31`

The precise sequence of mixing operations and constants
is defined by `rmc_stream_init()` in
`src/rpg_manager_calcio/streams.c`.

The derivation must not depend on global state, execution
order, text labels, memory addresses or system time.

Unsupported versions and invalid arguments must be
rejected without modifying the destination state.

## 4. Canonical vectors

Eight reference cases cover:

- World seed zero.
- Ordinary world seed.
- Maximum unsigned 64-bit seed.
- Calendar stream.
- Two independent match streams.
- Entity-specific AI stream.
- Entity-specific RPG stream.

The canonical expected initial states, increments and
first three draws are recorded in:

`src/rpg_manager_calcio/test_stream_vectors.c`.

Changing any expected result requires an explicit
determinism contract revision.

## 5. Qualification

The following CTest cases are mandatory:

- `boardlab-c11-rng`
- `rmc-streams`
- `rmc-stream-vectors`

The first belongs to the shared BoardLab C11 library.
The other two belong to RPG Manager Calcio.

The current qualification establishes PRNG and stream
properties only. It does not establish complete universe,
match or season replay determinism.

## 6. Compatibility policy

GeneratorVersion and RulesVersion are part of the
persistent identity of the generated world.

Changes to PRNG arithmetic, derivation constants,
domain values or initialization order must not silently
alter an existing version.

Unsupported versions must be rejected explicitly.
