# BoardLab

[English](README.md) | [Italiano](README.it.md)

BoardLab is an open-source laboratory for studying game engines,
search algorithms, and artificial intelligence applied to board games.

It is not a collection of games.

Each game is introduced as a controlled experiment to understand,
implement, and compare computer science algorithms.

## Principles

- the engine does not know about specific games;
- algorithms depend only on abstract contracts;
- engine, game, and strategy remain separate;
- the CLI comes before any graphical interface;
- every algorithm produces measurable statistics;
- readability takes precedence over premature optimization;
- every component must be understandable, testable, and replaceable.

## Conceptual architecture

```text
Match Runner
        |
 +------+------+
 |             |
Game       Strategy
 |             |
GameState  Evaluator
```

## Roadmap

1. generic engine;
2. original game Tre Sigilli;
3. random strategy;
4. Minimax;
5. Alpha-Beta Pruning;
6. Monte Carlo Tree Search;
7. more complex games and experiments.

## Toolchain

- Python 3.12 or later;
- `uv` for environments, dependencies, and the lock file;
- Ruff for formatting and static analysis;
- mypy in strict mode for type contracts;
- pytest for automated tests.

## Code structure

```text
src/boardlab/
    engine/       contratti e coordinamento generico
    games/        implementazioni dei giochi
    cli/          interfaccia a riga di comando

tests/            test automatici
benchmarks/       esperimenti riproducibili
examples/         esempi di utilizzo
docs/             architettura, algoritmi e tutorial
```

## Learning / Build / Ship

The [learning and product map](docs/learning-product-map.md)
connects study, implementation, and potential mobile publication.

## Learning path

The structured learning path is available in
[Study sessions](sessions/README.md).

Material preparation and active study are tracked separately in
[Learning progress](docs/progress.md).

## Documentation

- [Architecture overview](docs/architecture/overview.md)
- [ADR 0001 — Python as the initial language](docs/architecture/adr/0001-python-toolchain.md)
- [Development guide](docs/development.md)
- [Roadmap](docs/roadmap.md)

Some legacy documents linked above are currently available in Italian.
Their English counterparts are tracked in issue #33.

## Status

BoardLab includes a generic Python engine, the Tre Sigilli and
Reversi 6×6 games, and Random, Minimax, Alpha-Beta, and MCTS strategies.
Reversi benchmarks are reproducible and documented.

Tre Sigilli includes Python/Kotlin parity contracts,
a platform-independent Kotlin controller, and an Android
Jetpack Compose application verified through a complete match
on a real device.

CI produces debug and unsigned release APKs; signing and
publication have not been completed.

Software development (BUILD), learning progress (LEARN), and
product validation (SHIP) are tracked separately.
Session 01 is prepared but has not yet been studied.

See the [roadmap](docs/roadmap.md),
[LEARN/BUILD/SHIP map](docs/learning-product-map.md), and
[architecture lessons](docs/lessons-learned/README.md).
