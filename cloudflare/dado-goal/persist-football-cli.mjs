import { readFile } from "node:fs/promises";

const [inputFile] = process.argv.slice(2);

if (!inputFile) {
  throw new Error("usage: node persist-football-cli.mjs CATALOGUE.json");
}

const catalogue = JSON.parse(await readFile(inputFile, "utf8"));

for (const field of ["provider", "competitionId", "season", "importedAt"]) {
  if (catalogue[field] === undefined || catalogue[field] === null) {
    throw new Error(`catalogue missing ${field}`);
  }
}

if (!Array.isArray(catalogue.teams)) {
  throw new Error("catalogue teams must be an array");
}

const sqlString = (value) =>
  `'${String(value).replaceAll("'", "''")}'`;

const now = Date.now();
const catalogueJson = JSON.stringify(catalogue);

const sql = `
INSERT INTO football_catalogues (
  provider,
  competition_id,
  season,
  catalogue_json,
  imported_at,
  updated_at
)
VALUES (
  ${sqlString(catalogue.provider)},
  ${sqlString(catalogue.competitionId)},
  ${sqlString(catalogue.season)},
  ${sqlString(catalogueJson)},
  ${sqlString(catalogue.importedAt)},
  ${now}
)
ON CONFLICT(provider, competition_id, season)
DO UPDATE SET
  catalogue_json = excluded.catalogue_json,
  imported_at = excluded.imported_at,
  updated_at = excluded.updated_at;
`.trim();

process.stdout.write(`${sql}\n`);
