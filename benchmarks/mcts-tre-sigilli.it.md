# MCTS — Benchmark Tre Sigilli

[English](mcts-tre-sigilli.md) | [Italiano](mcts-tre-sigilli.it.md)

## Metodo

Il benchmark confronta MCTS con Random, Minimax e Alpha-Beta
utilizzando il coordinatore `Match` già presente.

- Gioco: Tre Sigilli
- MCTS: 40 simulazioni per mossa, esplorazione UCT predefinita
- Minimax: profondità 1, valutatore territoriale
- Alpha-Beta: profondità 1, stesso valutatore territoriale
- Random: selezione pseudocasuale con seed
- Seed: da 0 a 4
- Ruoli: NORTH e SOUTH per ogni seed
- Partite: 10 per avversario, 30 complessive
- Riproducibilità: verificata ripetendo gli esperimenti
- Implementazione: `tests/test_mcts_benchmark.py`

## Risultati

I risultati sono espressi dal punto di vista di MCTS.

| Avversario | Vittorie | Pareggi | Sconfitte | Turni medi |
| --- | ---: | ---: | ---: | ---: |
| Random | 10 | 0 | 0 | 18.00 |
| Minimax (profondità 1) | 3 | 2 | 5 | 18.00 |
| Alpha-Beta (profondità 1) | 3 | 2 | 5 | 18.00 |

## Interpretazione

MCTS ha vinto tutte le partite del campione contro Random.

Contro Minimax e Alpha-Beta a profondità 1, MCTS ha registrato
più sconfitte che vittorie. I risultati aggregati identici delle
due strategie deterministiche sono coerenti con il loro obiettivo
di valutazione condiviso.

Queste osservazioni riguardano esclusivamente l'esperimento descritto.

## Limiti

- Dieci partite per avversario non consentono conclusioni
  statisticamente robuste.
- Quaranta simulazioni MCTS e una ricerca deterministica a profondità 1
  non rappresentano budget computazionali equivalenti.
- Non sono stati confrontati tempi di esecuzione o nodi visitati.
- Tutte le partite utilizzano un solo gioco e un insieme fisso di seed.
- Il benchmark verifica riproducibilità e completamento legale,
  senza imporre una quantità minima di vittorie.

Esperimenti futuri potranno ampliare il campione e confrontare
le strategie con budget computazionali misurati.
