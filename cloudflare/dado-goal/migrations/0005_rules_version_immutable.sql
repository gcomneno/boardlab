CREATE TRIGGER lifecycle_rules_version_immutable
BEFORE UPDATE OF rules_version ON lifecycle_matches
FOR EACH ROW
WHEN NEW.rules_version IS NOT OLD.rules_version
BEGIN
    SELECT RAISE(ABORT, 'Match rules version is immutable');
END;
