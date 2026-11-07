# Dado Goal! #69 — Catalogo degli eventi straordinari

[English](issue-69-extraordinary-events-catalogue.md) | [Italiano](issue-69-extraordinary-events-catalogue.it.md)

Stato: candidato di design — prima single-player

## Invarianti

- Gli eventi straordinari vengono valutati solo dopo una mano completata.
- Per ogni mano completata può essere selezionato al massimo un evento ausiliario.
- Gli eventi straordinari non modificano il risultato dei dadi o il punteggio, salvo che un contratto meccanico esplicitamente approvato stabilisca diversamente.
- Gli eventi narrativi non modificano mai lo stato della partita.
- Gli eventi meccanici devono avere test deterministici per prerequisiti, transizione di stato, limiti e casi di errore.
- Il comportamento esistente di ammonizioni, espulsioni e sostituzioni resta autoritativo finché non viene esplicitamente sostituito.
- Restano autoritative le invarianti esistenti sulle cinque sostituzioni e sui giocatori attivi.
- Ogni evento implementato richiede una presentazione equivalente IT/EN.
- La parità multiplayer è rinviata finché catalogo e meccaniche single-player non sono qualificati.

## Classificazione

`NARRATIVE`
: Solo evento cronologico della partita e telecronaca. Nessuna modifica a rosa, punteggio, dadi, probabilità o flusso della partita.

`MECHANICAL`
: Può modificare esclusivamente lo stato della partita esplicitamente indicato. Non deve mai alterare implicitamente il punteggio o la risoluzione dei dadi.

## Matrice dei candidati

| Evento | Classe | Prerequisiti | Probabilità candidata | Effetto | Test richiesti |
|---|---|---|---:|---|---|
| Leggera contusione | NARRATIVE | giocatore attivo | 0,6% | sola telecronaca; il giocatore continua | selezione deterministica; nessuna modifica di stato |
| Infortunio serio | MECHANICAL | giocatore attivo; sostituto legale disponibile | probabilità `injury` esistente: 1% | sostituzione forzata esistente con `reason: "injury"`; nessuna ulteriore estrazione di evento straordinario | il giocatore attivo esce; entra il sostituto; limite di cinque sostituzioni |
| Infortunio senza sostituto | EXISTING FAILURE CASE | nessun sostituto legale o limite sostituzioni raggiunto | ramo `injury` esistente | nessun evento emesso e nessuna modifica della rosa; nessun sostituto o esito narrativo inventato | rosa invariata; nessun evento incompleto |
| Pioggia intensa | NARRATIVE | nessuno | 0,25% | telecronaca atmosferica | nessuna modifica di stato; limite per partita |
| Vento forte | NARRATIVE | nessuno | 0,15% | telecronaca atmosferica | nessuna modifica di stato; limite per partita |
| Neve / scarsa visibilità | NARRATIVE | nessuno | 0,05% | rara telecronaca atmosferica | soglia di rarità; nessuna modifica di stato |
| Invasione di campo | NARRATIVE | nessuno | 0,03% | interruzione narrativa temporanea | nessuna mano aggiuntiva; nessuna modifica a cronometro/stato |
| Oggetto in campo | NARRATIVE | nessuno | 0,04% | interruzione narrativa | nessuna modifica di stato |
| Animale in campo | NARRATIVE | nessuno | 0,01% | interruzione rara, ironica ma plausibile | rarità; nessuna modifica di stato |
| Guasto ai riflettori | NARRATIVE | nessuno | 0,02% | sospensione/ripresa narrativa | nessuna modifica di stato |
| Rete della porta danneggiata | NARRATIVE | nessuno | 0,02% | ritardo narrativo per riparazione | nessuna modifica di stato |
| Controllo VAR | NARRATIVE | azione calcistica precedente idonea | 0,10% | sola revisione in telecronaca; il risultato originale resta valido | punteggio/dadi invariati |
| Problema all'equipaggiamento arbitrale | NARRATIVE | nessuno | 0,03% | interruzione narrativa | nessuna modifica di stato |

## Politica delle probabilità

Le percentuali sopra indicate sono candidate di design, non ancora costanti di gioco.

Formano un canale separato per gli eventi straordinari. Non devono essere inserite nelle soglie cumulative esistenti per ammonizioni, espulsioni, infortuni e sostituzioni finché l'architettura di selezione non viene approvata.

L'implementazione iniziale dovrebbe privilegiare gli eventi narrativi, perché aggiungono varietà senza modificare l'equilibrio competitivo.

L'infortunio serio non introduce una nuova probabilità di evento straordinario. La issue #69 adotta il ramo autoritativo `injury` esistente (`1%`) e le relative meccaniche di sostituzione forzata. Aggiungere una seconda estrazione dello `0.4%` per l'infortunio serio duplicherebbe lo stesso evento calcistico e modificherebbe le probabilità di gioco consolidate.

Qualsiasi espansione meccanica oltre questa meccanica di infortunio esistente è esplicitamente rinviata.

## Limiti per partita

- Famiglia meteo: massimo 1 evento meteorologico straordinario per partita.
- Famiglia interferenze esterne: massimo 1 per partita.
- Famiglia tecnica/arbitrale: massimo 1 per partita.
- Animale in campo: massimo 1 per partita.
- L'infortunio serio resta l'evento `injury` esistente ed è vincolato dall'idoneità della rosa e dal limite esistente di cinque sostituzioni.
- Se il ramo injury esistente non può effettuare una sostituzione legale, non emette alcun evento e non inventa un sostituto narrativo.
- Il fallimento di un prerequisito meccanico non deve generare una transizione di stato alternativa inventata.

## Politica del minuto

- Le perdite di tempo restano disciplinate dal modello disciplinare esistente.
- Il VAR richiede una precedente azione calcistica idonea e non costituisce un'interruzione casuale libera.
- Gli eventi meteorologici possono verificarsi durante tutta la partita.
- Le interferenze esterne e gli incidenti tecnici possono verificarsi durante tutta la partita, ma restano individualmente rari.
- Nessun evento straordinario può far avanzare autonomamente il cronometro della partita.

## Ordine di implementazione single-player

1. Introdurre un catalogo degli eventi straordinari come dati.
2. Introdurre idoneità e selezione deterministiche senza modifiche alla UI.
3. Implementare gli eventi esclusivamente narrativi.
4. Aggiungere descrizioni IT/EN e rendering cronologico.
5. Qualificare l'infortunio serio come ramo autoritativo `injury` esistente; non aggiungere una seconda probabilità o transizione di stato.
6. Eseguire test deterministici e statistici.
7. Eseguire il road test single-player.
8. Solo successivamente derivare l'implementazione autoritativa multiplayer.

## Esplicitamente rinviato

- Modifiche al punteggio causate dal VAR.
- Nuovi lanci o modifica dei dadi a causa del meteo.
- Abbandono o ripetizione delle partite.
- Modificatori meteorologici persistenti.
- Inferiorità numerica temporanea che richieda un nuovo modello temporizzato dello stato dei giocatori.
- Qualsiasi modifica alla persistenza multiplayer o D1.
