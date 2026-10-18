# BoardLab

BoardLab è un laboratorio open source per lo studio di motori di gioco,
algoritmi di ricerca e intelligenza artificiale applicata ai giochi da tavolo.

Non è una collezione di giochi.

Ogni gioco viene introdotto come esperimento controllato per comprendere,
implementare e confrontare algoritmi di Computer Science.

## Principi

- il motore non conosce giochi specifici;
- gli algoritmi dipendono solo da contratti astratti;
- motore, gioco e strategia restano separati;
- la CLI precede qualsiasi interfaccia grafica;
- ogni algoritmo produce statistiche misurabili;
- la leggibilità viene prima dell'ottimizzazione prematura;
- ogni componente deve essere comprensibile, testabile e sostituibile.

## Architettura concettuale

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

1. motore generico;
2. gioco originale Tre Sigilli;
3. strategia casuale;
4. Minimax;
5. Alpha-Beta Pruning;
6. Monte Carlo Tree Search;
7. giochi ed esperimenti più complessi.

## Toolchain

- Python 3.12 o successivo;
- `uv` per ambiente, dipendenze e lock file;
- Ruff per formattazione e analisi statica;
- mypy in modalità strict per i contratti di tipo;
- pytest per i test automatici.

## Struttura del codice

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

La [mappa di apprendimento e prodotto](docs/learning-product-map.it.md) collega studio, implementazione e possibile pubblicazione mobile.

## Percorso didattico

Il percorso di studio strutturato è disponibile in
[Sessioni di studio](sessions/README.it.md).

La preparazione del materiale e lo studio attivo vengono tracciati
separatamente in [Progresso didattico](docs/progress.it.md).

## Documentazione

- [Panoramica architetturale](docs/architecture/overview.md)
- [ADR 0001 — Python come linguaggio iniziale](docs/architecture/adr/0001-python-toolchain.md)
- [Guida allo sviluppo](docs/development.md)
- [Roadmap](docs/roadmap.md)

## Stato

BoardLab dispone di un motore Python generico, dei giochi Tre Sigilli
e Reversi 6×6 e delle strategie Random, Minimax, Alpha-Beta e MCTS.
I benchmark Reversi sono riproducibili e documentati.

Per Tre Sigilli sono disponibili contratti di parità Python/Kotlin,
un controller Kotlin indipendente dalla piattaforma e un'app Android
Jetpack Compose verificata in una partita completa su dispositivo reale.
La CI produce APK debug e release unsigned; la firma e la pubblicazione
non sono state completate.

Lo sviluppo software (BUILD), il progresso didattico (LEARN) e la
validazione di prodotto (SHIP) sono tracciati separatamente.
La Sessione 01 è preparata, ma non risulta studiata.

Consulta la [roadmap](docs/roadmap.md), la
[mappa LEARN/BUILD/SHIP](docs/learning-product-map.it.md) e le
[lezioni architetturali](docs/lessons-learned/README.it.md).
