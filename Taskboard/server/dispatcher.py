"""Start a headless Claude Code session for every ready task, up to --workers at a time.

Sessions are owned by sessions.py (one detached tmux session per task under sessions/task-<id>).
Attach from the taskboard ("Open session") or with: tmux attach -t task-<id>
"""
import argparse
import time
import uuid

import sessions
from db import claim_next, connect, log_event
from statuses import LIVE_STATUSES


def reap():
    """Tasks with a live status whose session ended without completing are blocked."""
    sessions.reconcile()
    live = ", ".join("?" for _ in LIVE_STATUSES)
    with connect() as conn:
        for t in conn.execute(
                f"SELECT id FROM tasks WHERE status IN ({live}) AND session_state='ended'", LIVE_STATUSES).fetchall():
            conn.execute("UPDATE tasks SET status='blocked' WHERE id=?", (t["id"],))
            log_event(conn, t["id"], "status", "blocked: session ended without completing")


def running_count():
    with connect() as conn:
        return conn.execute("SELECT count(*) FROM tasks WHERE status='wip'").fetchone()[0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--workers", type=int, default=3, help="max concurrently running tasks")
    ap.add_argument("--once", action="store_true", help="exit when nothing is queued or wip")
    args = ap.parse_args()
    while True:
        reap()
        while running_count() < args.workers:
            with connect() as conn:
                task = claim_next(conn, f"disp-{uuid.uuid4().hex[:8]}")
            if task is None:
                break
            try:
                sessions.start(task["id"])
            except Exception as e:
                with connect() as conn:
                    conn.execute("UPDATE tasks SET status='blocked' WHERE id=?", (task["id"],))
                    log_event(conn, task["id"], "status", f"blocked: failed to launch: {e}")
        if args.once and running_count() == 0:
            with connect() as conn:
                if not conn.execute("SELECT 1 FROM tasks WHERE status='queued'").fetchone():
                    break
        time.sleep(2)


if __name__ == "__main__":
    main()
