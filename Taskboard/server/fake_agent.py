"""Test agent: completes its task directly via the DB, no LLM needed."""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from db import connect, log_event  # noqa: E402

tid = int(os.environ["TASK_ID"])
with open(os.path.join("reports", "summary.md"), "w") as report:
    report.write(f"# Task {tid}\n\nFake agent result.\n")
time.sleep(float(os.environ.get("FAKE_DELAY", "1")))
with connect() as conn:
    title = conn.execute("SELECT title FROM tasks WHERE id=?", (tid,)).fetchone()["title"]
    conn.execute("UPDATE tasks SET status='done', result=? WHERE id=?", (f"fake result for '{title}'", tid))
    log_event(conn, tid, "status", "done (fake agent)")
print(f"fake agent finished task {tid}")
