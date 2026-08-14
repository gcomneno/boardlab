# Sessione 01 — Mappa di copertura delle fonti

[English](source-coverage-map.md) | [Italiano](source-coverage-map.it.md)

## Scopo

Questa mappa definisce il confine delle fonti della Sessione 01 prima di
scrivere unità didattiche, esempi, quiz o answer key.

Identifica ciò che la sessione può derivare dal materiale BoardLab esistente,
quale materiale fornisce contesto di supporto, cosa viene deliberatamente
rimandato e dove sarà necessaria un'interpretazione didattica originale.

Questo documento guida la preparazione. Non sostituisce alcun documento fonte.

## Ambito della sessione

La Sessione 01 prepara il modello computazionale necessario per ragionare su un
gioco da tavolo prima di implementare un gioco concreto o un algoritmo di
ricerca.

Il confine concettuale previsto è:

- stato del gioco;
- mosse legali;
- transizioni di stato;
- stati terminali;
- giocatori;
- strategie;
- coordinamento della partita;
- separazione tra regole del gioco e algoritmi decisionali.

## Fonti primarie del repository

### `docs/architecture/overview.md`

Ruolo: fonte architetturale primaria.

Materiale rilevante per questa sessione:

- responsabilità di `Game`;
- responsabilità di `GameState`;
- responsabilità di `Move`;
- responsabilità di `Player`;
- responsabilità di `Strategy`;
- responsabilità di `Match`;
- separazione tra regole specifiche del gioco e comportamento generico del
  motore;
- requisito che la ricerca non modifichi clandestinamente lo stato;
- contratto concettuale minimo per mosse legali, applicazione delle mosse,
  rilevamento degli stati terminali, valutazione e stato indipendente per la
  ricerca.

Confine della copertura:

Il documento definisce responsabilità e vincoli architetturali ma non definisce
ancora interfacce Python definitive o un modello formale completo.

### `README.md`

Ruolo: fonte primaria a livello di progetto.

Materiale rilevante per questa sessione:

- BoardLab è un laboratorio per motori di gioco e algoritmi, non una collezione
  di giochi;
- il motore non deve conoscere giochi specifici;
- gli algoritmi dipendono da contratti astratti;
- motore, gioco e strategia rimangono separati;
- leggibilità e testabilità hanno precedenza sull'ottimizzazione prematura;
- Tre Sigilli è previsto come primo gioco concreto originale dopo la base del
  motore generico.

Confine della copertura:

Il README fornisce principi e direzione del progetto, non contenuto didattico
dettagliato.

## Fonti di supporto del repository

### `docs/roadmap.md`

Ruolo: sequenza e contesto del progetto.

Contributo rilevante:

- colloca il motore generico prima di Tre Sigilli;
- colloca Minimax, Alpha-Beta Pruning e Monte Carlo Tree Search dopo la base
  iniziale del dominio.

La roadmap supporta la sequenza didattica ma non è una fonte sui meccanismi
interni di tali algoritmi in questa sessione.

### `docs/architecture/adr/0001-python-toolchain.md`

Ruolo: supporto al contesto implementativo.

Contributo rilevante:

- spiega perché Python è stato scelto per algoritmi leggibili, contratti
  espliciti, testing e riproducibilità;
- stabilisce vincoli di type checking e testing che il futuro lavoro
  implementativo dovrà rispettare.

Questo ADR non definisce la semantica del dominio dei giochi da tavolo e non
viene usato come fonte per i concetti del modello di gioco.

## Materiale deliberatamente rimandato

La Sessione 01 non tenta di insegnare o implementare:

- regole o gameplay di Tre Sigilli;
- interfacce Python concrete per i contratti del dominio;
- implementazione della strategia casuale;
- Minimax;
- Alpha-Beta Pruning;
- Monte Carlo Tree Search;
- benchmark prestazionali;
- meccaniche avanzate specifiche di singoli giochi;
- UI o rappresentazione grafica.

Questi argomenti richiedono sessioni successive o fasi implementative dedicate.

## Fonti esterne

In questa fase di preparazione non è stato selezionato alcun libro di testo,
regolamento commerciale, manuale protetto o materiale relativo a giochi
commerciali come fonte della Sessione 01.

Eventuali fonti esterne future potranno essere aggiunte soltanto
deliberatamente, con riferimenti bibliografici e un chiaro confine di
pubblicazione.

La loro assenza non deve essere compensata silenziosamente copiando o
ricostruendo materiale protetto.

## Interpretazione originale BoardLab

Il materiale didattico della sessione dovrà necessariamente aggiungere
spiegazioni originali attorno alle dichiarazioni architetturali esistenti.

Il materiale originale può includere:

- modelli mentali per rappresentare il gameplay come transizioni di stato;
- esempi inventati di stati di gioco minimi;
- diagrammi originali;
- scenari di errore e modellazione;
- distinzioni tra regole, stato, strategia e coordinamento della partita;
- esercizi e domande di quiz originali.

Questi artefatti devono restare identificabili come materiale didattico
BoardLab e non come riproduzioni di una fonte esterna.

## Confine copyright e pubblicazione

La sessione potrà fare riferimento a concetti o sezioni specifiche di future
fonti esterne, ma il repository pubblico non deve contenere sostituti delle
fonti come:

- PDF di regolamenti commerciali;
- scansioni;
- artwork protetto;
- raccolte sostanziali di testi delle carte;
- regolamenti copiati;
- traduzioni sostanziali;
- trascrizioni delle fonti.

Il materiale privato, se mai necessario per lo studio, appartiene a
`sources/private/` e deve restare fuori dalla cronologia Git.

## Conseguenza per la preparazione

La copertura attuale delle fonti supporta la preparazione di una sessione
fondamentale sul modello astratto di gioco di BoardLab.

Non supporta ancora affermazioni su specifici giochi commerciali o algoritmi di
ricerca dettagliati.

Qualsiasi futura espansione oltre questo confine richiede un aggiornamento
esplicito delle fonti prima di aggiungere il relativo materiale didattico.
