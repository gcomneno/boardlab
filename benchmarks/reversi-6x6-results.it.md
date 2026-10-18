# Reversi 6×6 — Risultati del pilota

[English](reversi-6x6-results.md) · [Italiano](reversi-6x6-results.it.md) ·
[Protocollo](reversi-6x6.it.md) ·
[Risultati leggibili dalle macchine](reversi-6x6-results.json)

## Configurazione sperimentale

- Avversario di riferimento: Random.
- Strategie candidate: Minimax, Alpha-Beta, MCTS.
- Seed: 0, 1, 2, 3, 4.
- Entrambi i ruoli del candidato: BLACK e WHITE.
- Minimax e Alpha-Beta: profondità 2, valutazione disc-difference.
- MCTS: 32 simulazioni per decisione.
- Dieci partite per candidato; 30 partite complessive.
- Commit sorgente: `3296607`.

## Esiti

Tutti gli esiti sono espressi dal punto di vista del candidato.

| Strategia | Partite | Vittorie | Pareggi | Sconfitte |
|---|---:|---:|---:|---:|
| Minimax | 10 | 7 | 0 | 3 |
| Alpha-Beta | 10 | 7 | 0 | 3 |
| MCTS | 10 | 10 | 0 | 0 |

## Costi osservati

| Strategia | Azioni totali | Secondi trascorsi | Nodi visitati | Foglie valutate |
|---|---:|---:|---:|---:|
| Minimax | 331 | 0.552 | 4668 | 3778 |
| Alpha-Beta | 331 | 0.381 | 2929 | 2039 |
| MCTS | 330 | 8.979 | N/A | N/A |

Il tempo trascorso è una misura osservazionale, dipendente
dall'ambiente, e non costituisce un criterio deterministico di
riproducibilità. Le azioni comprendono i turni di entrambi i
partecipanti. Le statistiche di ricerca riguardano soltanto il
candidato. Le simulazioni MCTS non equivalgono ai nodi visitati
da Minimax o Alpha-Beta: i budget di ricerca non sono direttamente
confrontabili.

## Interpretazione e limiti

MCTS ha ottenuto il maggior numero di vittorie in questo pilota
contro Random. È un'osservazione descrittiva, non una dimostrazione
di superiorità generale dell'algoritmo. Il campione comprende
soltanto dieci partite per candidato, un avversario di riferimento,
una dimensione della scacchiera e parametri di ricerca fissi.
L'esperimento non è un torneo completo tra tutte le strategie
e non stabilisce budget computazionali equivalenti.

Esiti, azioni e statistiche di ricerca applicabili possono essere
verificati rieseguendo il medesimo protocollo. I tempi possono
variare tra esecuzioni e macchine.

SHA-256 dei risultati: `a6fedd8f05605ba36609edc15b5b6f86be9697f1639707a17bb89fa49c45ec87`.
