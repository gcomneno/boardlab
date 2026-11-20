import sqlite3
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent
INIT = ROOT / "migrations" / "0001_init.sql"
MIGRATION = ROOT / "migrations" / "0004_rules_version.sql"
IMMUTABILITY = ROOT / "migrations" / "0005_rules_version_immutable.sql"


def insert_match(db, match_id, rules_version=None):
    columns = """
        match_id, revision, status, state_json,
        created_at, updated_at, expires_at
    """
    values = [match_id, 0, "waiting", '{"schemaVersion":1}',
              100, 100, 200]

    if rules_version is not None:
        columns += ", rules_version"
        values.append(rules_version)

    placeholders = ", ".join("?" for _ in values)
    db.execute(
        f"INSERT INTO lifecycle_matches ({columns}) "
        f"VALUES ({placeholders})",
        values,
    )


class RulesVersionMigrationTests(unittest.TestCase):
    def setUp(self):
        self.db = sqlite3.connect(":memory:")
        self.db.execute("PRAGMA foreign_keys = ON")
        self.db.executescript(INIT.read_text(encoding="utf-8"))
        insert_match(self.db, "historical")
        self.db.executescript(MIGRATION.read_text(encoding="utf-8"))

    def tearDown(self):
        self.db.close()

    def version(self, match_id):
        row = self.db.execute(
            "SELECT rules_version FROM lifecycle_matches WHERE match_id = ?",
            (match_id,),
        ).fetchone()
        return row[0]

    def test_historical_match_remains_v1(self):
        self.assertEqual(self.version("historical"), 1)

    def test_default_new_match_remains_v1(self):
        insert_match(self.db, "new-default")
        self.assertEqual(self.version("new-default"), 1)

    def test_explicit_v2_match_is_persisted(self):
        insert_match(self.db, "new-v2", 2)
        self.assertEqual(self.version("new-v2"), 2)

    def test_invalid_versions_are_rejected(self):
        for invalid in (0, 3, -1):
            with self.subTest(version=invalid):
                with self.assertRaises(sqlite3.IntegrityError):
                    insert_match(self.db, f"bad-{invalid}", invalid)

    def test_null_version_is_rejected(self):
        with self.assertRaises(sqlite3.IntegrityError):
            self.db.execute(
                "UPDATE lifecycle_matches SET rules_version = NULL "
                "WHERE match_id = 'historical'"
            )

    def test_historical_state_is_not_rewritten(self):
        row = self.db.execute("""
            SELECT revision, status, state_json
            FROM lifecycle_matches WHERE match_id = 'historical'
        """).fetchone()
        self.assertEqual(
            row, (0, "waiting", '{"schemaVersion":1}')
        )

    def install_immutability(self):
        self.db.executescript(IMMUTABILITY.read_text(encoding="utf-8"))
        insert_match(self.db, "new-v2", 2)

    def revision_and_version(self, match_id):
        return self.db.execute(
            "SELECT revision, rules_version FROM lifecycle_matches "
            "WHERE match_id = ?",
            (match_id,),
        ).fetchone()

    def test_ordinary_updates_preserve_versions(self):
        self.install_immutability()
        self.db.execute(
            "UPDATE lifecycle_matches SET revision = 1, updated_at = 110 "
            "WHERE match_id IN ('historical', 'new-v2')"
        )
        self.assertEqual(self.revision_and_version("historical"), (1, 1))
        self.assertEqual(self.revision_and_version("new-v2"), (1, 2))

    def test_version_changes_are_rejected(self):
        self.install_immutability()
        for match_id, current, requested in (("historical", 1, 2), ("new-v2", 2, 1)):
            with self.subTest(match=match_id):
                with self.assertRaisesRegex(
                    sqlite3.IntegrityError, "^Match rules version is immutable$"
                ):
                    self.db.execute(
                        "UPDATE lifecycle_matches SET rules_version = ?, revision = 1 "
                        "WHERE match_id = ?",
                        (requested, match_id),
                    )
                self.assertEqual(self.revision_and_version(match_id), (0, current))

    def test_same_version_updates_are_allowed(self):
        self.install_immutability()
        for match_id, version in (("historical", 1), ("new-v2", 2)):
            with self.subTest(match=match_id):
                self.db.execute(
                    "UPDATE lifecycle_matches SET rules_version = ?, revision = 1 "
                    "WHERE match_id = ?",
                    (version, match_id),
                )
                self.assertEqual(self.revision_and_version(match_id), (1, version))


if __name__ == "__main__":
    unittest.main(verbosity=2)
