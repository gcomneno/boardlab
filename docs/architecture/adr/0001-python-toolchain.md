# ADR 0001 — Python come linguaggio iniziale

- Stato: approvata
- Data: 2026-07-18

## Contesto

BoardLab è un laboratorio didattico per studiare motori di gioco, algoritmi di
ricerca e intelligenza artificiale applicata ai giochi da tavolo.

Il linguaggio iniziale deve favorire:

- leggibilità degli algoritmi;
- modellazione esplicita dei contratti;
- test rapidi e comprensibili;
- misurazione delle prestazioni;
- bassa quantità di codice infrastrutturale;
- riproducibilità dell'ambiente di sviluppo.

## Decisione

BoardLab utilizza Python 3.12 o successivo.

La toolchain iniziale è composta da:

- `uv` per ambiente virtuale, dipendenze e lock file;
- Ruff per formattazione e analisi statica;
- mypy in modalità strict per il controllo dei tipi;
- pytest per i test automatici;
- Hatchling come backend di build;
- layout `src/` per il package installabile.

## Motivazioni

### Leggibilità

Python permette di rappresentare algoritmi come Minimax, Alpha-Beta Pruning e
Monte Carlo Tree Search con poco rumore sintattico.

### Contratti espliciti

I type hint e mypy strict permettono di rendere visibili le dipendenze tra
motore, giochi e strategie senza rinunciare alla semplicità del linguaggio.

### Testabilità

pytest consente di descrivere il comportamento atteso con test piccoli e
leggibili.

### Riproducibilità

`uv.lock` conserva le versioni risolte delle dipendenze. L'ambiente può essere
ricreato senza dipendere da pacchetti Python installati globalmente.

### Isolamento del package

Il layout `src/boardlab` impedisce ai test di importare accidentalmente il
codice direttamente dalla directory del repository invece che dal package
installato.

## Alternative considerate

### PHP

È un linguaggio ben conosciuto dal manutentore, ma meno adatto come scelta
iniziale per esprimere algoritmi e strutture dati in un laboratorio didattico
generalista.

### TypeScript

Offre un buon sistema di tipi e una toolchain matura, ma introduce più
infrastruttura legata all'ecosistema Node.js.

### Rust

Offre prestazioni e garanzie forti, ma il costo cognitivo iniziale rischierebbe
di spostare l'attenzione dagli algoritmi al linguaggio.

### Java

Offre tipi statici e ottimi strumenti, ma richiede maggiore cerimonia per gli
esperimenti iniziali.

## Conseguenze

### Positive

- gli algoritmi possono essere implementati in modo vicino allo pseudocodice;
- i contratti possono essere verificati staticamente;
- l'ambiente è riproducibile;
- test e benchmark possono condividere la stessa toolchain.

### Negative

- Python non rappresenta il riferimento assoluto per le prestazioni;
- mypy non rende Python un linguaggio staticamente tipizzato;
- eventuali confronti prestazionali dovranno distinguere qualità
  dell'algoritmo e costo del runtime.

## Vincoli conseguenti

- il codice deve superare Ruff, mypy strict e pytest;
- le dipendenze devono essere dichiarate in `pyproject.toml`;
- `uv.lock` deve essere versionato;
- non si devono usare dipendenze globali come requisito implicito;
- le ottimizzazioni non devono ridurre la leggibilità senza misure che le
  giustifichino.
