# Unità 03 — Transizioni di stato

[English](unit-03-state-transitions.md) | [Italiano](unit-03-state-transitions.it.md)

## Learning objectives

Dopo aver studiato questa unità, il learner dovrebbe essere in grado di:

- spiegare una transizione di stato come applicazione di una mossa legale a uno
  stato;
- distinguere una mossa dallo stato prodotto da quella mossa;
- spiegare perché gli stati successori devono preservare la validità del
  dominio;
- spiegare perché la ricerca richiede stati successori indipendenti;
- riconoscere effetti collaterali nascosti durante una transizione come difetto
  di modellazione.

## Riferimenti alle fonti

Riferimento BoardLab primario:

- `docs/architecture/overview.md`

Dichiarazioni architetturali rilevanti:

- il contratto concettuale del gioco deve permettere di applicare una mossa;
- una ricerca non deve modificare clandestinamente uno stato;
- il gioco deve poter produrre uno stato indipendente adatto alla ricerca;
- `GameState` rappresenta una configurazione di gioco completa e valida.

Riferimento di supporto:

- `README.md`

BoardLab richiede che le responsabilità di motore, gioco e strategia rimangano
separate e che i componenti rimangano comprensibili e testabili.

Le spiegazioni e gli esempi seguenti sono materiale didattico originale
BoardLab.

## Modello mentale

Il gameplay può essere visto come una sequenza di fotografie collegate da
azioni.

La forma di base è:

Stato corrente
→ mossa legale
→ stato successivo

La mossa descrive cosa accade.

La transizione descrive come l'applicazione di quella mossa modifica la
situazione completa del gioco.

Lo stato successivo non è la mossa stessa.

È una nuova descrizione di ciò che è vero dopo l'applicazione della mossa.

## Spiegazione tecnica

Una transizione di stato riceve due input concettuali:

- un `GameState` corrente valido;
- una `Move` legale.

Produce un `GameState` successore.

Quel successore deve rispettare le regole del gioco e contenere tutte le
informazioni necessarie per continuare la partita.

Per esempio, applicare una mossa può modificare:

- posizioni dei pezzi;
- proprietà;
- risorse rimanenti;
- giocatore attivo;
- fase o contatori;
- effetti che influenzano future mosse legali.

Una transizione appartiene quindi al dominio del gioco.

Una strategia generica non dovrebbe decidere come cambia lo stato.

Un coordinatore generico della partita non dovrebbe contenere regole concrete
che descrivono come una specifica mossa aggiorna uno specifico tabellone.

## Esempio originale

Continuiamo con il gioco inventato **Three Stones**.

Stato corrente:

- spazio 1: North;
- spazio 2: vuoto;
- spazio 3: South;
- next player: North.

Mossa legale:

North sceglie lo spazio 2.

Dopo l'applicazione della mossa, lo stato successore diventa:

- spazio 1: North;
- spazio 2: North;
- spazio 3: South;
- next player: South.

La mossa può essere rappresentata semplicemente come selezione dello spazio 2.

Lo stato successore contiene molte più informazioni della mossa.

Registra l'intera situazione di gioco risultante.

## Validità della transizione

Una mossa legale applicata a uno stato valido dovrebbe produrre un altro stato
valido.

Supponiamo invece che la transizione precedente produca:

- spazio 1: North;
- spazio 2: North;
- spazio 3: South;
- next player: North.

Se le regole impongono l'alternanza dei turni, questo successore violerebbe il
modello di gioco.

La mossa candidata potrebbe essere stata legale.

L'implementazione della transizione sarebbe comunque errata.

Legalità della mossa e correttezza della transizione sono aspetti collegati ma
distinti.

## Stati successori indipendenti

Gli algoritmi di ricerca esplorano spesso diverse mosse legali partendo dallo
stesso stato.

Concettualmente:

Stato corrente
→ candidata A
→ successore A

Stato corrente
→ candidata B
→ successore B

Successore A e successore B devono poter essere utilizzati indipendentemente.

Esplorare la candidata A non deve alterare lo stato dal quale verrà poi
derivata la candidata B.

BoardLab richiede quindi transizioni che preservino l'indipendenza tra rami
ipotetici della ricerca.

Questo requisito non prescrive ancora se la futura implementazione Python userà
oggetti immutabili, copie, strutture dati persistenti o un'altra tecnica.

Il comportamento osservabile richiesto è l'indipendenza.

## Errori comuni di modellazione

### Modificare inaspettatamente lo stato originale

Applicare una mossa ipotetica modifica lo stato fornito dal chiamante.

Conseguenza:

le operazioni successive non partono più dallo stato previsto.

### Restituire un successore incompleto

La transizione modifica il tabellone visibile ma dimentica di aggiornare il
giocatore attivo o un altro campo rilevante per il gameplay.

Conseguenza:

il successore non descrive una situazione completa.

### Far eseguire le transizioni alle strategie

Una strategia modifica direttamente pezzi o risorse secondo regole specifiche
del gioco.

Conseguenza:

la logica decisionale diventa dipendente dal gioco concreto.

### Confondere una mossa con il suo risultato

Il programma tratta la descrizione dell'azione come se contenesse già lo stato
successore completo.

Conseguenza:

le responsabilità tra rappresentazione dell'azione e transizione del dominio
diventano confuse.

## Scenario problematico

Una strategia valuta due mosse candidate.

Dopo aver valutato la prima candidata, lo stato corrente che avrebbe dovuto
rimanere originale è cambiato.

La seconda candidata viene quindi esplorata da un punto di partenza diverso.

La ricerca può produrre risultati incoerenti anche se l'algoritmo di ricerca è
logicamente corretto.

Il difetto sottostante è una mutazione incontrollata durante la transizione.

## Takeaway

Una transizione collega un `GameState` valido a un altro applicando una `Move`
legale.

Il successore deve essere completo, valido e sufficientemente indipendente per
il gioco successivo o per la ricerca ipotetica.

Il dominio del gioco possiede questa trasformazione.

## Esercizio per il learner

Usando il gioco originale descritto negli esercizi precedenti:

1. scrivi uno stato corrente valido;
2. scegli una mossa legale;
3. descrivi lo stato successore completo dopo l'applicazione della mossa;
4. elenca tutti i campi che sono cambiati;
5. spiega un bug che potrebbe verificarsi se stato originale e stato successore
   condividessero accidentalmente dati di gameplay mutabili.

Non implementare ancora la transizione in Python.
