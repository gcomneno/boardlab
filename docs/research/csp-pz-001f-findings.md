# PZ-001F — CSP semantic diversity and operational difficulty

Status: exploratory findings qualified locally.
Scope: generic finite-domain CSP research only.

## Research objective

Determine whether BoardLab can distinguish genuinely different
constraint problems from alternative formulations of the same
solution space, and separate that diversity from solver effort.

This research does not certify autonomous invention of new games,
human difficulty, or universal CSP equivalence.

## Qualified foundation

The generic CSP model supports:

- EQUAL
- NOT_EQUAL
- ALL_DIFFERENT
- SUM_EQUALS

The deterministic solver enumerates satisfying assignments.
The generator selects constraint subsets according to a required
solution count and reports FOUND, EXHAUSTED or BUDGET_EXHAUSTED.

## Semantic equivalence

An exact exhaustive reference implementation compares complete
solution sets, including equivalence under bijective variable
renaming with fixed domain values.

It does not implement arbitrary domain-value relabeling,
general CSP isomorphism, or scalable equivalence testing.

Research commit: 83f23ac.

## Semantic diversity

The bounded classifier groups candidate problems into semantic
equivalence classes.

Research commit: 340a121.

Controlled catalog experiment:

| Catalog | Candidates | Classes | Ratio |
| --- | ---: | ---: | ---: |
| Equality only | 8 | 3 | 0.3750 |
| Equality and difference | 16 | 7 | 0.4375 |
| Mixed relations | 16 | 8 | 0.5000 |

These figures describe only the specified finite catalogs.
Different catalog sizes prevent attribution of the ratio
differences solely to constraint-type composition.

## Class quality

| Catalog | UNSAT classes | Unique classes | Multiple classes |
| --- | ---: | ---: | ---: |
| Equality only | 0 | 0 | 3 |
| Equality and difference | 1 | 0 | 6 |
| Mixed relations | 1 | 1 | 6 |

Every unsatisfiable problem with the same fixed variable/domain
interface has the empty solution set. It therefore belongs to
the same semantic class under this equivalence definition.

Unique solutions and satisfiability are not difficulty metrics.

## Operational search effort

The exhaustive reference checks all 27 complete assignments
for each three-variable, three-value-domain problem.

Instrumenting the existing deterministic solver instead exposed
variation in consistency-check counts across equivalent formulations:

| Catalog | Equivalent classes with varying cost |
| --- | ---: |
| Equality only | 2 |
| Equality and difference | 4 |
| Mixed relations | 2 |

Variable-order sensitivity, six permutations per example:

| Example | Solutions | Minimum calls | Maximum calls |
| --- | ---: | ---: | ---: |
| Equality chain | 3 | 24 | 42 |
| Mixed unique | 1 | 16 | 34 |
| Contradiction | 0 | 12 | 39 |

These measurements count calls to the internal `_consistent`
predicate. They are not CPU timings, uniform search-tree node
counts, or validated measurements of human difficulty.

The search implementation and pruning strategy were unchanged.

## Findings

1. Distinct declarative formulations can be semantically equivalent.
2. Constraint catalog size alone does not determine semantic diversity.
3. Unsatisfiable formulations can inflate syntactic candidate counts.
4. Equivalent problems can incur different operational solver costs.
5. Variable ordering can alter those costs without changing solutions.

## Boundaries and open problems

- Measurements are restricted to small, explicitly declared catalogs.
- Equivalence classification is exhaustive and budget-bounded.
- No generalization to arbitrary CSP instances has been established.
- No human-oriented difficulty model has been validated.
- No criterion for autonomous invention or originality is established.
- No inference of statistical significance is claimed.

## Next research question

Define a falsifiable originality criterion relative to an explicit
reference corpus and equivalence relation.

An instance being new to a finite catalog is not, by itself,
evidence of a new puzzle mechanic or an original game.

Do not integrate speculative originality claims into production
generation APIs without further qualification.
