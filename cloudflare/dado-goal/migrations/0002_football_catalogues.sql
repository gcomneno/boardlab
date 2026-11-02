CREATE TABLE football_catalogues (
    provider TEXT NOT NULL,
    competition_id TEXT NOT NULL,
    season TEXT NOT NULL,
    catalogue_json TEXT NOT NULL,
    imported_at TEXT NOT NULL,
    updated_at INTEGER NOT NULL CHECK(updated_at >= 0),
    PRIMARY KEY(provider, competition_id, season)
);
