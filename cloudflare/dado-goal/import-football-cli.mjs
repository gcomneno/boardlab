import { readFile, writeFile } from "node:fs/promises";
import { buildFootballCatalogue } from "./import-football-catalogue.mjs";

function parseSecrets(source) {
  return Object.fromEntries(
    source
      .split(/\r?\n/)
      .map((line) => line.trim())
      .filter((line) => line && !line.startsWith("#"))
      .map((line) => {
        const separator = line.indexOf("=");
        return [
          line.slice(0, separator).trim(),
          line.slice(separator + 1).trim(),
        ];
      }),
  );
}

const [competitionId, season, outputFile] = process.argv.slice(2);

if (!/^\d+$/.test(competitionId ?? "") ||
    !/^\d{4}$/.test(season ?? "") ||
    !outputFile) {
  throw new Error(
    "usage: node import-football-cli.mjs COMPETITION SEASON OUTPUT.json",
  );
}

const secrets = parseSecrets(await readFile(".env.secrets", "utf8"));
const apiKey = secrets.API_FOOTBALL_KEY;

if (!apiKey) {
  throw new Error("API_FOOTBALL_KEY missing from .env.secrets");
}

const excludedTeams = {
  "79:2024": new Set(["1639"]),
  "62:2024": new Set(["1299"]),
};

const excluded = excludedTeams[`${competitionId}:${season}`];
let allowedTeamIds = null;

if (excluded) {
  const { createApiFootballAdapter } = await import(
    "../../web/dado-goal/providers/api-football.mjs"
  );
  const adapter = createApiFootballAdapter({ apiKey });
  const teams = await adapter.fetchTeams({ competitionId, season });
  allowedTeamIds = teams
    .filter((team) => !excluded.has(String(team.id)))
    .map((team) => team.id);

  if (allowedTeamIds.length !== 18 || teams.length !== 19) {
    throw new Error("unexpected competition membership; import blocked");
  }
}

const catalogue = await buildFootballCatalogue({
  apiKey,
  competitionId,
  season,
  allowedTeamIds,
});

await writeFile(
  outputFile,
  `${JSON.stringify(catalogue, null, 2)}\n`,
  { flag: "wx" },
);

console.log(
  `Imported ${catalogue.teams.length} teams into ${outputFile}`,
);
