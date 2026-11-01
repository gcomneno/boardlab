# Slot Trasparente — Esperimento 001

[English](mission-001.md) | [Italiano](mission-001.it.md)

- **Progetto:** GiadaWare BoardLab
- **Documento:** Contratto matematico e didattico M001
- **Versione:** 1.0
- **Stato:** specifica iniziale; implementazione non eseguita
- **Dominio:** simulazione con crediti virtuali, senza denaro reale

## 1. Scopo

Slot Trasparente è un esperimento didattico sulla casualità,
sul valore atteso e sul rischio di rovina.

Il gioco espone probabilità, puntate, incassi, risultato netto
e differenza fra risultati osservati e valori teorici.

Non è uno strumento terapeutico e non promette di curare
il disturbo da gioco d'azzardo.

Principio: il sistema spiega gli esiti, senza manipolarli.

## 2. Rulli

- Cinque rulli indipendenti.
- Tre righe visibili per rullo.
- Una sola linea pagante: quella centrale.
- Venti posizioni equiprobabili su ogni striscia circolare.
- Le frequenze dei simboli sono identiche su tutti i rulli.
- Ogni giro costa un credito virtuale.
- Nessun jolly o bonus in M001.

| Simbolo | Posizioni per rullo | Probabilità |
|---|---:|---:|
| Ciliegia | 6 | 30% |
| Limone | 5 | 25% |
| Arancia | 4 | 20% |
| Uva | 3 | 15% |
| Campana | 1 | 5% |
| Sette | 1 | 5% |

L'ordine fisico dei simboli sulle strisce resta da definire.
Non modifica le probabilità della linea centrale, ma determina
i simboli visualizzati sopra e sotto di essa.

## 3. Regole di pagamento

Pagano esclusivamente 3, 4 o 5 simboli uguali consecutivi
a partire dal primo rullo a sinistra.

Si paga soltanto la combinazione più lunga.
I premi rappresentano l'incasso totale, puntata inclusa.

Per un simbolo con probabilità p:

- Esattamente 3: p^3 × (1-p).
- Esattamente 4: p^4 × (1-p).
- Esattamente 5: p^5.

I rulli successivi alla prima interruzione della combinazione
non influiscono sul premio.

## 4. Tabellone Banco favorito

| Simbolo | Premio 3 | Premio 4 | Premio 5 |
|---|---:|---:|---:|
| Ciliegia | 5 | 15 | 40 |
| Limone | 8 | 25 | 80 |
| Arancia | 12 | 40 | 150 |
| Uva | 20 | 80 | 300 |
| Campana | 100 | 500 | 2500 |
| Sette | 150 | 1000 | 5000 |

Ritorno teorico esatto: 546167/640000 = 85,33859375%.
Perdita attesa per giro: 0,1466140625 crediti.

## 5. Tabellone Equo

| Simbolo | Premio 3 | Premio 4 | Premio 5 |
|---|---:|---:|---:|
| Ciliegia | 6 | 18 | 47 |
| Limone | 9 | 29 | 94 |
| Arancia | 14 | 47 | 176 |
| Uva | 23 | 96 | 352 |
| Campana | 117 | 586 | 2930 |
| Sette | 176 | 1176 | 5859 |

Ritorno teorico esatto: 1 = 100%.
Risultato netto atteso per giro: zero.

L'equità matematica non garantisce pareggio,
recupero delle perdite o sopravvivenza del capitale.

## 6. Tabellone Personalizzato

I premi sono modificabili e il ritorno viene ricalcolato.
Rulli, frequenze e regole di pagamento restano invariati.

Il tabellone è fissato all'inizio di ogni sessione.
Validazione dei premi e limiti dell'editor da specificare.

## 7. Probabilità e valore atteso

Per ciascun simbolo s, con probabilità p_s e premi P3, P4, P5:

RTP = somma_s [
    p_s^3 × (1-p_s) × P3
  + p_s^4 × (1-p_s) × P4
  + p_s^5 × P5
].

Probabilità teorica di ottenere un premio: 5,425%.

Risultato netto del giro = incasso - 1.
Risultato netto cumulativo = totale incassato - totale puntato.

La verifica matematica usa aritmetica razionale esatta;
la presentazione decimale non determina la validità del contratto.

## 8. Riproducibilità

Il motore sarà implementato in ISO C11.

Un seme esplicito renderà riproducibile la sequenza.
Algoritmo PRNG, campionamento uniforme delle venti posizioni,
strisce, formato del registro e verifica indipendente
devono essere specificati prima dell'implementazione.

Non è consentito alterare gli esiti per correggere
artificialmente la frequenza osservata o il rendimento.

Tabelloni diversi possono essere confrontati sulla medesima
sequenza di posizioni estratte.

## 9. Modalità Gioca / Studia

Mostrare durante la sessione:

- Capitale disponibile e puntata.
- Totale puntato, incassato e risultato netto.
- Risultato teorico atteso del tabellone scelto.
- Frequenza osservata e teorica dei premi.
- Spiegazione di posizioni, simboli, combinazione e incasso.
- Passaggi dell'algoritmo di valutazione.

Il risultato del singolo giro e quello cumulativo
devono rimanere chiaramente distinti.

## 10. Modalità Esperimento

Configurazione iniziale: 100 sessioni da 100 giri,
con capitale iniziale di 100 crediti per sessione.

Presentare grafici, risultati individuali,
sessioni esaurite, primo giro di esaurimento
e sessioni ancora attive.

Con questa configurazione il capitale non può esaurirsi
prima del centesimo giro: l'orizzonte e il capitale
dovranno quindi diventare configurabili.

Una sessione giocabile termina quando il capitale
non consente la puntata successiva.

Eventuali estrazioni successive appartengono soltanto
a una simulazione contabile esplicitamente separata.

Per le sessioni sopravvissute all'orizzonte mostrare:
"Saldo non esaurito entro i giri osservati".

## 11. Messaggi didattici

- Le perdite non aumentano la probabilità del giro seguente.
- Una vincita non modifica le probabilità future.
- Un RTP del 100% non garantisce un pareggio individuale.
- Equità, durata del capitale e rischio di rovina sono distinti.
- Un risultato positivo del giro non implica un saldo positivo.
- Con capitale iniziale finito e senza ricariche, continuando
  indefinitamente, anche il tabellone equo raggiunge
  la rovina con probabilità 1; non esiste un giro finito
  entro cui la rovina sia garantita.

Nessuna ricarica automatica e nessuna celebrazione
sproporzionata degli incassi.

## 12. Confini M001

Questa fase formalizza la specifica, non il software.

Restano da definire prima del motore operativo:
ordine delle strisce, PRNG e campionamento, gestione del seme,
formato del registro, validazione dei premi personalizzati,
parametri degli esperimenti e criteri di test.

Il motore C11 dovrà essere indipendente dall'interfaccia.
La CLI precederà qualsiasi interfaccia grafica.

Nessun impiego di denaro reale.
