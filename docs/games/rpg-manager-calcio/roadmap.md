# RPG Manager Calcio — M001 Roadmap

**Stato:** documentazione pronta; implementazione non iniziata. I gate non sono ancora superati.

| Fase | Deliverable | Closure gate |
|---|---|---|
| M001-A — World Generation | 8 club, 144 calciatori, allenatori, 3 proposte, calendario | WORLD_VALIDITY; ROSTER_COVERAGE; CANDIDATE_DIVERSITY; GENERATION_REPLAY; CALENDAR_VALIDITY |
| M001-B — Match Engine | Possessi, fasi, regole, eventi e replay | MATCH_STATE; MATCH_RULES; MATCH_TIME; EVENT_INTEGRITY; MATCH_REPLAY |
| M001-C — Decisions & AI | Decisioni validate e allenatori con personalità | DECISION_VALIDATION; HUMAN_AI_PARITY; AI_INFORMATION_FAIRNESS; AI_STABILITY; TACTICAL_EFFECT |
| M001-D — RPG Progression | Allenamenti, esperienza, talenti, morale | XP_INTEGRITY; ATTRIBUTE_LIMITS; TALENT_VALIDATION; TRAINING_ONCE; RPG_PERSISTENCE |
| M001-E — Season & CLI | Stagione completa e salvataggi | SEASON_SCHEDULE; RESULT_COMMIT; STANDINGS; SAVE_RESUME; CLI_PLAYABILITY |
| M001-F — Qualification | Dossier di test e campagne statistiche | INVARIANTS; DETERMINISM; METAMORPHIC; STATISTICAL; END_TO_END |

## Specifiche esecutive residue

Fissare prima dell'implementazione dei componenti interessati: inizializzazione/vettori PCG32, tabella logistica canonica, schema dei salvataggi, transizioni di rigori/fuorigioco/autoreti/sospensioni.

**Fonte normativa:** [M001 Engineering Contract v1.0](m001-engineering-contract.md).
