# ChronoOops! — Missione 003

[English](mission-003.md) | [Italiano](mission-003.it.md)

## Il messaggio che non doveva arrivare

- Progetto: GiadaWare BoardLab
- Documento: Game Design Specification M003
- Versione: 0.1
- Stato: proposta progettuale, non implementata
- Base: Missioni 001–002, motori deterministici C11

## 1. Obiettivo

Nel 2025 B trova un registro che racconta un incidente
avvenuto nel 1985 durante una prova pubblica del Cronofono.

Il registro contiene un dettaglio decisivo: l'orario preciso
di una scarica elettrica e il nome della persona che la vide.

B può inviare l'informazione ad A nel 1985.

A deve mettere in sicurezza le persone coinvolte senza
cancellare la testimonianza che rende possibile il messaggio.

Il mondo conserva una sola cronologia modificabile.

## 2. Regole conservate

- Otto turni, alternando A (1985) e B (2025).
- Due punti azione per turno.
- Ogni azione riuscita costa un punto azione.
- Propagazione causale alla fine del turno.
- Determinismo, nessuna cronologia parallela.
- Azioni rifiutate atomicamente, senza consumo di PA.
- Nessuna modifica ai motori M001 e M002.

## 3. Informazione e provenienza

Il registro del 2025 esiste perché, nel 1985, un testimone
ha documentato l'incidente.

B può leggere il registro e trasmettere un solo avvertimento.

L'avvertimento contiene:
- l'orario della scarica;
- l'identità del testimone;
- il riferimento al registro da cui proviene.

La trasmissione crea un vincolo di provenienza: finché
l'informazione è utilizzata, deve rimanere possibile
la testimonianza che ne giustifica l'origine.

Ricevere un messaggio dal futuro non crea automaticamente
un paradosso.

## 4. Due interventi storici

A può scegliere tra:

### A. Cancellare la prova

Annulla completamente la prova pubblica.

L'intervento eviterebbe la scarica, ma impedirebbe anche
la testimonianza che ha prodotto il registro.

Se l'avvertimento è già stato trasmesso, questa scelta
è causalmente incompatibile e viene rifiutata.

### B. Mettere in sicurezza la prova

Allontana le persone dalla zona pericolosa e isola
il circuito interessato, mantenendo la prova osservabile.

La scarica viene registrata senza provocare feriti.
Il testimone conserva quindi un fatto autentico da
documentare, e il registro mantiene una provenienza valida.

La missione non richiede di provocare danni alle persone
per preservare l'informazione.

## 5. Vincolo causale

Dopo la trasmissione dell'avvertimento, un'azione che
cancellerebbe l'unica origine documentale necessaria
deve essere rifiutata con un risultato specifico.

Il rifiuto:
- identifica l'azione incompatibile;
- identifica il registro e la testimonianza vincolati;
- spiega quale origine informativa andrebbe perduta;
- non modifica stato, mondo o punti azione.

La messa in sicurezza, invece, è compatibile con la
conservazione della fonte.

Il motore non introduce un sistema generale di paradossi.

## 6. Sequenza nominale

| Turno | Agente | Azione 1 | Azione 2 |
|---|---|---|---|
| 1 | A | Ispeziona impianto | Individua rischio |
| 2 | B | Consulta registro | Trasmette avvertimento |
| 3 | A | Riceve avvertimento | Mette in sicurezza la prova |
| 4 | B | Verifica testimonianza | Controlla registro |
| 5 | A | Certifica la prova | Archivia testimonianza |
| 6 | B | Convalida provenienza | Completa missione |

L'intervento storico viene propagato alla fine del turno 3.

La vittoria nominale avviene al turno 6, quando:
- le persone sono al sicuro;
- la testimonianza rimane valida;
- l'informazione ha una provenienza verificabile;
- la missione viene completata.

## 7. Sequenza incompatibile

Dopo la trasmissione al turno 2, A tenta di cancellare
completamente la prova.

Il motore rifiuta l'intervento al turno 3 senza consumare PA.

A conserva la possibilità di scegliere l'intervento
compatibile e proseguire verso la vittoria.

Il giocatore deve ricevere una spiegazione causale,
non soltanto un messaggio generico di errore.

## 8. Vittoria, sconfitta e blocco

- Vittoria: persone al sicuro, fonte valida e provenienza
  convalidata entro il turno 8.
- Sconfitta: obiettivo incompleto alla fine del turno 8.
- Blocco anticipato: soltanto quando lo stato rende
  definitivamente impossibile raggiungere l'obiettivo.
- Nessuna vittoria se l'avvertimento manca o la sua
  provenienza non è più giustificabile.

## 9. Contratto di verifica

- Alternanza A/B, massimo due PA, limite di otto turni.
- Trasmissione dell'avvertimento consentita una sola volta.
- Messaggio associato a una fonte identificabile.
- Intervento incompatibile rifiutato atomicamente.
- Intervento compatibile propagato a fine turno.
- Provenienza conservata nel percorso vincente.
- Vittoria nominale al turno 6.
- Nessuna vittoria con fonte informativa invalida.
- Risultati riproducibili per sequenze identiche.
- Spiegazioni basate sulle transizioni reali del motore.

## 10. Confini di implementazione

La Missione 003 userà un modulo C11 specifico, con
stato e transizioni separati dalle missioni precedenti.

La CLI unificata verrà estesa soltanto dopo la
qualificazione del nuovo motore.

Esclusi: universi paralleli, IA, multiplayer di rete,
sistemi generali di inferenza, combattimento e grafica 2D/3D.

## 11. Qualificazione

- DESIGN_GATE: PROPOSED
- ENGINE_VALIDATION: NOT_RUN
- IMPLEMENTATION: NOT_STARTED
- CLI_INTEGRATION: NOT_STARTED
- PUBLICATION: DEFERRED
