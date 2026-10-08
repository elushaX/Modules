"""Dispatcher bookkeeping for live statuses: wip and daemon tasks whose session ended are blocked, and only wip
tasks take a worker slot (run: uv run python test_dispatcher.py)."""
import importlib
import os
import sys
import tempfile


def open_dispatcher(directory):
    os.environ["TASKS_DB"] = os.path.join(directory, "tasks.db")
    os.environ["AGENTS_SESSIONS_DIR"] = os.path.join(directory, "sessions")
    for module in ("paths", "statuses", "history", "db", "sessions", "dispatcher"):
        sys.modules.pop(module, None)
    dispatcher = importlib.import_module("dispatcher")
    dispatcher.sessions.reconcile = lambda: None
    return dispatcher, importlib.import_module("db")


def add_task(db, title, status, session_state):
    with db.connect() as conn:
        cursor = conn.execute(
            "INSERT INTO tasks(title, status, session_state) VALUES (?, ?, ?)", (title, status, session_state))
        return cursor.lastrowid


def status_of(db, task_id):
    with db.connect() as conn:
        return conn.execute("SELECT status FROM tasks WHERE id=?", (task_id,)).fetchone()["status"]


def main():
    with tempfile.TemporaryDirectory() as directory:
        dispatcher, db = open_dispatcher(directory)
        running_daemon = add_task(db, "Watcher", "daemon", "working")
        ended_daemon = add_task(db, "Dead watcher", "daemon", "ended")
        ended_wip = add_task(db, "Dead job", "wip", "ended")
        ended_review = add_task(db, "Reviewed job", "review", "ended")
        add_task(db, "Job", "wip", "working")

        assert dispatcher.running_count() == 2, "a daemon must not take a worker slot"

        dispatcher.reap()

        assert status_of(db, running_daemon) == "daemon"
        assert status_of(db, ended_daemon) == "blocked"
        assert status_of(db, ended_wip) == "blocked"
        assert status_of(db, ended_review) == "review"
        assert dispatcher.running_count() == 1

    print("dispatcher: all checks passed")


main()
