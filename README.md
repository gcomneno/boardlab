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

## Stato

Il progetto è nella fase iniziale di definizione del dominio.

Nessun algoritmo è ancora stato implementato.

Consulta [la panoramica architetturale](docs/architecture/overview.md).
