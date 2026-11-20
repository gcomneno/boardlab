-- M034-V34: persist authoritative Dado Goal rules version.
-- Existing matches and default inserts remain V1.
-- V2 activation requires separate server authorization.

ALTER TABLE lifecycle_matches
ADD COLUMN rules_version INTEGER NOT NULL DEFAULT 1
CHECK (rules_version IN (1, 2));
