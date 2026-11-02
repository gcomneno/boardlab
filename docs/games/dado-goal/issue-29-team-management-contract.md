# Dado Goal! — Issue #29: Team Management MVP Contract

Status: DESIGN APPROVED — IMPLEMENTATION NOT STARTED
Scope: Optional team management for single-player and multiplayer.
Baseline: M2D public release, commit 0303e971.

## 1. Principles

- The original quick-match workflow remains available without registration, API access or management data.
- Teams and players are imported from real football data providers.
- Imported data is descriptive and must not change dice probabilities, scoring or match rules.
- Team management is optional and explicitly activated by the player.
- The single-player remains playable offline.
- The multiplayer server remains authoritative for match state and events.

## 2. MVP phases

### M1 — Football catalogue

Import real teams, players, roles and shirt numbers.

Store provider identity, competition and season metadata.

Maintain a local catalogue with explicit refresh, import and export operations.

### M2 — Team selection

Allow selection of two imported teams before starting a match.

Associate provider player identities with match-local shirt numbers.

Preserve the existing quick-match fallback using createRoster().

Validate multiplayer selections server-side before accepting them.

### M3 — Sports management

Record match results, statistics and manually entered player ratings.

Keep simulated match records separate from imported real-world statistics.

Provide local history and JSON import/export.

Standings, seasons, cups and reports are future extensions.

## 3. Minimum data model

Team:
- provider
- provider_team_id
- name
- competition_id
- season

Player:
- provider
- provider_player_id
- name
- position
- shirt_number (nullable)

Roster:
- team reference
- player references
- starting players
- substitutes
- source season
- import timestamp

Match record:
- local match ID
- participating team snapshots
- result
- event history
- match statistics
- optional manually entered ratings

Provider identifiers are not interchangeable with shirt numbers.

Imported rosters are snapshots: subsequent provider updates must not
retroactively change completed matches.

## 4. Persistence and privacy

- Local-first persistence with explicit user control.
- JSON import/export with schema versioning and validation.
- No mandatory accounts or cloud synchronization.
- No provider API credentials embedded in public HTML or JavaScript.
- No automatic external transmission of locally stored match history.
- Provider provenance and refresh timestamps must be retained.
- Users must be able to remove locally stored management data.

## 5. Provider integration

API-Football is the initial candidate, not an approved dependency.

Before integration, verify:
- permitted uses and redistribution rights;
- free-tier limits and relevant competition coverage;
- current squad and player data availability;
- authentication and credential-handling requirements.

The provider adapter must be replaceable.

Import failures or exhausted API quotas must not block gameplay.

## 6. Engine compatibility

Existing integration points:
- web/dado-goal/app.mjs
- cloudflare/dado-goal/match-events-authority.mjs

Keep createRoster() as the default quick-match path.

The imported roster adapter must preserve:
- 11 starting players;
- valid substitutes;
- maximum five substitutions;
- no return after substitution or expulsion;
- goalscorer eligibility among active players.

Imported identities must be mapped without changing dice mechanics.

For multiplayer, the server validates roster selections and persists
the accepted snapshot with the match. Browser data is not authoritative.

## 7. Ratings

Player ratings are manually entered and descriptive.

No rating formula is introduced.

Ratings must not affect random outcomes, scoring or match probabilities.

Any future gameplay effects require a separate contract and approval.

## 8. Acceptance gates

- [ ] Provider terms and coverage verified.
- [ ] Versioned data schema and validation tests.
- [ ] Import/export round-trip tests.
- [ ] Offline quick-match regression tests.
- [ ] Real-team selection tests.
- [ ] Multiplayer authority and recovery tests.
- [ ] Privacy and credential-handling review.
- [ ] Game changelog updated upon implementation.

## 9. Exclusions

- No changes to the released M2D gameplay.
- No immediate deployment.
- No scraping.
- No automatic player ratings.
- No mandatory provider dependency.
- No league or cup management in the initial MVP.

Implementation requires separate approval.
