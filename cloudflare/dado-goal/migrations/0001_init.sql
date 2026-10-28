CREATE TABLE lifecycle_matches (
    match_id TEXT PRIMARY KEY,
    revision INTEGER NOT NULL CHECK(revision >= 0),
    status TEXT NOT NULL
        CHECK(status IN ('waiting', 'active', 'completed')),
    state_json TEXT NOT NULL,
    invite_digest BLOB,
    invite_used INTEGER NOT NULL DEFAULT 0
        CHECK(invite_used IN (0, 1)),
    created_at INTEGER NOT NULL CHECK(created_at >= 0),
    updated_at INTEGER NOT NULL CHECK(updated_at >= created_at),
    expires_at INTEGER NOT NULL CHECK(expires_at > updated_at),
    completed_at INTEGER,
    CHECK(
        (status = 'completed'
         AND completed_at IS NOT NULL
         AND completed_at >= created_at
         AND completed_at <= updated_at)
        OR
        (status != 'completed' AND completed_at IS NULL)
    )
);

CREATE TABLE lifecycle_seats (
    match_id TEXT NOT NULL
        REFERENCES lifecycle_matches(match_id) ON DELETE CASCADE,
    seat INTEGER NOT NULL CHECK(seat IN (0, 1)),
    token_digest BLOB NOT NULL,
    PRIMARY KEY(match_id, seat),
    UNIQUE(token_digest)
);

CREATE TABLE lifecycle_events (
    match_id TEXT NOT NULL
        REFERENCES lifecycle_matches(match_id) ON DELETE CASCADE,
    revision INTEGER NOT NULL,
    event_json TEXT NOT NULL,
    PRIMARY KEY(match_id, revision)
);

CREATE TABLE lifecycle_receipts (
    match_id TEXT NOT NULL
        REFERENCES lifecycle_matches(match_id) ON DELETE CASCADE,
    seat INTEGER NOT NULL CHECK(seat IN (0, 1)),
    request_id TEXT NOT NULL,
    expected_revision INTEGER NOT NULL,
    response_json TEXT NOT NULL,
    PRIMARY KEY(match_id, seat, request_id)
);

CREATE TABLE IF NOT EXISTS dado_browser_sessions (
  session_digest TEXT PRIMARY KEY,
  csrf_digest TEXT NOT NULL,
  expires_at INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS dado_login_limits (
  client_digest TEXT PRIMARY KEY,
  window_start INTEGER NOT NULL,
  attempts INTEGER NOT NULL CHECK (attempts >= 1)
);
