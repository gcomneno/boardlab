# Sessione 01 — Modellare un gioco da tavolo come sistema di transizione degli stati

[English](README.md) | [Italiano](README.it.md)

## Stato

Preparazione repository: **Prepared**.

Studio attivo: **not studied**.

## Scopo

Questa sessione costruisce il modello computazionale necessario per ragionare
sui giochi da tavolo prima di implementare un gioco concreto o un algoritmo di
ricerca.

L'idea centrale è descrivere il gioco tramite stati, mosse legali, transizioni
di stato, condizioni terminali, giocatori, strategie e coordinamento della
partita.

## Relazione con BoardLab

La sessione prepara i concetti già introdotti dall'architettura di BoardLab:

- `Game`;
- `GameState`;
- `Move`;
- `Player`;
- `Strategy`;
- `Match`.

Non ne definisce ancora le interfacce Python definitive e non avvia il lavoro
di implementazione attiva.

## Preparazione e orientamento

Inizia dai documenti che definiscono ambito e ordine didattico:

1. [Source coverage map](source-coverage-map.it.md)
2. [Study map](study-map.it.md)
3. [Quiz plan](quiz-plan.it.md)

La source coverage map definisce ciò che la sessione può affermare e ciò che
rimane rimandato.

La study map definisce la dipendenza concettuale tra le unità.

Il quiz plan definisce la copertura dell'assessment prima che inizi
l'assessment del learner.

## Unità didattiche

Studia le unità in ordine:

1. [Unità 01 — Stato del gioco](units/unit-01-game-state.it.md)
2. [Unità 02 — Mosse legali](units/unit-02-legal-moves.it.md)
3. [Unità 03 — Transizioni di stato](units/unit-03-state-transitions.it.md)
4. [Unità 04 — Stati terminali e risultato](units/unit-04-terminal-states-and-outcome.it.md)
5. [Unità 05 — Giocatore e strategia](units/unit-05-player-and-strategy.it.md)
6. [Unità 06 — Coordinamento della partita e confini delle responsabilità](units/unit-06-match-coordination.it.md)

La presenza di queste unità significa che il materiale didattico è stato
preparato.

Non significa che sia stato studiato.

## Assessment

La sessione fornisce due quiz destinati al learner:

1. [Quiz 01 — Stato, legalità e transizioni](quizzes/quiz-01-state-legality-transitions.it.md)
2. [Quiz 02 — Terminazione, strategia e coordinamento](quizzes/quiz-02-termination-strategy-coordination.it.md)

I file dei quiz intenzionalmente non contengono le relative risposte.

## Materiale di review

Consulta le answer key soltanto dopo un tentativo reale del quiz:

1. [Answer key Quiz 01](answer-keys/quiz-01-state-legality-transitions.it.md)
2. [Answer key Quiz 02](answer-keys/quiz-02-termination-strategy-coordination.it.md)

Creare o pubblicare le answer key non conta come review effettuata dal learner.

## Coverage review

L'audit della preparazione è documentato in:

- [Coverage review](coverage-review.it.md)

La coverage review conclude attualmente che la preparazione dei contenuti è
completa.

Questa conclusione è distinta dallo stato finale **Prepared** del repository.

## Materiale rimandato

Tre Sigilli e gli algoritmi di ricerca concreti rimangono deliberatamente
rimandati oltre questa sessione fondamentale.

La sessione non introduce ancora:

- interfacce Python definitive del dominio;
- implementazione concreta del gioco;
- implementazione della strategia casuale;
- Minimax;
- Alpha-Beta Pruning;
- Monte Carlo Tree Search;
- benchmark prestazionali.

## Stato dello studio

Per questa sessione non è stato ancora completato alcun esercizio, quiz, analisi
di partita, review delle risposte o altra attività del learner.

Preparazione del repository e studio del learner rimangono stati separati.
