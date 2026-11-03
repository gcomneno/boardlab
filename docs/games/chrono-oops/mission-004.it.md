# ChronoOops! — Missione 004

[English](mission-004.md) | [Italiano](mission-004.it.md)

## Due eventi, una sola causa

- Progetto: GiadaWare BoardLab
- Documento: Game Design Specification M004
- Versione: 0.1
- Stato: proposta progettuale, non implementata
- Base: Missioni 001–003, motori deterministici C11

## 1. Premessa narrativa

Nel 1985 il Cronofono produce un impulso di sincronizzazione
che attraversa un circuito di distribuzione.

Nel 2025 due sistemi dipendono da quell'impulso storico:

- il Segnale Faro, necessario per individuare una stazione;
- il Registro Centrale, necessario per verificare la
  sincronizzazione degli eventi.

Entrambi devono risultare attivi.

Un vecchio commutatore permette apparentemente soltanto
due scelte: FARO oppure REGISTRO.

Il giocatore deve scoprire che il circuito possiede anche
un contatto doppio, inizialmente non documentato.

La difficoltà non consiste nel produrre due impulsi:
consiste nel distribuire correttamente lo stesso impulso.

## 2. Obiettivo

A (1985) deve individuare e configurare il distributore
storico dell'impulso.

B (2025) deve esaminare entrambi gli effetti, verificare
che derivino dalla stessa causa e confermare la stabilità
della configurazione.

Vittoria: entrambi i sistemi funzionanti, origine comune
verificata e missione completata entro otto turni.

## 3. Regole conservate

- Una sola cronologia modificabile.
- Otto turni massimi, alternanza A/B.
- A opera nel 1985; B opera nel 2025.
- Due punti azione per turno.
- Ogni azione riuscita consuma un punto azione.
- Le azioni rifiutate non modificano stato o PA.
- Propagazione degli effetti storici a fine turno di A.
- Determinismo e risultati riproducibili.
- Nessuna modifica ai motori M001, M002 e M003.

## 4. Modello causale

Esiste un solo impulso storico, identificato come IMPULSO-85.

L'impulso è un segnale di attivazione, non una riserva
di energia da dividere.

Il distributore può assumere tre configurazioni:

- FARO: attiva soltanto il Segnale Faro;
- REGISTRO: attiva soltanto il Registro Centrale;
- DOPPIO: distribuisce il segnale a entrambi.

La configurazione DOPPIO richiede che A abbia scoperto
e verificato il contatto aggiuntivo.

Il distributore non produce nuove cause e non duplica
l'evento storico: collega due effetti allo stesso impulso.

## 5. Scelte e conseguenze

### Percorso FARO

A configura il circuito esclusivamente per il Faro.

Dopo la propagazione, B osserva:
- Segnale Faro attivo;
- Registro Centrale inattivo.

La missione non è completabile in questa configurazione.

### Percorso REGISTRO

A configura il circuito esclusivamente per il Registro.

Dopo la propagazione, B osserva:
- Segnale Faro inattivo;
- Registro Centrale attivo.

Anche questa configurazione è insufficiente.

### Percorso DOPPIO

A individua il contatto doppio e lo abilita.

Dopo la propagazione, B osserva:
- Segnale Faro attivo;
- Registro Centrale attivo.

B deve ancora verificare che entrambi dipendano
dallo stesso IMPULSO-85.

Questa verifica distingue la riconciliazione causale
dalla semplice presenza di due indicatori accesi.

## 6. Reversibilità

Le configurazioni FARO e REGISTRO non provocano
automaticamente una sconfitta.

A può correggere una scelta esclusiva in un proprio
turno successivo, se dispone di PA e tempo sufficiente.

Dopo ogni modifica, gli effetti del 2025 si aggiornano
soltanto alla fine del turno storico.

Le verifiche effettuate su una configurazione precedente
non valgono automaticamente per quella nuova.

Una configurazione certificata e stabilizzata non può
essere alterata dopo il completamento della missione.

Non servono universi paralleli né un sistema di rollback
generale.

## 7. Sequenza nominale

| Turno | Agente | Azione 1 | Azione 2 |
|---|---|---|---|
| 1 | A | Ispeziona distributore | Identifica impulso |
| 2 | B | Esamina Faro | Esamina Registro |
| 3 | A | Scopre contatto doppio | Attiva distribuzione doppia |
| 4 | B | Verifica Faro | Verifica Registro |
| 5 | A | Certifica configurazione | Registra origine comune |
| 6 | B | Convalida coerenza | Completa missione |

L'attivazione DOPPIO viene propagata alla fine
del turno 3.

La vittoria nominale avviene al turno 6.

## 8. Percorso alternativo

Il giocatore può selezionare FARO oppure REGISTRO
prima di scoprire il contatto doppio.

L'effetto esclusivo diventa visibile nel 2025
dopo la propagazione.

L'osservazione del sistema mancante fornisce un
indizio utile per cercare un'altra configurazione.

Se A scopre successivamente il contatto doppio,
può correggere il distributore.

Il successo dipende dai turni e dai PA rimasti.

## 9. Rifiuti e invarianti

- Non si può attivare DOPPIO senza averne scoperto
  il contatto.
- Non si possono verificare effetti futuri non attivi.
- Non si può convalidare la coerenza senza entrambe
  le verifiche e senza l'origine comune certificata.
- Una verifica diventa obsoleta se cambia il circuito.
- La configurazione non produce effetti nel 2025
  prima della propagazione di fine turno.
- I rifiuti sono atomici e non consumano PA.
- Una configurazione esclusiva non costituisce
  un paradosso: è una soluzione incompleta.

## 10. Vittoria e sconfitta

Vittoria quando:
- entrambi gli effetti sono attivi;
- entrambi sono stati verificati dopo l'ultima modifica;
- la configurazione storica è certificata;
- IMPULSO-85 è registrato come origine comune;
- la coerenza è convalidata;
- la missione è completata entro il turno 8.

Sconfitta quando:
- termina il turno 8 senza soddisfare gli obiettivi;
- uno stato irreversibile rende impossibile
  completare la missione.

Nessuna sconfitta anticipata basata soltanto su
una configurazione esclusiva ancora correggibile.

## 11. Contratto di verifica

- Alternanza A/B, 2 PA, massimo 8 turni.
- Tre configurazioni distinte: FARO, REGISTRO, DOPPIO.
- Un'unica causa storica.
- Propagazione esclusivamente a fine turno di A.
- FARO non attiva il Registro.
- REGISTRO non attiva il Faro.
- DOPPIO attiva entrambi gli effetti.
- Le modifiche invalidano le verifiche obsolete.
- Le azioni rifiutate sono atomiche.
- Il percorso nominale vince al turno 6.
- Il percorso alternativo è correggibile entro i limiti.
- Determinismo su sequenze identiche.
- Nessuna modifica ai motori precedenti.

## 12. Confini di implementazione

La Missione 004 avrà un modulo C11 autonomo,
test propri e una CLI dedicata.

La CLI unificata sarà estesa dopo la qualificazione
del motore.

Esclusi:
- cronologie parallele;
- propagazione retroattiva generalizzata;
- simulazioni elettriche realistiche;
- framework esterni;
- nuove dipendenze;
- grafica 2D/3D;
- multiplayer.

## 13. Qualificazione

- DESIGN_GATE: PROPOSED
- ENGINE_VALIDATION: NOT_RUN
- IMPLEMENTATION: NOT_STARTED
- CLI_INTEGRATION: NOT_STARTED
- PUBLICATION: DEFERRED
