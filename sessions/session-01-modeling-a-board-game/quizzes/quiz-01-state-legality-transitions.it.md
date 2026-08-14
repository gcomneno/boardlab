# Quiz 01 — Stato, legalità e transizioni

[English](quiz-01-state-legality-transitions.md) | [Italiano](quiz-01-state-legality-transitions.it.md)

## Ambito

Questo quiz copre:

- Unità 01 — Stato del gioco;
- Unità 02 — Mosse legali;
- Unità 03 — Transizioni di stato.

Rispondi a ogni domanda prima di consultare il successivo materiale di review.

## Domanda 1 — Stato completo

Un gioco inventato per due giocatori utilizza un tabellone di tre celle.

Il tabellone visibile è:

- cella 1: North;
- cella 2: vuota;
- cella 3: South.

Il giocatore attivo è conservato soltanto in una variabile globale esterna a
`GameState`.

Quale affermazione descrive meglio il problema di modellazione?

A. Non esiste alcun problema perché il programma può comunque leggere il
   giocatore attivo.

B. Lo stato è incompleto perché il comportamento del gameplay dipende da
   informazioni non contenute nello stato.

C. Lo stato non è valido perché ogni stato valido deve contenere esattamente tre
   campi.

D. Il giocatore attivo appartiene a `Strategy`, non allo stato del gioco.

## Domanda 2 — Stati ipotetici indipendenti

Una ricerca esplora due mosse candidate partendo dallo stesso `GameState`
corrente.

Dopo aver generato il successore A, modificare uno dei suoi pezzi modifica
inaspettatamente anche il pezzo corrispondente nel successore B.

Quale proprietà architetturale è stata violata più chiaramente?

A. L'identità del giocatore deve essere immutabile.

B. Ogni mossa deve avere una rappresentazione testuale unica.

C. Gli stati successori ipotetici devono poter essere utilizzati
   indipendentemente.

D. Una strategia deve scegliere sempre le mosse casualmente.

## Domanda 3 — Rappresentabile rispetto a legale

In **Three Stones**, le mosse sono rappresentate dal numero dello spazio
selezionato.

Stato corrente:

- spazio 1: North;
- spazio 2: vuoto;
- spazio 3: South;
- next player: North.

Il valore `1` è una mossa candidata rappresentabile.

Quali informazioni aggiuntive sono necessarie per determinare se possa essere
effettivamente giocata?

A. Soltanto il tipo Python usato per rappresentare la mossa.

B. Lo stato corrente insieme alle regole del gioco.

C. La strategia che ha generato la candidata.

D. Il comando CLI usato per avviare la partita.

## Domanda 4 — Responsabilità della legalità

BoardLab acquisisce una strategia Random Legal e una futura strategia di
ricerca.

Entrambe implementano separatamente la regola concreta secondo cui una mossa
può scegliere soltanto uno spazio vuoto del tabellone.

Qual è il principale problema architetturale?

A. Ogni strategia dovrebbe invece usare una rappresentazione differente di
   `Move`.

B. La conoscenza delle regole concrete del gioco è stata duplicata dentro
   algoritmi decisionali generici.

C. Le strategie dovrebbero conservare globalmente il giocatore attivo.

D. `Match` dovrebbe contenere una terza copia della stessa regola.

## Domanda 5 — Mossa legale, successore non valido

Lo stato corrente di un gioco è valido e la mossa selezionata è legale.

Le regole richiedono che i turni alternino North e South.

Dopo la mossa legale di North, lo stato successore prodotto indica ancora North
come giocatore attivo.

Quale affermazione caratterizza meglio il difetto?

A. Una mossa legale garantisce che ogni possibile rappresentazione dello stato
   successore sia valida.

B. La rappresentazione della mossa è necessariamente errata.

C. La legalità della mossa è stata verificata, ma la transizione ha prodotto
   uno stato che viola le regole del gioco.

D. La strategia dovrebbe correggere il giocatore attivo prima di restituire la
   mossa.

## Domanda 6 — Contaminazione dei rami di ricerca

Una ricerca valuta la candidata A applicandola direttamente allo stato corrente.

Successivamente valuta la candidata B utilizzando lo stesso oggetto, che
contiene ancora le modifiche prodotte durante l'esame della candidata A.

Dove si trova il problema fondamentale?

A. Il processo di transizione non ha preservato uno stato iniziale indipendente
   per i rami ipotetici.

B. La candidata B dovrebbe essere valutata sempre prima della candidata A.

C. `Player` dovrebbe ripristinare lo stato dopo ogni chiamata alla strategia.

D. Gli algoritmi di ricerca richiedono conoscenza concreta del layout del
   tabellone.

## Registrazione del tentativo

La presenza di questo file non deve marcare come completato alcun progresso nel
repository.

Il learner dovrebbe considerare l'assessment affrontato soltanto dopo aver
risposto realmente a tutte e sei le domande.
