# Quiz 02 — Terminazione, strategia e coordinamento

[English](quiz-02-termination-strategy-coordination.md) | [Italiano](quiz-02-termination-strategy-coordination.it.md)

## Ambito

Questo quiz copre:

- Unità 04 — Stati terminali e risultato;
- Unità 05 — Giocatore e strategia;
- Unità 06 — Coordinamento della partita e confini delle responsabilità.

Rispondi a ogni domanda prima di consultare il successivo materiale di review.

## Domanda 1 — Terminalità rispetto alla valutazione

Una strategia considera un `GameState` ancora in corso molto favorevole per
North.

Le regole del gioco consentono ancora diverse mosse legali.

Quale affermazione è più accurata?

A. Lo stato deve essere terminale perché possiede già una valutazione forte.

B. Valutazione strategica e rilevamento dello stato terminale sono concetti
   differenti.

C. La strategia dovrebbe terminare immediatamente la partita.

D. `Player` dovrebbe decidere se lo stato sia terminale.

## Domanda 2 — Risultato relativo al giocatore

Uno stato terminale rappresenta una vittoria per North e una sconfitta per
South.

Cosa dimostra più chiaramente questo esempio?

A. Uno stesso stato terminale può avere significato differente in base alla
   prospettiva del giocatore.

B. Ogni giocatore dovrebbe ricevere la stessa etichetta di risultato.

C. Il risultato appartiene interamente alla CLI.

D. Uno stato terminale deve contenere una strategia per ogni giocatore.

## Domanda 3 — Giocatore rispetto alla strategia

Una partita utilizza queste associazioni:

- North usa First Legal;
- South usa Random Legal.

Una seconda partita scambia le due strategie senza modificare le regole del
gioco.

Quale proprietà architetturale rende possibile questa configurazione?

A. Identità del giocatore e comportamento della strategia sono responsabilità
   separate.

B. Ogni giocatore possiede permanentemente una sola implementazione di
   strategia.

C. `Match` implementa internamente entrambe le strategie.

D. Le regole del gioco dipendono dalla strategia assegnata.

## Domanda 4 — Confine della strategia

Una strategia generica di ricerca contiene questa regola concreta:

"Lo spazio 2 può essere selezionato soltanto quando quella cella del tabellone è
vuota."

Qual è il problema principale?

A. La strategia contiene conoscenza di legalità specifica di un gioco.

B. La strategia dovrebbe invece conservare la regola in una variabile globale.

C. `Player` dovrebbe duplicare la stessa regola.

D. Ogni strategia generica deve conoscere layout concreti dei tabelloni.

## Domanda 5 — Responsabilità di Match

Durante un turno, quale responsabilità appartiene più appropriatamente a
`Match`?

A. Decidere se un particolare spazio sia legale secondo le regole di uno
   specifico gioco.

B. Scegliere direttamente la mossa usando logica Minimax.

C. Coordinare giocatore attivo, strategia associata, mossa selezionata,
   transizione di stato e controllo terminale.

D. Definire come vengono rappresentati i pezzi di un gioco concreto.

## Domanda 6 — Confine delle dipendenze

Viene aggiunto un nuovo gioco concreto a BoardLab.

Gli sviluppatori scoprono di dover modificare la logica generica di `Match`
perché contiene assunzioni sul layout del tabellone del gioco precedente.

Qual è il segnale architetturale più forte?

A. Il nuovo gioco dovrebbe essere rimosso.

B. `Match` ha assorbito conoscenza del gioco concreto e non è più
   sufficientemente generico.

C. Ogni nuovo gioco dovrebbe richiedere modifiche all'orchestrazione generica.

D. La CLI dovrebbe possedere il layout del tabellone.

## Registrazione del tentativo

La presenza di questo quiz non significa che l'assessment sia stato completato.

Il learner dovrebbe considerarlo affrontato soltanto dopo aver risposto
realmente a tutte e sei le domande.
