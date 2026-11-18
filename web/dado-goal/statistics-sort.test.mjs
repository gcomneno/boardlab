import assert from "node:assert/strict";
import test from "node:test";
import {
  nextStatisticsSort,
  sortStatisticsRows,
} from "./statistics-sort.mjs";

test("three clicks cycle ASC, DESC, original", () => {
  const asc = nextStatisticsSort(null, "goals");
  const desc = nextStatisticsSort(asc, "goals");
  const original = nextStatisticsSort(desc, "goals");

  assert.deepEqual(asc, { column: "goals", direction: "asc" });
  assert.deepEqual(desc, { column: "goals", direction: "desc" });
  assert.equal(original, null);
});

test("clicking a different column starts ASC", () => {
  assert.deepEqual(
    nextStatisticsSort({ column: "goals", direction: "desc" }, "assists"),
    { column: "assists", direction: "asc" },
  );
});

test("numeric statistics sort in either direction", () => {
  const rows = [{ goals: 10 }, { goals: 2 }, { goals: 5 }];
  assert.deepEqual(
    sortStatisticsRows(rows, { column: "goals", direction: "asc" })
      .map(row => row.goals),
    [2, 5, 10],
  );
  assert.deepEqual(
    sortStatisticsRows(rows, { column: "goals", direction: "desc" })
      .map(row => row.goals),
    [10, 5, 2],
  );
});

test("player names sort naturally", () => {
  const rows = [{ name: "Zeta" }, { name: "Alfa" }];
  assert.deepEqual(
    sortStatisticsRows(rows, { column: "name", direction: "asc" })
      .map(row => row.name),
    ["Alfa", "Zeta"],
  );
});

test("reset restores input order without mutation", () => {
  const rows = [{ goals: 3 }, { goals: 1 }, { goals: 2 }];
  const sorted = sortStatisticsRows(rows, {
    column: "goals",
    direction: "asc",
  });
  assert.deepEqual(sorted.map(row => row.goals), [1, 2, 3]);
  assert.deepEqual(sortStatisticsRows(rows, null), rows);
  assert.deepEqual(rows.map(row => row.goals), [3, 1, 2]);
});

test("ties remain stable and missing values stay last", () => {
  const rows = [
    { name: "A", assists: 2 },
    { name: "B", assists: null },
    { name: "C", assists: 2 },
  ];
  for (const direction of ["asc", "desc"]) {
    assert.deepEqual(
      sortStatisticsRows(rows, { column: "assists", direction })
        .map(row => row.name),
      ["A", "C", "B"],
    );
  }
});
