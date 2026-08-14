# Quiz 01 — Answer key: State, legality, and transitions

[English](quiz-01-state-legality-transitions.md) | [Italiano](quiz-01-state-legality-transitions.it.md)

This review material corresponds to:

`../quizzes/quiz-01-state-legality-transitions.md`

It should be consulted only after an actual quiz attempt.

## Question 1

Expected answer:

B.

Reasoning:

The visible board is not sufficient to describe the complete game situation
because gameplay also depends on which player is active.

If that information exists only in a global variable outside `GameState`, two
apparently identical states can behave differently depending on hidden context.

This violates the Session 01 requirement that a state be complete for the
gameplay rules that depend on it.

Typical misconception:

"If the program can access the information somewhere, it does not need to be
part of the state."

Why that fails:

Search, testing, replay, and reasoning need one explicit description of the
current situation rather than a snapshot whose meaning depends on unrelated
mutable context.

Practical takeaway:

Gameplay-relevant information that affects what can happen next belongs in the
state model or in another explicit domain relationship, not in invisible global
context.

## Question 2

Expected answer:

C.

Reasoning:

Successor A and successor B represent different hypothetical futures derived
from the same starting state.

Changing one successor must not unexpectedly modify another.

If the two branches share mutable gameplay data, they cannot be reasoned about
independently.

Typical misconception:

"Separate variables automatically mean separate states."

Why that fails:

Two objects may still refer to the same mutable nested structures.

Practical takeaway:

Search requires independently usable hypothetical states at the observable
domain level.

## Question 3

Expected answer:

B.

Reasoning:

The value `1` describes an action candidate, but its legality depends on both:

- the current game state;
- the game's rules.

In the given state, space 1 is already occupied.

The representation of the move alone cannot determine whether it is allowed.

Typical misconception:

"If a `Move` value can be constructed, it is legal."

Why that fails:

Representability describes how an action is expressed. Legality is a domain
decision relative to one particular state.

Practical takeaway:

Keep move representation separate from rule-based legality.

## Question 4

Expected answer:

B.

Reasoning:

The concrete rule about empty spaces belongs to the game domain.

Duplicating that rule inside multiple strategies makes generic decision
algorithms depend on one concrete game and creates several competing copies of
the same rule.

A later rule change can therefore make the strategies disagree.

Typical misconception:

"Each strategy should validate moves for safety."

Why that fails:

Validation is useful, but authoritative game-specific legality must not be
reimplemented independently inside every strategy.

Practical takeaway:

Generic strategies should consume legal choices through the public game
contract rather than own concrete game rules.

## Question 5

Expected answer:

C.

Reasoning:

Two different properties are being checked:

1. whether North's selected move is legal from the current state;
2. whether applying that move produces a valid successor state.

The first property can succeed while the second fails.

Because the rules require alternating turns, a successor that still records
North as active violates the transition semantics.

Typical misconception:

"A legal input guarantees a correct output."

Why that fails:

The legality check validates the candidate action. The transition still has to
apply every required state change correctly.

Practical takeaway:

Test move legality and transition correctness as distinct domain properties.

## Question 6

Expected answer:

A.

Reasoning:

Candidate B is no longer evaluated from the same original state because
evaluating candidate A mutated that state.

The branches therefore contaminate one another.

The central problem is failure to preserve an independent starting point for
hypothetical exploration.

Typical misconception:

"The algorithm can mutate the current state temporarily as long as it knows the
order of evaluation."

Why that fails:

Search correctness should not depend on accidental evaluation order or perfect
manual rollback of hidden mutations.

Practical takeaway:

Hypothetical transitions must preserve branch independence so every candidate
can be evaluated from the intended state.

## Review summary

Quiz 01 tests three boundaries:

- `GameState` must explicitly represent the gameplay-relevant situation;
- legal `Move` values are determined by game rules applied to that state;
- applying a move must produce a complete, valid, independently usable
  successor state.

These concepts form the base required before reasoning about terminal states,
strategies, and match coordination.
