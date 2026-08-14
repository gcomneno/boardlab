# Quiz 02 — Answer key: Terminazione, strategia e coordinamento

[English](quiz-02-termination-strategy-coordination.md) | [Italiano](quiz-02-termination-strategy-coordination.it.md)

Questo materiale di review corrisponde a:

`../quizzes/quiz-02-termination-strategy-coordination.md`

Dovrebbe essere consultato soltanto dopo un tentativo reale del quiz.

## Domanda 1

Risposta attesa:

B.

Ragionamento:

Uno stato può essere strategicamente favorevole mentre il gioco è ancora in
corso.

Il rilevamento dello stato terminale risponde alla domanda se le regole dicano
che il gioco è terminato.

La valutazione strategica risponde alla domanda quanto uno stato appaia
desiderabile dal punto di vista decisionale.

I due concetti sono collegati ma distinti.

Misconception tipica:

"Una valutazione sufficientemente positiva o negativa significa che lo stato è
terminale."

Perché non funziona:

La valutazione può essere utile anche per stati non terminali, soprattutto
durante la ricerca.

Takeaway pratico:

Mantieni separata la terminazione definita dalle regole dalla valutazione
strategica.

## Domanda 2

Risposta attesa:

A.

Ragionamento:

Lo stesso `GameState` terminale rappresenta una singola situazione di gioco
conclusa, ma il suo risultato può essere interpretato diversamente in base alla
prospettiva del `Player`.

Una vittoria di North è contemporaneamente una sconfitta di South.

Misconception tipica:

"Uno stato terminale ha un'unica etichetta di risultato che significa la stessa
cosa per tutti i partecipanti."

Perché non funziona:

Vittoria e sconfitta sono intrinsecamente relative a un partecipante, salvo che
il gioco utilizzi una rappresentazione neutra del risultato interpretata in un
secondo momento.

Takeaway pratico:

Il ragionamento sul risultato deve preservare esplicitamente la prospettiva del
giocatore.

## Domanda 3

Risposta attesa:

A.

Ragionamento:

North e South identificano i partecipanti.

First Legal e Random Legal descrivono comportamenti di selezione della mossa.

Poiché queste responsabilità sono separate, le strategie possono essere
scambiate senza modificare le regole del gioco o le identità dei giocatori.

Misconception tipica:

"Un giocatore possiede permanentemente un solo algoritmo decisionale."

Perché non funziona:

Fondere identità e strategia rende inutilmente difficili esperimenti controllati
e confronti tra algoritmi.

Takeaway pratico:

Separa l'identità `Player` dal comportamento `Strategy` per mantenere gli
algoritmi intercambiabili.

## Domanda 4

Risposta attesa:

A.

Ragionamento:

L'affermazione secondo cui lo spazio 2 può essere scelto soltanto quando è vuoto
è una regola concreta del gioco.

Una strategia generica di ricerca dovrebbe ottenere le scelte legali attraverso
il contratto pubblico del gioco invece di conoscere direttamente tale regola.

Misconception tipica:

"Una strategia necessita delle regole concrete per prendere decisioni sicure."

Perché non funziona:

La strategia necessita di scelte legali autorevoli, non di una copia della
conoscenza usata dal gioco per determinarle.

Takeaway pratico:

Mantieni la semantica concreta nel dominio del gioco e la logica decisionale
generica dentro `Strategy`.

## Domanda 5

Risposta attesa:

C.

Ragionamento:

`Match` è responsabile dell'orchestrazione.

Coordina giocatore attivo, strategia associata, mossa selezionata, transizione
del dominio e controllo terminale.

Non dovrebbe possedere regole concrete del gioco o algoritmi decisionali.

Misconception tipica:

"Il componente che controlla il turno dovrebbe anche conoscere il funzionamento
di tutte le regole."

Perché non funziona:

Coordinare significa invocare responsabilità, non assorbirle.

Takeaway pratico:

Un `Match` generico dovrebbe rimanere un orchestratore sottile.

## Domanda 6

Risposta attesa:

B.

Ragionamento:

Se aggiungere un nuovo gioco richiede di modificare `Match` generico perché
conosce dettagli del vecchio layout, la conoscenza del gioco concreto è
penetrata nell'orchestrazione generica.

Questo rompe il confine delle dipendenze previsto da BoardLab.

Misconception tipica:

"L'orchestrazione generica necessita naturalmente di casi speciali per ogni
gioco."

Perché non funziona:

Lo scopo del contratto del gioco è precisamente mantenere queste regole
speciali dietro l'implementazione concreta.

Takeaway pratico:

Aggiungere un nuovo gioco dovrebbe richiedere principalmente
l'implementazione del contratto del gioco, non insegnare nuove regole del
tabellone a `Match`.

## Riepilogo della review

Quiz 02 verifica tre confini principali:

- terminalità e risultato appartengono alla semantica del dominio e devono
  rimanere distinti dalla valutazione strategica;
- identità `Player` e comportamento `Strategy` sono separati e
  intercambiabili;
- `Match` coordina il sistema senza assorbire regole concrete del gioco o
  algoritmi decisionali.

Insieme a Quiz 01, questi concetti coprono l'intero modello concettuale
preparato dalla Sessione 01.
