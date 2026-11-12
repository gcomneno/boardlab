import assert from "node:assert/strict";
import test from "node:test";

import {
  selectLatestTeamCoach,
} from "./coach-selection.mjs";

test("selects coach with latest career start for the requested team", () => {
  const coaches = [
    {
      id: 783,
      name: "P. Pezzolano",
      career: [{
        team: { id: 38 },
        start: "2025-05-01",
        end: null,
      }],
    },
    {
      id: 1612,
      name: "A. Dionisi",
      career: [{
        team: { id: 38 },
        start: "2026-08-01",
        end: null,
      }],
    },
    {
      id: 25654,
      name: "Javier Gracia Carlos",
      career: [{
        team: { id: 38 },
        start: "2025-07-01",
        end: null,
      }],
    },
  ];

  assert.equal(selectLatestTeamCoach(coaches, 38)?.id, 1612);
});

test("ignores newer career entries belonging to another team", () => {
  const coaches = [{
    id: 10,
    name: "Coach A",
    career: [
      {
        team: { id: 38 },
        start: "2025-01-01",
        end: null,
      },
      {
        team: { id: 99 },
        start: "2026-01-01",
        end: null,
      },
    ],
  }, {
    id: 20,
    name: "Coach B",
    career: [{
      team: { id: 38 },
      start: "2025-06-01",
      end: null,
    }],
  }];

  assert.equal(selectLatestTeamCoach(coaches, "38")?.id, 20);
});

test("does not use null end as the current-coach signal", () => {
  const coaches = [{
    id: 10,
    career: [{
      team: { id: 38 },
      start: "2024-01-01",
      end: null,
    }],
  }, {
    id: 20,
    career: [{
      team: { id: 38 },
      start: "2025-01-01",
      end: "2025-12-01",
    }],
  }];

  assert.equal(selectLatestTeamCoach(coaches, 38)?.id, 20);
});

test("returns null when no career entry matches the team", () => {
  assert.equal(
    selectLatestTeamCoach([{
      id: 10,
      career: [{
        team: { id: 99 },
        start: "2025-01-01",
        end: null,
      }],
    }], 38),
    null,
  );
});
