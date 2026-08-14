# Unità 04 — Stati terminali e risultato

[English](unit-04-terminal-states-and-outcome.md) | [Italiano](unit-04-terminal-states-and-outcome.it.md)

## Learning objectives

Dopo aver studiato questa unità, il learner dovrebbe essere in grado di:

- distinguere uno stato in corso da uno stato terminale;
- spiegare perché le condizioni terminali appartengono alle regole del gioco;
- distinguere la terminazione del gioco dalla valutazione strategica;
- ragionare su un risultato dal punto di vista di un giocatore;
- riconoscere una logica di terminazione incoerente come difetto del modello di
  dominio.

## Riferimenti alle fonti

Riferimento BoardLab primario:

- `docs/architecture/overview.md`

Dichiarazioni architetturali rilevanti:

- il contratto concettuale del gioco deve permettere di rilevare gli stati
  terminali;
- uno stato deve poter essere valutato dal punto di vista di un giocatore;
- `GameState` rappresenta una configurazione di gioco completa e valida;
- le strategie generiche devono dipendere dal contratto pubblico del gioco e
  non da regole di giochi concreti.

Riferimento di supporto:

- `README.md`

BoardLab mantiene separate le regole del gioco dagli algoritmi di strategia
generici.

Le spiegazioni e gli esempi seguenti sono materiale didattico originale
BoardLab.

## Modello mentale

Non ogni stato consente un'altra mossa.

Alcuni stati significano che il gioco è terminato.

Questi sono stati terminali.

Una distinzione utile è:

Terminalità
→ chiede se il gioco è terminato

Risultato
→ descrive cosa significa lo stato conclusivo per i partecipanti

Valutazione strategica
→ può stimare quanto sia desiderabile uno stato, compresi stati non terminali

Questi concetti sono collegati ma non dovrebbero essere fusi in un'unica idea.

## Spiegazione tecnica

Una condizione terminale fa parte delle regole del gioco.

Esempi di schemi terminali generici includono:

- è stata raggiunta una condizione di vittoria;
- è stata raggiunta una condizione di sconfitta;
- è stata raggiunta una condizione di pareggio;
- non è consentito proseguire il gioco;
- si è verificata una condizione di arresto specifica del gioco.

Il motore generico dovrebbe poter chiedere se lo stato corrente sia terminale
senza conoscere la regola concreta che lo rende tale.

Allo stesso modo, una strategia generica non dovrebbe codificare direttamente
quelle concrete regole di arresto.

Il dominio del gioco è autorevole.

## Esempio originale

Continuiamo con il gioco inventato **Three Stones**.

Supponiamo che la sua regola didattica originale sia:

Il gioco termina quando tutti e tre gli spazi sono occupati.

Consideriamo questo stato:

- spazio 1: North;
- spazio 2: North;
- spazio 3: South;
- next player: South.

Tutti gli spazi sono occupati.

Secondo questa regola inventata, lo stato è terminale.

Non è legale alcuna ulteriore mossa di piazzamento.

Supponiamo ora che la regola del risultato dica:

Vince il giocatore che possiede più spazi.

Dal punto di vista di North, il risultato è una vittoria.

Dal punto di vista di South, lo stesso stato terminale è una sconfitta.

Lo stato è identico.

L'interpretazione dipende dalla prospettiva del giocatore.

## Terminalità e valutazione non sono la stessa cosa

Un futuro algoritmo di ricerca potrebbe dover assegnare valori utili agli
stati.

Questo non significa che ogni stato valutato sia terminale.

Per esempio, uno stato ancora in corso potrebbe essere considerato
strategicamente favorevole a North anche se il gioco non è terminato.

Quindi:

rilevamento dello stato terminale
e
valutazione dello stato

non devono essere trattati come sinonimi.

La terminalità risponde a una domanda sulle regole.

La valutazione risponde a una domanda di supporto alla decisione.

Questa distinzione diventa particolarmente importante prima di introdurre
algoritmi come Minimax.

## Prospettiva del giocatore

Un risultato richiede spesso la prospettiva di un giocatore.

Lo stesso stato terminale può significare:

- vittoria per un giocatore;
- sconfitta per un altro;
- pareggio per entrambi.

L'architettura BoardLab stabilisce già che la valutazione debba essere possibile
dal punto di vista di un giocatore.

Questo evita di nascondere nel modello di dominio una prospettiva universale
arbitraria.

## Errori comuni di modellazione

### Codificare le regole terminali dentro una strategia

Una strategia controlla direttamente se un gioco concreto è stato vinto.

Conseguenza:

la strategia diventa dipendente da quel gioco.

### Considerare universalmente l'assenza di mosse legali equivalente alla fine

Alcuni giochi possono definire quella condizione come terminale, mentre altri
possono prevedere un passaggio del turno o un'altra regola.

Conseguenza:

un'assunzione generica sostituisce la semantica specifica del gioco.

### Trattare la valutazione come rilevamento della terminalità

Uno stato riceve un valore alto o basso e quindi il programma presume che il
gioco sia terminato.

Conseguenza:

la desiderabilità strategica viene confusa con il completamento definito dalle
regole.

### Omettere la prospettiva del giocatore

Un risultato viene conservato come se la stessa etichetta avesse significato
identico per ogni partecipante.

Conseguenza:

la semantica di vittoria e sconfitta diventa ambigua.

## Scenario problematico

Una strategia generica contiene questa regola:

"Se non esistono mosse legali, il gioco è terminato."

Un futuro gioco aggiunto a BoardLab permette invece a un giocatore senza mosse
legali di passare il turno.

La strategia generica termina ora le partite in modo errato.

Il difetto esiste perché un'assunzione terminale specifica di un gioco è
penetrata in un algoritmo generico.

## Takeaway

La terminalità è una proprietà di un `GameState` definita dalle regole.

Il risultato spiega il significato di una partita conclusa, spesso dalla
prospettiva di un particolare `Player`.

La valutazione strategica è collegata ma distinta e può applicarsi anche a
stati non terminali.

Mantenere separati questi concetti prepara BoardLab ai successivi algoritmi di
ricerca generici.

## Esercizio per il learner

Usando il tuo gioco originale:

1. definisci almeno una condizione terminale;
2. fornisci un esempio di stato in corso;
3. fornisci un esempio di stato terminale;
4. descrivi il risultato terminale dal punto di vista di ciascun giocatore;
5. descrivi uno stato non terminale che potrebbe comunque apparire
   strategicamente migliore per un giocatore rispetto all'altro.

Non assegnare ancora punteggi numerici di valutazione.
