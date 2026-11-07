
export async function getLatestFootballCatalogue(db, provider) {
  const row = await db.prepare(`
    SELECT catalogue_json
    FROM football_catalogues
    WHERE provider = ?
    ORDER BY updated_at DESC
    LIMIT 1
  `).bind(String(provider)).first();

  if (!row) {
    return null;
  }

  return JSON.parse(row.catalogue_json);
}

export async function getFootballCatalogue(
  db,
  { provider, competitionId, season },
) {
  const row = await db.prepare(`
    SELECT catalogue_json
    FROM football_catalogues
    WHERE provider = ?
      AND competition_id = ?
      AND season = ?
  `).bind(
    String(provider),
    String(competitionId),
    String(season),
  ).first();

  if (!row) {
    return null;
  }

  return JSON.parse(row.catalogue_json);
}

export async function putFootballCatalogue(db, catalogue, now = Date.now()) {
  const catalogueJson = JSON.stringify(catalogue);

  await db.prepare(`
    INSERT INTO football_catalogues (
      provider,
      competition_id,
      season,
      catalogue_json,
      imported_at,
      updated_at
    )
    VALUES (?, ?, ?, ?, ?, ?)
    ON CONFLICT(provider, competition_id, season)
    DO UPDATE SET
      catalogue_json = excluded.catalogue_json,
      imported_at = excluded.imported_at,
      updated_at = excluded.updated_at
  `).bind(
    String(catalogue.provider),
    String(catalogue.competitionId),
    String(catalogue.season),
    catalogueJson,
    String(catalogue.importedAt),
    now,
  ).run();

  return catalogue;
}
