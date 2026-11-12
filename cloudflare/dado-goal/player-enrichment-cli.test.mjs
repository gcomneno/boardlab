import assert from "node:assert/strict";
import test from "node:test";
import {
  mkdtemp,
  readFile,
  writeFile,
} from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";

import {
  exportCatalogueJson,
} from "../../web/dado-goal/catalogue-json.mjs";
import {
  createCatalogue,
  createPlayer,
  createTeam,
} from "../../web/dado-goal/team-catalogue.mjs";
import {
  runPlayerEnrichmentCli,
} from "./player-enrichment-cli.mjs";

function response(body) {
  return {
    ok: true,
    status: 200,
    async json() {
      return body;
    },
  };
}

test("CLI batch persists checkpoint and resumes without /teams", async () => {
  const directory = await mkdtemp(join(tmpdir(), "dado-goal-m2-"));
  const catalogueFile = join(directory, "catalogue.json");
  const checkpointFile = join(directory, "checkpoint.json");
  const secretsFile = join(directory, ".env.secrets");

  const teams = ["10", "20"].map((teamId) => ({
    ...createTeam({
      provider: "api-football",
      providerTeamId: teamId,
      name: `Team ${teamId}`,
      competitionId: "39",
      season: "2024",
      colors: ["#000000"],
      logo: `team-${teamId}.png`,
      enrichment: { country: "England" },
    }),
    players: [createPlayer({
      provider: "api-football",
      providerPlayerId: `${teamId}1`,
      name: `Player ${teamId}`,
      position: "Midfielder",
      shirtNumber: 8,
    })],
  }));

  const catalogue = createCatalogue({
    provider: "api-football",
    competitionId: "39",
    season: "2024",
    importedAt: "2026-11-12T00:00:00.000Z",
    teams,
  });

  await writeFile(catalogueFile, exportCatalogueJson(catalogue));
  await writeFile(secretsFile, "API_FOOTBALL_KEY=fake-test-key\n");

  const requests = [];

  const fetchImpl = async (url) => {
    requests.push({
      path: url.pathname,
      team: url.searchParams.get("team"),
    });

    assert.equal(url.pathname, "/players/squads");

    const teamId = url.searchParams.get("team");

    return response({
      errors: [],
      response: [{
        team: { id: Number(teamId) },
        players: [{
          id: Number(`${teamId}1`),
          name: `Player ${teamId}`,
          age: 25,
          number: 8,
          position: "Midfielder",
          photo: `player-${teamId}.png`,
        }],
      }],
    });
  };

  const first = await runPlayerEnrichmentCli({
    catalogueFile,
    checkpointFile,
    secretsFile,
    requestBudget: 1,
    requestIntervalMs: 0,
    fetchImpl,
  });

  assert.equal(first.complete, false);

  const second = await runPlayerEnrichmentCli({
    catalogueFile,
    checkpointFile,
    secretsFile,
    requestBudget: 1,
    requestIntervalMs: 0,
    fetchImpl,
  });

  assert.equal(second.complete, true);
  assert.deepEqual(requests, [
    { path: "/players/squads", team: "10" },
    { path: "/players/squads", team: "20" },
  ]);

  const checkpoint = JSON.parse(
    await readFile(checkpointFile, "utf8"),
  );

  assert.deepEqual(checkpoint.completedTeamIds, ["10", "20"]);
  assert.equal(checkpoint.snapshots.length, 2);
  assert.equal(checkpoint.snapshots[0].age, 25);
});
