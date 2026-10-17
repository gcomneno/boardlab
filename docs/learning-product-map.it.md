# Mappa di apprendimento e prodotto

[English](learning-product-map.md) | [Italiano](learning-product-map.it.md)

## Missione

BoardLab è un laboratorio open source per imparare a progettare,
implementare, testare e valutare giochi da tavolo originali e AI di gioco.

I giochi promettenti potranno diventare prodotti mobile. La
pubblicazione commerciale è facoltativa e non determina lo studio.

## LEARN — Comprendere

Acquisire conoscenze trasferibili su stati, mosse legali,
transizioni, terminalità, architettura, strategie, Minimax,
Alpha-Beta pruning, Monte Carlo Tree Search e sperimentazione.

Le evidenze comprendono esercizi completati, assessment revisionati,
spiegazioni e applicazione a problemi nuovi.

Prepared non significa Studied.

## BUILD — Implementare

Costruire un motore generico, giochi originali, strategie
intercambiabili, algoritmi di ricerca e benchmark riproducibili.

Le evidenze comprendono contratti, test, conformità architetturale,
comportamento misurato e risultati riproducibili.

Superare i controlli software non completa automaticamente lo studio.

## SHIP — Validare ed eventualmente pubblicare

Valutare se i giochi originali sono comprensibili, divertenti,
rigiocabili e adatti alla distribuzione mobile.

Le evidenze comprendono playtesting esterno, usabilità,
metriche di gioco, fattibilità tecnica e valutazione economica.

Pubblicazione e ricavi sono opportunità, non obblighi.

## Tracciabilità

| Fase | LEARN | BUILD | SHIP |
| --- | --- | --- | --- |
| 0. Fondamenta | Stati, azioni e contratti | Motore generico | Criteri di giocabilità |
| 1. Tre Sigilli | Modellazione e game design | Gioco, strategia random e match | Prototipo giocabile |
| 2. Minimax | Ricerca avversaria | Strategia deterministica | Avversario computerizzato |
| 3. Alpha-Beta | Complessità e pruning | Ricerca ottimizzata e benchmark | Reattività |
| 4. MCTS | Ricerca statistica negli alberi | Strategia alternativa | Rilevanza per il gameplay |
| 5. Scalabilità | Metodologia sperimentale | Altro gioco e confronti | Playtesting esterno |
| 6. Distribuzione (proposta) | UX, packaging e metriche | Integrazione mobile se giustificata | Eventuale pubblicazione Android |

La fase 6 è un'estensione proposta, non una modifica alla
roadmap software approvata.

## Gate indipendenti

- LEARN_GATE: studio effettivo, esercizi e assessment revisionati.
- BUILD_GATE: implementazione corretta, testata e riproducibile.
- SHIP_GATE: regole originali, rigiocabilità, feedback esterno,
  usabilità e plausibilità economica.

Nessun gate sostituisce gli altri.

## Confini architetturali

- Python e ADR 0001 restano la base iniziale.
- La CLI precede le interfacce grafiche.
- Il motore resta indipendente da giochi e interfacce.
- La tecnologia mobile non viene scelta prematuramente.
- Riuso del codice e riuso del comportamento sono valutati separatamente.
- Asset, presentazione e monetizzazione del prodotto restano separati.
- I contenuti pubblici rispettano repository hygiene e licenze.

## Responsabilità del tracking

- Il progresso didattico resta nel tracker esistente.
- Le milestone software restano nella roadmap esistente.
- Questa mappa collega apprendimento, implementazione e prodotti.
- La validazione commerciale non cambia automaticamente lo studio.

## Primo candidato

Tre Sigilli è il primo candidato per un prototipo giocabile,
non un prodotto commerciale automaticamente approvato.

Regole, profondità strategica e rigiocabilità richiedono validazione.
