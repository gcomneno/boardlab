CREATE TABLE lifecycle_match_teams (
    match_id TEXT NOT NULL
        REFERENCES lifecycle_matches(match_id) ON DELETE CASCADE,
    seat INTEGER NOT NULL CHECK(seat IN (0, 1)),
    provider TEXT NOT NULL,
    competition_id TEXT NOT NULL,
    season TEXT NOT NULL,
    provider_team_id TEXT NOT NULL,
    PRIMARY KEY(match_id, seat)
);
