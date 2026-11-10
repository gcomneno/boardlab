# Dado Goal GUI parity contract

## Product invariant

The multiplayer match GUI derives from the single-player match GUI.

Except for functionality that necessarily belongs to multiplayer, a shared
presentation change is incomplete until both surfaces expose the same user
experience.

This is a presentation contract. It does not require the single-player and
multiplayer orchestration code to be identical.

## Canonical ownership

Shared presentation has the following ownership model:

- `web/dado-goal/style.css` is the canonical shared stylesheet.
- `web/dado-goal/i18n.mjs` is the canonical localization resource.
- `cloudflare/dado-goal/public/i18n.mjs` is a deployed copy and must remain
  byte-identical to the canonical localization resource.
- The single-player match DOM is the reference structure for common match
  presentation.
- Multiplayer may wrap that structure with multiplayer-specific access,
  invitation and synchronization UI, but must preserve the common match
  contract.

## Required common match surface

Both modes expose the following presentation elements:

- scoreboard teams `panel-0` and `panel-1`;
- match chronicle `match-events-list` and its empty state;
- match clock and current turn;
- `pitch-branding` containing `pitch-logo-0`, the central die and
  `pitch-logo-1`;
- dice guide;
- phase and result presentation;
- roll and AUTO controls;
- match commentary and commentary toggle;
- match statistics;
- common status/message presentation.

Where an authoritative event carries a real player identity, both modes use
the shared player-identity display contract for goals, penalties, cards and
substitutions.

## Intentional multiplayer differences

The following are outside parity and remain independently implemented:

- authentication and logout;
- create/join lifecycle;
- invitation code and copy-invite controls;
- network and synchronization state;
- seat and turn authority;
- authoritative server lifecycle and replay;
- multiplayer AUTO orchestration.

In particular, multiplayer AUTO is intentionally not the configurable
single-player AUTO mode. The single-player AUTO configuration dialog is
therefore not required in multiplayer.

## Regression policy

Automated parity tests guard structural contracts rather than pixel-perfect
HTML. They must detect accidental loss or drift of:

- common match DOM elements;
- pitch team crests and their ordering around the die;
- chronicle and statistics containers;
- shared stylesheet dependency;
- shared localization resources.

Behavioral tests remain responsible for event projection, player identity,
commentary, statistics and game rules.

A new multiplayer-only element does not require a corresponding
single-player element when it belongs to one of the explicit exceptions
above.

## Release gate

Changes affecting the common match presentation require:

1. parity regression tests;
2. relevant existing single-player and multiplayer tests;
3. mandatory manual road tests of both surfaces before publication.
