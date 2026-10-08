"""Legacy databases: sibling-local order_index becomes one tree-ordered rank, renamed statuses get their new names
(run: uv run python test_migrations.py)."""
import importlib
import json
import os
import sqlite3
import sys
import tempfile

LEGACY_ROWS = [
    (1, None, "Root B", 1, "running"),
    (2, None, "Root A", 0, "backlog"),
    (3, 2, "A child 2", 1, "ready"),
    (4, 2, "A child 1", 0, "running"),
    (5, 1, "B child", 0, "done"),
    (6, 1, "B child 2", 1, "failed"),
]


def seed_legacy_database(path):
    conn = sqlite3.connect(path)
    conn.execute("""CREATE TABLE tasks (id INTEGER PRIMARY KEY AUTOINCREMENT,
        parent_id INTEGER REFERENCES tasks(id) ON DELETE CASCADE, title TEXT NOT NULL,
        description TEXT NOT NULL DEFAULT '', priority INTEGER NOT NULL DEFAULT 3,
        status TEXT NOT NULL DEFAULT 'backlog', agent_model TEXT NOT NULL DEFAULT 'claude:sonnet',
        order_index INTEGER NOT NULL DEFAULT 0, session_id TEXT, result TEXT,
        created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)""")
    conn.executemany("INSERT INTO tasks(id, parent_id, title, order_index, status) VALUES (?, ?, ?, ?, ?)", LEGACY_ROWS)
    conn.commit()
    conn.close()


def open_database(path):
    os.environ["TASKS_DB"] = path
    for module in ("paths", "statuses", "history", "db"):
        sys.modules.pop(module, None)
    return importlib.import_module("db")


def ranks(db):
    with db.connect() as conn:
        return {row["title"]: (row["order_index"], row["status"])
                for row in conn.execute("SELECT title, order_index, status FROM tasks")}


def main():
    with tempfile.TemporaryDirectory() as directory:
        path = os.path.join(directory, "tasks.db")
        seed_legacy_database(path)
        db = open_database(path)

        assert ranks(db) == {
            "Root A": (0, "backlog"), "A child 1": (1, "wip"), "A child 2": (2, "queued"),
            "Root B": (3, "wip"), "B child": (4, "done"), "B child 2": (5, "blocked"),
        }, ranks(db)

        with db.connect() as conn:
            conn.execute("UPDATE tasks SET order_index = 10 WHERE title = 'Root A'")
            conn.execute("UPDATE tasks SET status = 'running' WHERE title = 'B child'")
            assert db.next_order_index(conn) == 11
        after = ranks(db)
        assert after["Root A"] == (10, "backlog"), "the one-time renumbering must not run again"
        assert after["B child"] == (4, "wip"), "rows written by an old client are renamed on the next connect"

        import history
        with db.connect() as conn:
            snapshots = [json.loads(row[0]) for row in conn.execute("SELECT snapshot FROM board_history WHERE snapshot IS NOT NULL")]
            assert any(snapshot["status"] == "running" for snapshot in snapshots), "history itself is never rewritten"
            past = history.tasks_at(conn, 1e12)
            assert not {"running", "ready", "failed"} & {task["status"] for task in past.values()}

    print("migrations: all checks passed")


main()
