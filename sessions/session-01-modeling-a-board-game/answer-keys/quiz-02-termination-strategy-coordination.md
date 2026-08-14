# Quiz 02 — Answer key: Termination, strategy, and coordination

[English](quiz-02-termination-strategy-coordination.md) | [Italiano](quiz-02-termination-strategy-coordination.it.md)

This review material corresponds to:

`../quizzes/quiz-02-termination-strategy-coordination.md`

It should be consulted only after an actual quiz attempt.

## Question 1

Expected answer:

B.

Reasoning:

A state may be strategically favorable while the game is still ongoing.

Terminal-state detection answers whether the rules say play has ended.

Strategic evaluation answers how desirable a state appears from a decision
perspective.

The two concepts are related but distinct.

Typical misconception:

"A sufficiently good or bad evaluation means the state is terminal."

Why that fails:

Evaluation may also be useful for non-terminal states, especially during
search.

Practical takeaway:

Keep rule-defined termination separate from strategic evaluation.

## Question 2

Expected answer:

A.

Reasoning:

The same terminal `GameState` represents one objective completed game
situation, but its outcome can be interpreted differently depending on the
`Player` perspective.

A North victory is simultaneously a South defeat.

Typical misconception:

"A terminal state has one universal outcome label that means the same thing to
every participant."

Why that fails:

Win and loss are inherently relative to a participant unless the game uses a
perspective-neutral result representation that is interpreted later.

Practical takeaway:

Outcome reasoning must preserve player perspective explicitly.

## Question 3

Expected answer:

A.

Reasoning:

North and South identify participants.

First Legal and Random Legal describe move-selection behavior.

Because those responsibilities are separate, the strategies can be swapped
without changing the game rules or the player identities.

Typical misconception:

"A player permanently owns one decision algorithm."

Why that fails:

Fusing identity and strategy makes controlled experiments and strategy
comparison unnecessarily difficult.

Practical takeaway:

Separate `Player` identity from `Strategy` behavior so algorithms remain
interchangeable.

## Question 4

Expected answer:

A.

Reasoning:

The statement about space 2 being selectable only when empty is a concrete game
rule.

A generic search strategy should obtain legal choices through the public game
contract rather than know that rule directly.

Typical misconception:

"A strategy needs concrete rules in order to make safe decisions."

Why that fails:

The strategy needs authoritative legal choices, not duplicated knowledge of how
the concrete game determines them.

Practical takeaway:

Keep concrete game semantics inside the game domain and generic decision logic
inside `Strategy`.

## Question 5

Expected answer:

C.

Reasoning:

`Match` is responsible for orchestration.

It coordinates the active player, the associated strategy, the selected move,
the game-domain transition, and the terminal check.

It should not own concrete game rules or decision algorithms.

Typical misconception:

"The component controlling the turn should also know how every rule works."

Why that fails:

Coordination requires invoking responsibilities, not absorbing them.

Practical takeaway:

A generic `Match` should remain a thin orchestrator.

## Question 6

Expected answer:

B.

Reasoning:

If adding a new game requires changing generic `Match` because it knows details
about an old board layout, concrete game knowledge has leaked into generic
orchestration.

That breaks BoardLab's intended dependency boundary.

Typical misconception:

"Generic orchestration naturally needs special cases for every game."

Why that fails:

The purpose of the game contract is precisely to keep those special rules
behind the concrete game implementation.

Practical takeaway:

Adding a new game should primarily require implementing the game contract, not
teaching generic `Match` new board rules.

## Review summary

Quiz 02 tests three major boundaries:

- terminality and outcome belong to game-domain semantics and must remain
  distinct from strategic evaluation;
- `Player` identity and `Strategy` behavior are separate and interchangeable;
- `Match` coordinates the system without absorbing concrete game rules or
  decision algorithms.

Together with Quiz 01, these concepts cover the complete conceptual model
prepared by Session 01.
