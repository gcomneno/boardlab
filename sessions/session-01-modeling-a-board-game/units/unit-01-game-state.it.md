# Unità 01 — Stato del gioco

[English](unit-01-game-state.md) | [Italiano](unit-01-game-state.it.md)

## Learning objectives

Dopo aver studiato questa unità, il learner dovrebbe essere in grado di:

- distinguere un gioco da una sua particolare situazione;
- identificare le informazioni necessarie per descrivere uno stato completo;
- spiegare perché la validità dello stato è una responsabilità del dominio;
- spiegare perché stati indipendenti sono importanti per ricerca e simulazione;
- riconoscere il contesto mutabile nascosto come rischio di modellazione.

## Riferimenti alle fonti

Riferimento BoardLab primario:

- `docs/architecture/overview.md`

Dichiarazioni architetturali rilevanti:

- `GameState` rappresenta una configurazione completa e valida della partita;
- la ricerca non deve modificare clandestinamente uno stato;
- il contratto concettuale del gioco deve supportare stati indipendenti
  utilizzabili durante la ricerca.

Riferimento di progetto di supporto:

- `README.md`

BoardLab richiede che i componenti rimangano comprensibili, testabili e
sostituibili.

Le spiegazioni e gli esempi seguenti sono materiale didattico originale
BoardLab.

## Modello mentale

Un gioco è l'insieme delle regole che definisce cosa può accadere.

Uno stato del gioco è una fotografia completa di ciò che è vero in un
particolare momento.

La distinzione può essere pensata così:

Gioco
→ definisce le regole

Stato del gioco
→ registra la situazione corrente rispetto a quelle regole

Uno stato dovrebbe contenere informazioni sufficienti per rispondere alle
domande sulla posizione corrente senza dipendere da variabili esterne
invisibili.

## Spiegazione tecnica

Uno stato completo contiene ogni informazione di dominio necessaria per
descrivere la situazione corrente rilevante per il gameplay.

A seconda del gioco, può includere:

- posizioni dei pezzi;
- risorse possedute dai giocatori;
- giocatore di turno;
- contatori o informazioni sulla fase;
- effetti già stabiliti che continuano a influenzare il gioco legale.

La rappresentazione esatta varia da gioco a gioco.

Il requisito architetturale non è che ogni gioco utilizzi gli stessi campi. Il
requisito è che uno stato sia completo rispetto alle regole del proprio gioco.

Uno stato dovrebbe anche essere valido.

Per esempio, se le regole stabiliscono che esattamente un giocatore abbia il
turno, uno stato che dichiara entrambi i giocatori contemporaneamente di turno
violerebbe il modello di dominio.

## Esempio originale

Consideriamo un gioco inventato chiamato **Three Stones**.

Due giocatori, North e South, inseriscono a turno una pietra in uno dei tre
spazi vuoti.

Un possibile stato potrebbe essere descritto così:

- spazio 1: North;
- spazio 2: vuoto;
- spazio 3: South;
- prossimo giocatore: North.

Questa fotografia è diversa dalle regole del gioco.

Le regole spiegano cosa i giocatori possono fare.

Lo stato registra ciò che è vero in questo momento.

Se `next player` fosse conservato in una variabile globale estranea allo
stato, la fotografia non sarebbe più autosufficiente.

Questa dipendenza nascosta renderebbe più fragili ragionamento, test, replay e
ricerca.

## Indipendenza degli stati

Gli algoritmi di ricerca devono esplorare futuri ipotetici.

Se applicare una mossa ipotetica modifica silenziosamente lo stato corrente
originale, diversi rami della ricerca possono interferire tra loro.

BoardLab richiede quindi una rappresentazione dello stato capace di produrre
uno stato successore indipendente per la ricerca.

Questo non prescrive ancora una particolare implementazione Python.

La proprietà importante è concettuale:

esaminare un futuro ipotetico non deve corromperne un altro.

## Errori comuni di modellazione

### Stato incompleto

Alcune informazioni rilevanti per il gameplay esistono soltanto in una
variabile esterna.

Conseguenza:

lo stesso stato visibile può comportarsi diversamente in base a un contesto
nascosto.

### Mescolare le regole nella fotografia

Uno stato contiene logica procedurale che descrive come funziona il gioco invece
di rappresentare la situazione corrente.

Conseguenza:

rappresentazione dello stato e regole diventano inutilmente accoppiate.

### Mutazione condivisa accidentale

Due stati ipotetici che dovrebbero essere indipendenti condividono dati
mutabili.

Conseguenza:

modificare un ramo può alterarne un altro.

### Conservare dati di presentazione irrilevanti come verità del dominio

Layout della UI o informazioni destinate soltanto alla visualizzazione vengono
trattati come parte dello stato del gioco.

Conseguenza:

il modello di dominio diventa dipendente da una particolare interfaccia.

## Scenario problematico

Un algoritmo di ricerca esplora due mosse candidate partendo dallo stesso stato
corrente.

Genera lo stato A per la prima candidata e lo stato B per la seconda.

Modificare un pezzo nello stato A modifica anche il pezzo corrispondente nello
stato B.

I due rami della ricerca non sono quindi indipendenti.

Il problema non è principalmente un errore dell'algoritmo di ricerca.

È un errore di modellazione dello stato.

## Takeaway

Un `GameState` dovrebbe essere una rappresentazione completa, valida e
utilizzabile indipendentemente di una situazione di gioco.

Se informazioni importanti per il gameplay vivono fuori dallo stato, oppure
stati ipotetici si influenzano inaspettatamente, i futuri algoritmi di ricerca
non possono ragionare in modo affidabile.

## Esercizio per il learner

Immagina un gioco originale per due giocatori nel quale i partecipanti
rivendicano alternativamente celle di un piccolo tabellone e ciascun giocatore
dispone di un numero limitato di gettoni.

Elenca le informazioni minime che, secondo te, uno stato completo del gioco
deve contenere.

Per ciascun elemento spiega brevemente perché lasciarlo fuori dallo stato
potrebbe creare ambiguità.

Non progettare ancora classi Python.
