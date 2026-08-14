# Unità 02 — Mosse legali

[English](unit-02-legal-moves.md) | [Italiano](unit-02-legal-moves.it.md)

## Learning objectives

Dopo aver studiato questa unità, il learner dovrebbe essere in grado di:

- distinguere un'azione immaginabile da una mossa legale;
- spiegare perché la legalità dipende sia dalle regole sia dallo stato corrente;
- distinguere la rappresentazione della mossa dalla valutazione della legalità;
- spiegare perché una strategia dovrebbe ricevere o derivare soltanto scelte
  legali;
- riconoscere la duplicazione della logica delle regole come rischio
  architetturale.

## Riferimenti alle fonti

Riferimento BoardLab primario:

- `docs/architecture/overview.md`

Dichiarazioni architetturali rilevanti:

- il contratto del gioco deve permettere di ottenere le mosse legali;
- `Move` descrive una possibile azione senza imporre una rappresentazione comune
  a tutti i giochi;
- `Strategy` sceglie tra le mosse legali usando il contratto pubblico del gioco;
- il motore non deve contenere regole appartenenti a un gioco concreto.

Riferimento di supporto:

- `README.md`

BoardLab richiede che gli algoritmi dipendano da contratti astratti invece che
da giochi specifici.

Le spiegazioni e gli esempi seguenti sono materiale didattico originale
BoardLab.

## Modello mentale

Una mossa descrive un'azione candidata.

Le regole del gioco decidono se quella candidata è legale nello stato corrente.

Questa distinzione è importante:

Mossa
→ descrive quale azione viene considerata

Regole del gioco + stato corrente
→ decidono se quell'azione è attualmente consentita

La stessa rappresentazione di una mossa può quindi essere legale in uno stato e
illegale in un altro.

## Spiegazione tecnica

Le mosse legali derivano dalla combinazione di:

- regole del gioco;
- `GameState` corrente.

Una strategia non dovrebbe inventare una propria interpretazione della
legalità.

Se strategie diverse reimplementano indipendentemente le regole, possono
essere in disaccordo su quali mosse siano consentite.

La separazione delle responsabilità di BoardLab richiede invece che la legalità
specifica del gioco rimanga nel dominio del gioco.

La rappresentazione esatta di `Move` rimane specifica del gioco.

Un gioco può rappresentare una mossa tramite una cella di destinazione.

Un altro può richiedere posizione di partenza, posizione di arrivo, tipo di
azione o parametri aggiuntivi.

Il motore generico non dovrebbe costringere giochi non correlati a utilizzare
una forma artificiale comune per le mosse.

## Esempio originale

Continuiamo con il gioco inventato **Three Stones**.

Stato corrente:

- spazio 1: North;
- spazio 2: vuoto;
- spazio 3: South;
- prossimo giocatore: North.

Immaginiamo che le mosse siano rappresentate semplicemente dal numero dello
spazio selezionato.

Mossa candidata:

`2`

Questa mossa è legale perché lo spazio 2 è vuoto.

Mossa candidata:

`1`

Questa mossa non è legale perché lo spazio 1 è già occupato.

L'intero `1` rimane comunque un'azione candidata rappresentabile.

La sua illegalità deriva dalla valutazione dell'azione rispetto allo stato
corrente e alle regole.

## Perché la strategia non dovrebbe possedere la legalità

Supponiamo che una strategia casuale generi numeri da 1 a 3 e contenga il
proprio codice per controllare se uno spazio sia vuoto.

Successivamente una strategia Minimax implementa separatamente la stessa
regola.

Ora la stessa regola del gioco esiste almeno in tre punti:

- gioco;
- strategia casuale;
- strategia Minimax.

Una futura modifica della regola potrebbe aggiornare un'implementazione ma non
le altre.

Le strategie sarebbero quindi in disaccordo sul gioco stesso.

Questo viola l'architettura prevista da BoardLab.

Una strategia generica dovrebbe invece operare attraverso il contratto pubblico
del gioco.

## Errori comuni di modellazione

### Considerare legale ogni mossa rappresentabile

È possibile costruire un oggetto mossa e quindi il programma presume che
l'azione sia consentita.

Conseguenza:

la rappresentazione viene confusa con la validità del dominio.

### Duplicare le regole di legalità nelle strategie

Ogni algoritmo controlla indipendentemente regole specifiche del gioco.

Conseguenza:

le strategie diventano dipendenti dai giochi concreti e possono essere in
disaccordo tra loro.

### Far conoscere al motore generico regole concrete sulle mosse

Il match runner sa che una particolare cella del tabellone deve essere vuota.

Conseguenza:

il motore non è più generico.

### Calcolare la legalità senza uno stato completo

Il controllo dipende da informazioni nascoste che non fanno parte di
`GameState`.

Conseguenza:

lo stesso stato apparente può produrre insiemi incoerenti di mosse legali.

## Scenario problematico

Una strategia casuale afferma che la mossa X è legale.

Una strategia Minimax afferma che la stessa mossa X è illegale.

Entrambe dovrebbero giocare allo stesso gioco partendo dallo stesso stato.

Questo disaccordo suggerisce fortemente che la legalità specifica del gioco sia
penetrata nelle strategie invece di rimanere in un unico contratto di dominio
autorevole.

## Takeaway

Una mossa legale non è semplicemente un'azione rappresentabile.

È un'azione consentita dalle regole del gioco in uno specifico stato.

BoardLab dovrebbe mantenere questa conoscenza nel dominio del gioco affinché le
strategie generiche possano consumare un unico insieme autorevole di scelte
legali.

## Esercizio per il learner

Usando lo stato del gioco originale che hai descritto nell'Unità 01, inventa
tre mosse candidate:

- una chiaramente legale;
- una chiaramente illegale;
- una la cui legalità sarebbe impossibile da determinare se mancasse qualche
  informazione importante dallo stato.

Per ciascuna candidata, spiega quale parte dello stato e quale regola sono
necessarie per determinarne la legalità.

Non implementare ancora una classe mossa o una funzione di legalità.
