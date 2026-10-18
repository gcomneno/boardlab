# Roadmap

## Fase 0 — Motore generico

Definizione di Game, GameState, Move, Player, Strategy e Match.

## Fase 1 — Tre Sigilli

Primo gioco originale e strategia casuale.

## Fase 2 — Minimax

Implementazione leggibile, testata e strumentata.

## Fase 3 — Alpha-Beta Pruning

Confronto con Minimax su qualità della mossa, nodi e tempo.

## Fase 4 — Monte Carlo Tree Search

Studio di selezione, espansione, simulazione e backpropagation.

## Fase 5 — Scalabilità

Introduzione di un gioco con spazio degli stati e meccaniche più articolate.

## Estensione completata — Benchmark Reversi 6×6

Implementazione di un protocollo riproducibile per confrontare Minimax, Alpha-Beta e MCTS su Reversi 6×6, con harness, test e risultati documentati.

Completata tramite PR #7. Questa estensione non coincide con la proposta di distribuzione mobile descritta nella mappa LEARN/BUILD/SHIP.

## Stato dell'estensione Android

L'estensione SHIP ha prodotto contratti di parità Python/Kotlin,
un'implementazione Kotlin del dominio Tre Sigilli, un controller
indipendente dalla piattaforma e un'interfaccia Jetpack Compose.
Sono state verificate una partita completa su dispositivo reale,
le build Android in CI e la produzione di un APK release unsigned.

Evidenze: PR #8–#13. Le lezioni architetturali sono state
documentate tramite PR #14.

Questi risultati non modificano retroattivamente le fasi della
roadmap originale e non implicano pubblicazione o readiness produttiva.
Restano da qualificare test Android automatizzati, firma delle release,
gestione delle chiavi, ciclo di vita, accessibilità e distribuzione.

Lo studio attivo è tracciato separatamente in
[Progresso didattico](progress.it.md).
