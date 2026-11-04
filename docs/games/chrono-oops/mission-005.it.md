# ChronoOops! — Missione 005

[Italiano](mission-005.it.md) | [English](mission-005.md)

## Il minuto che non è mai esistito

## 1. Premessa narrativa

Nel 2025, l'Archivio delle Anomalie raccoglie le testimonianze
dei quattro incidenti temporali precedenti.

Due eventi storici, chiamati SEGNALE e RISPOSTA, risultano
registrati nell'ordine sbagliato: la RISPOSTA sembra precedere
il SEGNALE che l'ha provocata.

Nel 1985 l'orologio di riferimento aveva perso sessanta secondi.
Gli eventi reali non hanno cambiato ordine: le loro etichette
temporali sono diventate inattendibili.

La missione conclusiva non richiede di inventare una nuova
cronologia. Richiede di dimostrare quella realmente avvenuta.

## 2. Obiettivo

A opera nel 1985 e B nel 2025.

A deve identificare il difetto dell'orologio, preservare una
testimonianza indipendente dell'ordine degli eventi e certificare
la provenienza della fonte originale.

B deve riconoscere l'anomalia negli archivi, ricostruire
l'ordine reale con prove indipendenti e validare la cronologia.

Vittoria quando:

- l'ordine reale SEGNALE → RISPOSTA è dimostrato;
- la testimonianza indipendente è disponibile e verificata;
- la fonte storica è certificata;
- la ricostruzione è validata senza alterare gli eventi;
- la campagna è completata entro il turno 8.

## 3. Regole conservate

- Una sola timeline modificabile.
- A opera nel 1985, B nel 2025.
- Alternanza A/B, iniziando con A.
- Due punti azione (PA) per turno.
- Massimo otto turni.
- Le conseguenze storiche si propagano alla fine del turno di A.
- Le azioni rifiutate non consumano PA e non modificano lo stato.
- Nessun nuovo motore temporale o salto all'indietro generalizzato.

## 4. Modello causale

La sequenza reale è immutata:

1. Il SEGNALE viene emesso.
2. La RISPOSTA viene prodotta in conseguenza del SEGNALE.

L'orologio difettoso introduce una discrepanza di sessanta
secondi nelle registrazioni. Il registro può quindi rappresentare
la RISPOSTA come precedente al SEGNALE.

Il difetto riguarda la misura del tempo, non la causalità degli
eventi.

La testimonianza indipendente conserva l'ordine osservato
senza dipendere dall'orologio difettoso.

Non è necessario modificare il SEGNALE o la RISPOSTA.

## 5. Due strategie

### A. Rettifica superficiale

Il giocatore può tentare di correggere direttamente gli orari
nell'archivio del 2025.

La rettifica può rendere apparentemente ordinati i timestamp,
ma non costituisce una prova dell'ordine reale.

Non produce un paradosso; produce una ricostruzione
insufficiente, che può essere riesaminata.

### B. Ricostruzione documentata

A preserva una testimonianza indipendente dell'ordine
SEGNALE → RISPOSTA.

La testimonianza diventa consultabile da B solo dopo la
propagazione di fine turno storico.

B confronta la testimonianza con il registro originale,
ricostruisce l'ordine reale e verifica che i due eventi
rimangano distinti e correttamente collegati.

A certifica la provenienza della testimonianza storica.
B valida la ricostruzione finale.

## 6. Revisione e reversibilità

Una rettifica superficiale non blocca la missione.

B può riconoscerne l'insufficienza e tornare a usare
il registro originale e la testimonianza indipendente.

Una nuova testimonianza storica, propagata da A, rende
obsolete le verifiche dipendenti dalla precedente
ricostruzione.

La revisione non cancella eventi e non crea nuove timeline.

Una prova non ancora propagata non può essere verificata
anticipatamente da B.

## 7. Sequenza nominale

| Turno | Agente | Azione 1 | Azione 2 |
|---|---|---|---|
| 1 | A | Ispeziona orologio | Identifica perdita di 60 secondi |
| 2 | B | Consulta archivio | Rileva inversione apparente |
| 3 | A | Osserva ordine reale | Preserva testimonianza indipendente |
| 4 | B | Confronta testimonianza | Ricostruisce ordine reale |
| 5 | A | Certifica fonte originale | Registra provenienza storica |
| 6 | B | Valida cronologia | Completa campagna |

La testimonianza è propagata alla fine del turno 3.

La vittoria nominale avviene al turno 6.

## 8. Percorso alternativo

Al turno 2, B può scegliere una rettifica superficiale
dell'archivio invece di rilevare correttamente l'anomalia.

La correzione apparente non è sufficiente per vincere.

In un turno successivo B può riconoscere l'insufficienza,
ripristinare l'uso delle registrazioni originali e completare
la ricostruzione documentata.

Il percorso alternativo deve poter raggiungere la vittoria
entro il turno 8 quando gli interventi sono compatibili.

Non è garantita la vittoria se il giocatore esaurisce i turni.

## 9. Informazione e prove

Il motore distingue esplicitamente:

- ordine reale degli eventi;
- ordine risultante dai timestamp difettosi;
- presenza della testimonianza indipendente;
- propagazione della testimonianza;
- verifica della testimonianza;
- stato della rettifica superficiale;
- provenienza storica certificata;
- ricostruzione finale validata.

Il semplice riordino dei timestamp non modifica l'ordine
reale e non soddisfa i requisiti di vittoria.

Le verifiche devono riferirsi a prove effettivamente disponibili.

## 10. Vittoria, sconfitta e rifiuti

La vittoria richiede congiuntamente:

- testimonianza indipendente propagata;
- ordine SEGNALE → RISPOSTA verificato;
- provenienza storica certificata;
- cronologia finale validata;
- completamento entro il turno 8.

Sconfitta al termine del turno 8 se manca uno di questi
requisiti.

Sono rifiutate atomicamente:

- azioni del personaggio sbagliato;
- azioni senza PA;
- verifiche di testimonianze non propagate;
- certificazioni senza fonte indipendente;
- validazioni basate soltanto su timestamp ritoccati;
- completamento senza validazione e provenienza.

Una rettifica superficiale è una scelta inefficace,
non un paradosso causale.

## 11. Finale narrativo

Quando B completa la validazione, l'Archivio delle Anomalie
conserva finalmente una cronologia coerente delle vicende
attraversate da A e B.

I quattro casi precedenti non vengono riscritti né modificati.

L'apparente contraddizione finale scompare perché è stata
riconosciuta la differenza tra ciò che è accaduto e ciò che
l'orologio aveva registrato.

Ultima battuta:

> Il tempo non aveva sbagliato nulla.
> Erano gli orologi a raccontare frottole.

## 12. Contratto di verifica

L'implementazione C11 deve verificare:

- determinismo e limite di otto turni;
- massimo due PA per turno;
- ordine reale immutato;
- propagazione storica esclusivamente a fine turno di A;
- impossibilità di verificare prove non propagate;
- nessuna vittoria con sola rettifica superficiale;
- rifiuti atomici e stato invariato;
- invalidazione delle verifiche obsolete;
- vittoria nominale al turno 6;
- recupero del percorso alternativo entro il turno 8;
- nessuna regressione delle Missioni 001–004;
- completamento tramite CLI unificata.

## 13. Confini di implementazione

- Motore autonomo in C11.
- API e test dedicati alla Missione 005.
- Terminale testuale coerente con le missioni precedenti.
- Nessuna modifica ai motori M001–M004.
- Nessun framework, database o dipendenza esterna.
- Nessuna interfaccia grafica introdotta in questa fase.
- La conclusione della quinta missione non autorizza
  automaticamente una release pubblica.
- Dopo la campagna sono richiesti collaudo complessivo
  e preparazione della distribuzione multipiattaforma.

## 14. Qualificazione

Il presente documento stabilisce il contratto candidato
per la conclusione della campagna.

La qualificazione richiede parità semantica italiano/inglese,
verifica delle regole causali e successiva implementazione
C11 separata.

Nessuna implementazione è inclusa in questa fase.
