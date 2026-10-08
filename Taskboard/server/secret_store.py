"""Secret store: key/value credentials agents fetch at run time instead of embedding in prompts or skills.

The values live in their own SQLite file (secrets.db) in the data directory, kept apart from tasks.db on
purpose: secrets are never part of the board, its history, or any snapshot, so viewing or exporting a board
can never leak them. A skill or task description names the keys an agent needs; the agent calls get_secret
to read each value only when it runs. list_secrets returns names and descriptions but never values, so the
board UI and reviewers can manage access without seeing the secrets themselves.
"""
import os
import sqlite3
from contextlib import contextmanager

from paths import SECRETS_DB

SCHEMA = """
CREATE TABLE IF NOT EXISTS secrets (
    key         TEXT PRIMARY KEY,
    value       TEXT NOT NULL,
    description TEXT NOT NULL DEFAULT '',
    created_at  TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at  TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
"""


def _is_key(key):
    return bool(key) and all(ch.isalnum() or ch in "._-" for ch in key)


@contextmanager
def connect():
    os.makedirs(os.path.dirname(SECRETS_DB), exist_ok=True)
    conn = sqlite3.connect(SECRETS_DB, timeout=30, isolation_level=None)
    conn.row_factory = sqlite3.Row
    conn.executescript(SCHEMA)
    try:
        os.chmod(SECRETS_DB, 0o600)
    except OSError:
        pass
    try:
        yield conn
    finally:
        conn.close()


def list_secrets():
    """Every stored key with its description and timestamps - never the value."""
    with connect() as conn:
        return [
            {"key": row["key"], "description": row["description"],
             "created_at": row["created_at"], "updated_at": row["updated_at"]}
            for row in conn.execute(
                "SELECT key, description, created_at, updated_at FROM secrets ORDER BY key")
        ]


def get_secret(key):
    """The value for one key. Agents call this at run time to retrieve a credential."""
    if not _is_key(key):
        raise ValueError(f"invalid secret key {key!r}")
    with connect() as conn:
        row = conn.execute("SELECT value FROM secrets WHERE key=?", (key,)).fetchone()
    if row is None:
        raise ValueError(f"secret {key!r} not found")
    return {"key": key, "value": row["value"]}


def set_secret(key, value, description=None):
    """Create or replace a secret. An existing description is kept when description is omitted."""
    if not _is_key(key):
        raise ValueError(f"invalid secret key {key!r} (use letters, digits, '.', '_', '-')")
    with connect() as conn:
        existing = conn.execute("SELECT description FROM secrets WHERE key=?", (key,)).fetchone()
        kept = existing["description"] if existing else ""
        conn.execute(
            """INSERT INTO secrets(key, value, description) VALUES (?, ?, ?)
               ON CONFLICT(key) DO UPDATE SET value=excluded.value, description=excluded.description,
                                              updated_at=CURRENT_TIMESTAMP""",
            (key, value, kept if description is None else description))
    return {"key": key, "description": kept if description is None else description}


def delete_secret(key):
    """Remove a secret. Returns whether a row existed."""
    if not _is_key(key):
        raise ValueError(f"invalid secret key {key!r}")
    with connect() as conn:
        cur = conn.execute("DELETE FROM secrets WHERE key=?", (key,))
    return {"key": key, "deleted": cur.rowcount > 0}
