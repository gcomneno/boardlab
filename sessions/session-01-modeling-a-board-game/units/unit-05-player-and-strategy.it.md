# Unità 05 — Giocatore e strategia

[English](unit-05-player-and-strategy.md) | [Italiano](unit-05-player-and-strategy.it.md)

## Learning objectives

Dopo aver studiato questa unità, il learner dovrebbe essere in grado di:

- distinguere l'identità del giocatore dalla logica di selezione delle mosse;
- spiegare la responsabilità di `Player`;
- spiegare la responsabilità di `Strategy`;
- spiegare perché le strategie dovrebbero dipendere dal contratto pubblico del
  gioco;
- ragionare su strategie intercambiabili senza modificare le regole;
- riconoscere la presenza di regole specifiche del gioco dentro strategie
  generiche.

## Riferimenti alle fonti

Riferimento BoardLab primario:

- `docs/architecture/overview.md`

Dichiarazioni architetturali rilevanti:

- `Player` identifica uno dei partecipanti alla partita;
- `Strategy` sceglie una mossa tra quelle legali;
- le strategie usano soltanto il contratto pubblico del gioco;
- le strategie non devono conoscere Tre Sigilli o altri giochi concreti.

Riferimento di supporto:

- `README.md`

BoardLab richiede che motore, gioco e strategia rimangano separati e che gli
algoritmi dipendano soltanto da contratti astratti.

Le spiegazioni e gli esempi seguenti sono materiale didattico originale
BoardLab.

## Modello mentale

Un giocatore risponde alla domanda:

Chi partecipa?

Una strategia risponde alla domanda:

Come viene selezionata la prossima mossa di quel partecipante?

Sono responsabilità differenti.

La stessa identità di giocatore può essere associata a strategie differenti in
esperimenti diversi.

Allo stesso modo, la stessa strategia generica può potenzialmente essere usata
da giocatori e giochi differenti quando i contratti pubblici sono compatibili.

## Spiegazione tecnica

`Player` rappresenta l'identità del partecipante nel modello di gioco.

Nella futura implementazione, tale identità potrebbe essere rappresentata da un
identificatore, un valore simile a un enum, un oggetto di dominio o un altro
meccanismo semplice.

Questa unità non decide ancora la forma Python definitiva.

`Strategy` rappresenta il comportamento decisionale.

Concettualmente, una strategia necessita di informazioni pubbliche sufficienti
per:

- ispezionare lo stato rilevante;
- ottenere le scelte legali;
- selezionare una mossa;
- produrre eventualmente statistiche di ricerca esplicite nelle fasi
  successive.

La strategia non dovrebbe possedere le regole che determinano se le mosse siano
legali.

Non dovrebbe modificare direttamente lo stato secondo regole specifiche del
gioco.

Non dovrebbe decidere quando uno specifico gioco concreto sia terminato.

Queste responsabilità appartengono al dominio del gioco.

## Esempio originale

Continuiamo con il gioco inventato **Three Stones**.

Partecipanti:

- North;
- South.

Queste sono identità di giocatore.

Immaginiamo ora due possibili strategie:

**First Legal**

Sceglie la prima mossa legale restituita dal gioco.

**Random Legal**

Sceglie casualmente una mossa tra quelle legali restituite dal gioco.

North potrebbe usare First Legal mentre South usa Random Legal.

In un'altra partita, le strategie potrebbero essere scambiate.

Nessuna regola di Three Stones deve cambiare.

I giocatori rimangono i partecipanti.

Le strategie determinano come vengono selezionate le mosse per quei
partecipanti.

## Intercambiabilità delle strategie

Separare identità del giocatore e strategia permette esperimenti controllati.

Per esempio:

Partita A:

- North → First Legal;
- South → Random Legal.

Partita B:

- North → Random Legal;
- South → First Legal.

Le regole del gioco rimangono identiche.

Cambia soltanto il comportamento decisionale.

Questo è essenziale per i futuri esperimenti BoardLab di confronto tra
algoritmi.

Se identità del giocatore e logica della strategia fossero fuse, cambiare
algoritmo potrebbe richiedere modifiche al codice del partecipante o del
dominio.

## Confine del contratto pubblico

Una strategia generica dovrebbe dipendere dalle astrazioni esposte dal gioco.

Concettualmente, potrebbe aver bisogno di operazioni come:

- ottenere le mosse legali dallo stato corrente;
- produrre stati successori;
- determinare gli stati terminali;
- valutare gli stati dalla prospettiva di un giocatore.

Non dovrebbe contenere regole come:

"lo spazio 2 è legale soltanto quando è vuoto"

oppure:

"tre spazi occupati terminano Three Stones."

Queste sono regole concrete del gioco.

La strategia dovrebbe consumarne i risultati attraverso il contratto pubblico
del gioco.

## Errori comuni di modellazione

### Giocatore e strategia sono concettualmente lo stesso oggetto

Identità del partecipante e comportamento decisionale vengono fusi.

Conseguenza:

gli esperimenti con algoritmi differenti diventano più difficili da configurare
e comprendere.

### La strategia reimplementa la legalità

Un algoritmo generico controlla direttamente regole concrete del tabellone.

Conseguenza:

la strategia diventa dipendente da un gioco.

### La strategia modifica direttamente lo stato

La strategia cambia autonomamente pezzi o contatori.

Conseguenza:

la logica decisionale diventa anche logica di transizione.

### La strategia possiede le regole terminali

L'algoritmo decide se un gioco concreto è terminato.

Conseguenza:

la semantica della terminazione viene duplicata fuori dal dominio.

### La strategia presume una prospettiva universale

La valutazione favorisce implicitamente un partecipante fisso.

Conseguenza:

la stessa strategia non può ragionare chiaramente dalla prospettiva di un altro
giocatore.

## Scenario problematico

BoardLab possiede due strategie:

- Random Legal;
- Future Minimax.

Entrambe contengono una propria implementazione delle regole di legalità di un
gioco concreto.

Una regola cambia.

Random Legal viene aggiornata.

Future Minimax no.

Le due strategie ora producono insiemi differenti di mosse legali per lo stesso
stato.

Il difetto architetturale è la duplicazione della conoscenza del gioco dentro
le strategie.

## Takeaway

`Player` identifica chi partecipa.

`Strategy` determina come viene selezionata una mossa per un partecipante.

Mantenerli separati rende le strategie intercambiabili, permette esperimenti
controllati e impedisce agli algoritmi generici di diventare dipendenti dai
giochi concreti.

## Esercizio per il learner

Usando il tuo gioco originale:

1. identifica le due identità dei giocatori;
2. inventa due strategie differenti di selezione della mossa senza cambiare le
   regole;
3. descrivi come le strategie potrebbero essere scambiate tra i giocatori;
4. elenca due informazioni di cui una strategia necessita dal contratto del
   gioco;
5. fornisci un esempio di regola concreta che non deve essere implementata
   dentro la strategia.

Non implementare ancora classi di strategia.
