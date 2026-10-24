# Tre Sigilli — Product Brief SHIP-01

Languages: [English](tre-sigilli-ship-product-brief.md) | [Italiano](tre-sigilli-ship-product-brief.it.md)

Versione: 0.1 — direzione prodotto approvata; ipotesi da validare
Ambito: product discovery e validazione pre-pubblicazione

## Missione

Valutare Tre Sigilli come gioco strategico mobile originale,
comprensibile, gradevole e rigiocabile.

La pubblicazione commerciale non è automatica.

## Evidenze esistenti

- Regole originali v0.2: griglia 4×4, tre sigilli per giocatore.
- Piazzamento, manovra e controllo territoriale.
- Motore Python e dominio Kotlin con parità verificata.
- Prototipo Android Compose giocabile.
- Avversario Android Alpha-Beta a profondità 2.
- Prime partite interne effettuate.

Le prime impressioni positive non dimostrano domanda di mercato,
bilanciamento o rigiocabilità esterna.

## Pubblico — ipotesi da validare

Giocatori mobile interessati a giochi strategici astratti,
partite brevi, apprendimento progressivo e gioco offline.

Non sono ancora disponibili evidenze sul pubblico effettivo.

## Promessa di prodotto — candidata

"Poche mosse. Molte strategie."

Promessa da verificare attraverso test con giocatori esterni.

## Game design

- Conservare la modalità classica 4×4.
- Preservare le regole canoniche fino a modifica esplicita.
- Valutare difficoltà AI basate su strategie effettivamente
  implementate e misurate.
- Studiare 5×5 e 6×6 come varianti separate: sigilli, durata,
  territorio, bilanciamento e tempi AI richiedono nuova validazione.

## Gaming UX

- Guida interattiva alle regole.
- Evidenza visiva dell'influenza sulla casella selezionata.
- Spiegazione dei punti e del risultato finale.
- Feedback chiaro per selezione, mossa valida e mossa illegale.
- Accessibilità: contrasti, dimensioni, touch target,
  contenuti comprensibili senza affidarsi soltanto al colore.

## Identità visiva

- Sostituire N/S come rappresentazione definitiva dei sigilli.
- Progettare simboli originali, distinguibili anche senza colore.
- Definire palette, tipografia, gerarchia e microanimazioni.
- Verificare leggibilità su schermi Android piccoli.
- Non adottare asset di terzi senza verifica dei diritti.

La direzione artistica resta da approvare.

## Product marketing

- Qualificare target e alternative di mercato.
- Validare nome, promessa e presentazione del gioco.
- Preparare test esterni prima di pianificare acquisizione utenti.
- Nessuna previsione di download o ricavi senza evidenze.
- Monetizzazione e pubblicazione richiedono decisioni separate.

## Modello Free/Premium — proposta

### Free: esperienza completa

- Modalità classica 4×4 con regole integrali.
- Partite locali a due giocatori.
- Avversario Android accessibile e difficoltà base.
- Tutorial, spiegazione dell'influenza e del risultato.
- Sigilli originali e tema grafico standard.
- Gioco offline, senza pubblicità invasive.

### Premium: personalizzazione ed espansione

- Collezioni di sigilli e temi grafici alternativi.
- Varianti ufficiali 5×5 e 6×6, solo dopo validazione.
- Modalità Laboratorio con griglia configurabile, soggetta
  a fattibilità tecnica e bilanciamento.
- Livelli AI avanzati, da calibrare con benchmark riproducibili.
- Modalità Laboratorio AI con MCTS, subordinata a implementazione
  Android, verifica prestazionale e validazione del gameplay.

### Livelli AI candidati

| Livello | Strategia candidata | Accesso |
| --- | --- | --- |
| Principiante | Random | Free |
| Normale | Alpha-Beta calibrato | Free |
| Esperto | Alpha-Beta con ricerca approfondita | Premium |
| Laboratorio AI | MCTS configurato e verificato | Premium |

MCTS non è automaticamente più difficile o più forte di
Alpha-Beta. I livelli richiedono misurazioni prima dell'approvazione.
Minimax resta una possibile alternativa sperimentale.

I cosmetici non alterano le regole né conferiscono vantaggi.

### Asset e licenze

- Preferire collezioni originali GiadaWare.
- Pacchetti di sticker esterni soltanto con diritti verificati
  per modifica, integrazione e distribuzione commerciale.
- Nessuna dipendenza da servizi esterni per i sigilli standard.

### Monetizzazione

- Ipotesi: una sola app gratuita con sblocco Premium permanente.
- Prezzo indicativo da testare: EUR 2,99–4,99.
- Nessun abbonamento previsto nella proposta iniziale.
- Pagamenti, store, fiscalità e gestione degli acquisti
  richiedono qualificazione e approvazione separate.
- Nessun acquisto, account commerciale o pubblicazione autorizzati.

La prima release non deve necessariamente includere Premium.
Le funzionalità non sviluppate non devono essere promesse come disponibili.

## Playtesting

Primo ciclo proposto: 5–10 giocatori esterni.

Osservare:
- Comprensione delle regole senza assistenza.
- Capacità di spiegare il punteggio finale.
- Completamento della prima partita.
- Desiderio spontaneo di rigiocare.
- Percezione della difficoltà AI.
- Problemi di interazione e accessibilità.

Registrare osservazioni e feedback senza inventare risultati.

## Gate SHIP

SHIP-01 — Product Direction Gate: approvazione esplicita di pubblico ipotizzato, promessa, priorità UX, game design e piano di validazione.

SHIP Release Gate — decisione di pubblicazione soltanto con:
- Prototipo comprensibile e giocabile.
- Problemi UX prioritari identificati e verificati.
- Difficoltà AI valutate attraverso dati riproducibili.
- Feedback esterno documentato.
- Piano tecnico per release, firma, test e distribuzione.
- Verifica dei diritti sugli asset e dei costi.

La pubblicazione sullo store richiede approvazione esplicita.

## Fuori ambito per SHIP-01

- Modifiche al motore e alle regole.
- Implementazione di varianti.
- Restyling e generazione definitiva degli asset.
- Monetizzazione, store listing e pubblicazione.
- Commit o push automatici.
