# Quiz 01 — Answer key: Stato, legalità e transizioni

[English](quiz-01-state-legality-transitions.md) | [Italiano](quiz-01-state-legality-transitions.it.md)

Questo materiale di review corrisponde a:

`../quizzes/quiz-01-state-legality-transitions.md`

Dovrebbe essere consultato soltanto dopo un tentativo reale del quiz.

## Domanda 1

Risposta attesa:

B.

Ragionamento:

Il tabellone visibile non è sufficiente a descrivere la situazione completa
perché il gameplay dipende anche da quale giocatore sia attivo.

Se questa informazione esiste soltanto in una variabile globale esterna a
`GameState`, due stati apparentemente identici possono comportarsi in modo
diverso in base a contesto nascosto.

Questo viola il requisito della Sessione 01 secondo cui uno stato deve essere
completo rispetto alle regole del gameplay che dipendono da esso.

Misconception tipica:

"Se il programma può accedere all'informazione da qualche parte, non serve che
faccia parte dello stato."

Perché non funziona:

Ricerca, testing, replay e ragionamento richiedono una descrizione esplicita
della situazione corrente invece di una fotografia il cui significato dipende
da contesto mutabile estraneo.

Takeaway pratico:

Le informazioni rilevanti per il gameplay che influenzano ciò che può accadere
successivamente appartengono al modello dello stato o a un'altra relazione di
dominio esplicita, non a contesto globale invisibile.

## Domanda 2

Risposta attesa:

C.

Ragionamento:

Il successore A e il successore B rappresentano futuri ipotetici differenti
derivati dallo stesso stato iniziale.

Modificare un successore non deve modificare inaspettatamente l'altro.

Se i due rami condividono dati mutabili del gameplay, non possono essere
ragionati indipendentemente.

Misconception tipica:

"Variabili separate significano automaticamente stati separati."

Perché non funziona:

Due oggetti possono comunque fare riferimento alle stesse strutture mutabili
annidate.

Takeaway pratico:

La ricerca richiede stati ipotetici utilizzabili indipendentemente a livello di
comportamento osservabile del dominio.

## Domanda 3

Risposta attesa:

B.

Ragionamento:

Il valore `1` descrive un'azione candidata, ma la sua legalità dipende da:

- stato corrente del gioco;
- regole del gioco.

Nello stato indicato, lo spazio 1 è già occupato.

La sola rappresentazione della mossa non può determinare se sia consentita.

Misconception tipica:

"Se un valore `Move` può essere costruito, allora è legale."

Perché non funziona:

La rappresentabilità descrive come viene espressa un'azione. La legalità è una
decisione di dominio relativa a uno specifico stato.

Takeaway pratico:

Mantieni separata la rappresentazione della mossa dalla legalità definita dalle
regole.

## Domanda 4

Risposta attesa:

B.

Ragionamento:

La regola concreta sugli spazi vuoti appartiene al dominio del gioco.

Duplicarla dentro strategie differenti rende gli algoritmi decisionali generici
dipendenti da un gioco concreto e crea più copie concorrenti della stessa
regola.

Una futura modifica può quindi portare le strategie a essere in disaccordo.

Misconception tipica:

"Ogni strategia dovrebbe validare autonomamente le mosse per sicurezza."

Perché non funziona:

La validazione può essere utile, ma la legalità autorevole specifica del gioco
non deve essere reimplementata indipendentemente dentro ogni strategia.

Takeaway pratico:

Le strategie generiche dovrebbero consumare le scelte legali attraverso il
contratto pubblico del gioco invece di possedere regole concrete.

## Domanda 5

Risposta attesa:

C.

Ragionamento:

Vengono controllate due proprietà differenti:

1. se la mossa selezionata da North sia legale nello stato corrente;
2. se l'applicazione di quella mossa produca uno stato successore valido.

La prima proprietà può essere soddisfatta mentre la seconda fallisce.

Poiché le regole richiedono alternanza dei turni, un successore che indica
ancora North come giocatore attivo viola la semantica della transizione.

Misconception tipica:

"Un input legale garantisce automaticamente un output corretto."

Perché non funziona:

Il controllo di legalità valida l'azione candidata. La transizione deve comunque
applicare correttamente tutte le modifiche richieste allo stato.

Takeaway pratico:

Testa legalità della mossa e correttezza della transizione come proprietà di
dominio distinte.

## Domanda 6

Risposta attesa:

A.

Ragionamento:

La candidata B non viene più valutata dallo stesso stato originale perché la
valutazione della candidata A ha modificato quello stato.

I rami quindi si contaminano reciprocamente.

Il problema centrale è la mancata conservazione di un punto di partenza
indipendente per l'esplorazione ipotetica.

Misconception tipica:

"L'algoritmo può modificare temporaneamente lo stato corrente purché conosca
l'ordine di valutazione."

Perché non funziona:

La correttezza della ricerca non dovrebbe dipendere dall'ordine accidentale di
valutazione o da un rollback manuale perfetto di mutazioni nascoste.

Takeaway pratico:

Le transizioni ipotetiche devono preservare l'indipendenza dei rami affinché
ogni candidata venga valutata dallo stato previsto.

## Riepilogo della review

Quiz 01 verifica tre confini:

- `GameState` deve rappresentare esplicitamente la situazione rilevante per il
  gameplay;
- le `Move` legali vengono determinate dalle regole applicate a quello stato;
- applicare una mossa deve produrre uno stato successore completo, valido e
  utilizzabile indipendentemente.

Questi concetti costituiscono la base necessaria prima di ragionare su stati
terminali, strategie e coordinamento della partita.
