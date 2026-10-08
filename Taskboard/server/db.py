"""SQLite task store shared by the MCP server and the dispatcher."""
import os
import sqlite3
from contextlib import contextmanager

import history
from paths import DB_PATH

from statuses import RENAMED_STATUSES

SCHEMA = """
CREATE TABLE IF NOT EXISTS tasks (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    parent_id   INTEGER REFERENCES tasks(id) ON DELETE CASCADE,
    title       TEXT NOT NULL,
    description TEXT NOT NULL DEFAULT '',
    priority    INTEGER NOT NULL DEFAULT 3,
    status      TEXT NOT NULL DEFAULT 'backlog',
    agent_model TEXT NOT NULL DEFAULT 'claude:sonnet', -- see sessions.model_argument
    order_index INTEGER NOT NULL DEFAULT 0,
    session_id  TEXT,
    result      TEXT,
    created_at  TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at  TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS task_deps (
    task_id       INTEGER NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
    depends_on_id INTEGER NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
    PRIMARY KEY (task_id, depends_on_id)
);
CREATE TABLE IF NOT EXISTS board_palette (
    position INTEGER PRIMARY KEY,
    color    TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS task_field_history (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    task_id    INTEGER NOT NULL,
    field      TEXT NOT NULL,
    old_value  TEXT,
    new_value  TEXT,
    source     TEXT NOT NULL,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS board_state (
    key   TEXT PRIMARY KEY,
    value INTEGER NOT NULL
);
INSERT OR IGNORE INTO board_state(key, value) VALUES ('revision', 0);
CREATE TABLE IF NOT EXISTS task_events (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    task_id    INTEGER NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
    kind       TEXT NOT NULL,
    message    TEXT NOT NULL,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
"""

TREE_RANK_SQL = """
WITH RECURSIVE tree_rank(id, key) AS (
    SELECT id, printf('%010d.%010d', order_index, id) FROM tasks WHERE parent_id IS NULL
    UNION ALL
    SELECT t.id, r.key || '/' || printf('%010d.%010d', t.order_index, t.id)
    FROM tasks t JOIN tree_rank r ON t.parent_id = r.id
)
"""

# A task is ready to run when it is 'queued' and every dependency is 'done'.
READY_SQL = TREE_RANK_SQL + """
SELECT t.* FROM tasks t JOIN tree_rank r ON r.id = t.id
WHERE t.status = 'queued'
  AND NOT EXISTS (
      SELECT 1 FROM task_deps d JOIN tasks dep ON dep.id = d.depends_on_id
      WHERE d.task_id = t.id AND dep.status != 'done')
ORDER BY r.key
"""


def next_order_index(conn):
    """order_index is one board-wide rank: kanban columns, siblings and the dispatcher all sort by it."""
    return conn.execute("SELECT COALESCE(MAX(order_index) + 1, 0) FROM tasks").fetchone()[0]


def _rename_statuses(conn):
    for old, new in RENAMED_STATUSES.items():
        conn.execute("UPDATE tasks SET status=? WHERE status=?", (new, old))


def _make_order_global(conn):
    """Sibling-local order_index values become one board-wide rank, numbered in tree order once."""
    if conn.execute("SELECT 1 FROM board_state WHERE key = 'global_order'").fetchone():
        return
    conn.execute("BEGIN IMMEDIATE")
    try:
        ranked = conn.execute(TREE_RANK_SQL + "SELECT id FROM tree_rank ORDER BY key").fetchall()
        for rank, row in enumerate(ranked):
            conn.execute("UPDATE tasks SET order_index=? WHERE id=?", (rank, row["id"]))
        conn.execute("INSERT INTO board_state(key, value) VALUES ('global_order', 1)")
        conn.execute("COMMIT")
    except Exception:
        conn.execute("ROLLBACK")
        raise


@contextmanager
def connect():
    conn = sqlite3.connect(DB_PATH, timeout=30, isolation_level=None)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA foreign_keys = ON")
    conn.execute("PRAGMA journal_mode = WAL")
    conn.executescript(SCHEMA)
    cols = {r[1] for r in conn.execute("PRAGMA table_info(tasks)")}
    for col, typ in (("claude_session", "TEXT"), ("tmux_window", "TEXT"), ("x", "REAL"), ("y", "REAL"),
                     ("width", "REAL"), ("body_height", "REAL"), ("collapsed", "INTEGER NOT NULL DEFAULT 0"),
                     ("color", "TEXT"), ("effort", "TEXT"), ("permission_mode", "TEXT"),
                     ("harness", "TEXT NOT NULL DEFAULT 'claude-code'"),
                     ("session_state", "TEXT"), ("session_detail", "TEXT"),
                     ("session_updated_at", "TEXT"), ("session_launch", "TEXT")):
        if col not in cols:
            conn.execute(f"ALTER TABLE tasks ADD COLUMN {col} {typ}")
    for table in ("tasks", "task_deps", "board_palette"):
        for action in ("INSERT", "UPDATE", "DELETE"):
            conn.execute(
                f"""CREATE TRIGGER IF NOT EXISTS bump_revision_{table}_{action.lower()} AFTER {action} ON {table}
                    BEGIN UPDATE board_state SET value = value + 1 WHERE key = 'revision'; END""")
    history.install(conn)
    _rename_statuses(conn)
    _make_order_global(conn)
    try:
        yield conn
    finally:
        conn.close()


def board_revision(conn):
    """Monotonic counter bumped by triggers on every change to tasks, dependencies or the palette."""
    return str(conn.execute("SELECT value FROM board_state WHERE key = 'revision'").fetchone()[0])


def record_field_changes(conn, task_id, changes, source):
    """Keep every replaced value so no edit is ever unrecoverable."""
    row = conn.execute("SELECT * FROM tasks WHERE id=?", (task_id,)).fetchone()
    for field, new_value in changes.items():
        old_value = row[field]
        if old_value == new_value:
            continue
        conn.execute(
            "INSERT INTO task_field_history(task_id, field, old_value, new_value, source) VALUES (?, ?, ?, ?, ?)",
            (task_id, field, None if old_value is None else str(old_value),
             None if new_value is None else str(new_value), source))


def log_event(conn, task_id, kind, message):
    conn.execute("INSERT INTO task_events(task_id, kind, message) VALUES (?, ?, ?)", (task_id, kind, message))


def would_create_cycle(conn, task_id, depends_on_id):
    """True if depends_on_id already (transitively) depends on task_id."""
    row = conn.execute(
        """WITH RECURSIVE reach(id) AS (
               SELECT ? UNION SELECT d.depends_on_id FROM task_deps d JOIN reach r ON d.task_id = r.id)
           SELECT 1 FROM reach WHERE id = ?""",
        (depends_on_id, task_id),
    ).fetchone()
    return row is not None


def claim_next(conn, session_id):
    """Atomically move the best runnable queued task to 'wip'. Returns the row or None."""
    conn.execute("BEGIN IMMEDIATE")
    try:
        row = conn.execute(READY_SQL + " LIMIT 1").fetchone()
        if row is None:
            conn.execute("COMMIT")
            return None
        conn.execute(
            "UPDATE tasks SET status='wip', session_id=?, updated_at=CURRENT_TIMESTAMP WHERE id=?",
            (session_id, row["id"]),
        )
        log_event(conn, row["id"], "status", f"wip (session {session_id})")
        conn.execute("COMMIT")
        return conn.execute("SELECT * FROM tasks WHERE id=?", (row["id"],)).fetchone()
    except Exception:
        conn.execute("ROLLBACK")
        raise
