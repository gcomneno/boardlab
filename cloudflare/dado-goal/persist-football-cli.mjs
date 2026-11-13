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
const SQL_CHUNK_SIZE = 16 * 1024;

const provider = sqlString(catalogue.provider);
const competitionId = sqlString(catalogue.competitionId);
const season = sqlString(catalogue.season);

const statements = [`
INSERT INTO football_catalogues (
  provider,
  competition_id,
  season,
  catalogue_json,
  imported_at,
  updated_at
)
VALUES (
  ${provider},
  ${competitionId},
  ${season},
  '',
  ${sqlString(catalogue.importedAt)},
  ${now}
)
ON CONFLICT(provider, competition_id, season)
DO UPDATE SET
  catalogue_json = '',
  imported_at = excluded.imported_at,
  updated_at = excluded.updated_at;
`.trim()];

for (let offset = 0; offset < catalogueJson.length; offset += SQL_CHUNK_SIZE) {
  const chunk = catalogueJson.slice(offset, offset + SQL_CHUNK_SIZE);
  statements.push(`
UPDATE football_catalogues
SET catalogue_json = catalogue_json || ${sqlString(chunk)}
WHERE provider = ${provider}
  AND competition_id = ${competitionId}
  AND season = ${season};
`.trim());
}

const sql = statements.join("\n");
process.stdout.write(`${sql}\n`);
