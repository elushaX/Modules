"""Terminal board: task tree + details. Enter opens the task's Claude Code session.

Keys: ↑/↓ or j/k move · Enter open session · r mark ready · d approve (done)
      R restart/resume session · s supervisor window · q quit
Run inside tmux (start.sh does this) so Enter can switch windows; outside tmux it attaches.
"""
import curses
import os
import subprocess
import textwrap
import time

import sessions
from db import connect, log_event

COLORS = {"backlog": 8, "queued": 6, "wip": 3, "daemon": 4, "review": 5, "done": 2, "blocked": 1}

_subrouter_cache: dict = {"data": None, "ts": 0.0}


def subrouter_line() -> str:
    """Cached one-liner for the dock bar; refreshes every 30 s to avoid slowing the 2 s redraw loop."""
    now = time.time()
    if now - _subrouter_cache["ts"] > 30:
        _subrouter_cache["data"] = sessions.subrouter_status()
        _subrouter_cache["ts"] = now
    s = _subrouter_cache["data"]
    if not s or not s.get("available"):
        return " subrouter: unavailable"
    accounts = s.get("accounts", {})
    parts = []
    for provider, pool in accounts.items():
        for a in pool.get("accounts", []):
            tag = "cooling" if a.get("coolingDownUntil") else "ok"
            parts.append(f"{a['email']} [{tag}]")
    if not parts:
        return " subrouter: no accounts — run: subrouter login anthropic"
    return " subrouter  " + "  ·  ".join(parts)


def session_auth(evs: list) -> str | None:
    """Extract auth method from the most recent session start/resume event."""
    for e in reversed(evs):
        if e["kind"] == "session" and "(" in e["message"] and e["message"].endswith(")"):
            return e["message"][e["message"].rfind("(") + 1:-1][:50]
    return None


def load():
    with connect() as conn:
        rows = [dict(r) for r in conn.execute("SELECT * FROM tasks ORDER BY order_index, id")]
        deps = {}
        for r in conn.execute("SELECT task_id, depends_on_id FROM task_deps"):
            deps.setdefault(r[0], []).append(r[1])
    kids = {}
    for r in rows:
        kids.setdefault(r["parent_id"], []).append(r)
    flat = []

    def walk(pid, depth):
        for r in kids.get(pid, []):
            r["depth"], r["deps"] = depth, deps.get(r["id"], [])
            flat.append(r)
            walk(r["id"], depth + 1)
    walk(None, 0)
    return flat


def events(tid, n=8):
    with connect() as conn:
        return [dict(r) for r in conn.execute(
            "SELECT kind, message, created_at FROM task_events WHERE task_id=? ORDER BY id DESC LIMIT ?", (tid, n))][::-1]


def open_window(task_id):
    target = sessions.session_name(task_id)
    if os.environ.get("TMUX"):
        subprocess.run(sessions.tmux_command("switch-client", "-t", target))
        return True
    curses.endwin()
    subprocess.run(sessions.tmux_command("attach", "-t", target))
    return True


def main(scr):
    curses.curs_set(0); curses.use_default_colors(); scr.timeout(2000)
    for status, c in COLORS.items():
        curses.init_pair(c, c if c != 8 else 7, -1)
    sel, msg = 0, ""
    while True:
        tasks = load()
        sel = max(0, min(sel, len(tasks) - 1))
        h, w = scr.getmaxyx(); left = w // 2
        content_rows = h - 4  # header row 0, blank row 1, dock row h-2, msg row h-1
        scr.erase()
        scr.addnstr(0, 0, " TASK BOARD  ⏎ open  r queue  d approve  R resume  s supervisor  q quit ", w - 1, curses.A_REVERSE)
        for i, t in enumerate(tasks[:content_rows]):
            line = f"{'  ' * t['depth']}#{t['id']} {t['title']}"
            attr = curses.A_REVERSE if i == sel else 0
            scr.addnstr(i + 2, 0, line.ljust(left - 12), left - 12, attr)
            scr.addnstr(i + 2, left - 11, f"{t['status']:<9}", 9, curses.color_pair(COLORS.get(t["status"], 7)) | attr)
        if tasks:
            t = tasks[sel]
            evs = events(t["id"])
            auth = session_auth(evs)
            auth_line = f"auth: {auth}" if auth else "auth: —"
            info = [f"#{t['id']} {t['title']}", f"status: {t['status']}   order: {t['order_index']}",
                    f"model: {t['agent_model']}   {auth_line}", f"depends on: {t['deps'] or '-'}",
                    f"session: {t['claude_session'] or '-'}", f"tmux: {t['tmux_window'] or '-'}", "",
                    *textwrap.wrap(t["description"] or "(no description)", w - left - 2), "",
                    "result:", *textwrap.wrap(t["result"] or "-", w - left - 2), "", "log:"]
            info += [f"{e['created_at'][11:16]} {e['kind']}: {e['message']}" for e in evs]
            for j, line in enumerate(info[:content_rows]):
                scr.addnstr(j + 2, left, line, w - left - 1, curses.A_BOLD if j == 0 else 0)
        # subrouter dock bar
        dock = subrouter_line()
        scr.addnstr(h - 2, 0, dock.ljust(w - 1), w - 1, curses.A_REVERSE)
        scr.addnstr(h - 1, 0, msg, w - 1); msg = ""
        scr.refresh()

        k = scr.getch()
        if k in (ord("q"), 27):
            return
        if k in (curses.KEY_DOWN, ord("j")): sel += 1
        elif k in (curses.KEY_UP, ord("k")): sel -= 1
        elif not tasks: continue
        elif k in (10, 13, curses.KEY_ENTER):
            t = tasks[sel]
            if sessions.is_alive(t["id"]):
                open_window(t["id"])
            elif t["claude_session"]:
                sessions.start(t["id"]); open_window(t["id"])
            else:
                msg = "no session yet - press r to queue it"
        elif k == ord("R") and tasks[sel]["claude_session"]:
            sessions.start(tasks[sel]["id"]); msg = "session resumed"
        elif k in (ord("r"), ord("d")):
            st = "queued" if k == ord("r") else "done"
            with connect() as conn:
                conn.execute("UPDATE tasks SET status=? WHERE id=?", (st, tasks[sel]["id"]))
                log_event(conn, tasks[sel]["id"], "status", f"{st} (by human)")
        elif k == ord("s"):
            msg = "the supervisor runs in its own tmux session: tmux attach -t agents"


if __name__ == "__main__":
    curses.wrapper(main)
