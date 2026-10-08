"""MCP server exposing the task database to you (manager tools) and to agents (agent tools)."""
import json

from mcp.server.fastmcp import FastMCP

import history
import secret_store
import sessions
import skills
from statuses import STATUSES, current_status
from db import READY_SQL, board_revision, next_order_index, connect, log_event, record_field_changes, would_create_cycle

mcp = FastMCP("task-manager")


def _task(conn, task_id):
    row = conn.execute("SELECT * FROM tasks WHERE id=?", (task_id,)).fetchone()
    if row is None:
        raise ValueError(f"task {task_id} not found")
    return row


def _full(conn, task_id):
    t = dict(_task(conn, task_id))
    t["children"] = [r["id"] for r in conn.execute("SELECT id FROM tasks WHERE parent_id=? ORDER BY order_index, id", (task_id,))]
    t["depends_on"] = [r["depends_on_id"] for r in conn.execute("SELECT depends_on_id FROM task_deps WHERE task_id=?", (task_id,))]
    t["workspace"] = sessions.session_directory(task_id)
    t["events"] = [dict(r) for r in conn.execute(
        "SELECT kind, message, created_at FROM task_events WHERE task_id=? ORDER BY id", (task_id,))]
    return t


def _checked_status(status):
    status = current_status(status)
    if status not in STATUSES:
        raise ValueError(f"status must be one of {STATUSES}")
    return status


def _set_status(conn, task_id, status, note=None):
    status = _checked_status(status)
    _task(conn, task_id)
    record_field_changes(conn, task_id, {"status": status}, "set_status")
    conn.execute("UPDATE tasks SET status=?, updated_at=CURRENT_TIMESTAMP WHERE id=?", (status, task_id))
    log_event(conn, task_id, "status", status + (f": {note}" if note else ""))


# ---------- manager tools (you) ----------

@mcp.tool()
def create_task(title: str, description: str = "", parent_id: int | None = None,
                agent_model: str = "claude-opus-5-5", status: str = "backlog", order_index: int | None = None,
                effort: str | None = None, permission_mode: str | None = None,
                harness: str = "claude-code") -> dict:
    """Create a task, last in the board-wide order unless order_index is given. order_index is one global rank:
    kanban columns, siblings and the dispatcher all sort by it, so the order is the priority. agent_model: a model id from get_options (or 'echo' for tests).
    harness: claude-code or dsh (DeepSeek Harness) - see get_options; claude-code runs Claude models only.
    effort: low..max; permission_mode: auto (no prompts) or manual (asks before reads/edits).
    status: backlog (not runnable yet) or queued (dispatcher may pick it up)."""
    status = _checked_status(status)
    sessions.model_argument(agent_model)
    sessions.validate_model_harness(agent_model, harness)
    if effort not in (None, *sessions.EFFORTS):
        raise ValueError(f"effort must be one of {sessions.EFFORTS}")
    if permission_mode not in (None, *sessions.PERMISSION_MODES):
        raise ValueError(f"permission_mode must be one of {tuple(sessions.PERMISSION_MODES)}")
    with connect() as conn:
        if parent_id is not None:
            _task(conn, parent_id)
        if order_index is None:
            order_index = next_order_index(conn)
        cur = conn.execute(
            "INSERT INTO tasks(title, description, parent_id, agent_model, status, order_index, effort, "
            "permission_mode, harness) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (title, description, parent_id, agent_model, status, order_index, effort, permission_mode, harness))
        log_event(conn, cur.lastrowid, "created", title)
        return _full(conn, cur.lastrowid)


@mcp.tool()
def update_task(task_id: int, title: str | None = None, description: str | None = None,
                agent_model: str | None = None,
                order_index: int | None = None, parent_id: int | None = None) -> dict:
    """Edit task fields. Only passed fields change. order_index is the task's board-wide rank (lower = earlier)."""
    fields = {k: v for k, v in dict(title=title, description=description,
                                    agent_model=agent_model, order_index=order_index,
                                    parent_id=parent_id).items() if v is not None}
    with connect() as conn:
        _task(conn, task_id)
        if fields:
            record_field_changes(conn, task_id, fields, "update_task")
            sets = ", ".join(f"{k}=?" for k in fields)
            conn.execute(f"UPDATE tasks SET {sets}, updated_at=CURRENT_TIMESTAMP WHERE id=?", (*fields.values(), task_id))
            log_event(conn, task_id, "edit", ", ".join(f"{k}={v}" for k, v in fields.items()))
        return _full(conn, task_id)


@mcp.tool()
def set_status(task_id: int, status: str, note: str | None = None) -> dict:
    """Set status: backlog, queued, wip, daemon, review, done, blocked. 'queued' lets the dispatcher pick it up
    once its dependencies are done; 'daemon' marks a long-running background session the dispatcher never starts
    and never counts against its worker slots; 'done' approves review."""
    with connect() as conn:
        _set_status(conn, task_id, status, note)
        return _full(conn, task_id)


@mcp.tool()
def add_dependency(task_id: int, depends_on_id: int) -> dict:
    """task_id cannot start until depends_on_id is done. Cycles are rejected."""
    with connect() as conn:
        _task(conn, task_id); _task(conn, depends_on_id)
        if task_id == depends_on_id or would_create_cycle(conn, task_id, depends_on_id):
            raise ValueError("dependency would create a cycle")
        conn.execute("INSERT OR IGNORE INTO task_deps VALUES (?, ?)", (task_id, depends_on_id))
        log_event(conn, task_id, "dep", f"depends on {depends_on_id}")
        return _full(conn, task_id)


@mcp.tool()
def remove_dependency(task_id: int, depends_on_id: int) -> dict:
    """Remove a dependency edge."""
    with connect() as conn:
        conn.execute("DELETE FROM task_deps WHERE task_id=? AND depends_on_id=?", (task_id, depends_on_id))
        log_event(conn, task_id, "dep", f"no longer depends on {depends_on_id}")
        return _full(conn, task_id)


def _subtree_ids(conn, task_id: int) -> list[int]:
    rows = conn.execute(
        "WITH RECURSIVE subtree(id) AS (SELECT ? UNION ALL SELECT t.id FROM tasks t JOIN subtree s ON t.parent_id = s.id) "
        "SELECT id FROM subtree",
        (task_id,),
    ).fetchall()
    return [row[0] for row in rows]


def _stop_sessions(task_ids: list[int]) -> None:
    for task_id in task_ids:
        sessions.stop_for_deleted_task(task_id)


@mcp.tool()
def delete_task(task_id: int) -> str:
    """Delete a task and all its children, stopping their agent sessions."""
    with connect() as conn:
        _task(conn, task_id)
        doomed = _subtree_ids(conn, task_id)
        conn.execute("DELETE FROM tasks WHERE id=?", (task_id,))
    _stop_sessions(doomed)
    return f"deleted {task_id}"


@mcp.tool()
def list_tasks(status: str | None = None, parent_id: int | None = None) -> list[dict]:
    """List tasks, optionally filtered by status and/or parent."""
    q, args = "SELECT id, parent_id, title, status, agent_model, order_index FROM tasks WHERE 1=1", []
    if status:
        q += " AND status=?"; args.append(status)
    if parent_id is not None:
        q += " AND parent_id=?"; args.append(parent_id)
    with connect() as conn:
        return [dict(r) for r in conn.execute(q + " ORDER BY order_index, id", args)]


@mcp.tool()
def list_ready() -> list[dict]:
    """Tasks the dispatcher would start next, in order (queued + all deps done)."""
    with connect() as conn:
        return [dict(r) for r in conn.execute(READY_SQL)]


@mcp.tool()
def get_tree(root_id: int | None = None) -> list[dict]:
    """Task hierarchy as nested dicts (whole forest if root_id is omitted)."""
    with connect() as conn:
        rows = [dict(r) for r in conn.execute(
            "SELECT id, parent_id, title, status, agent_model FROM tasks ORDER BY order_index, id")]
    by_parent = {}
    for r in rows:
        by_parent.setdefault(r["parent_id"], []).append(r)

    def build(node):
        node["children"] = [build(c) for c in by_parent.get(node["id"], [])]
        return node

    roots = [r for r in rows if r["id"] == root_id] if root_id else by_parent.get(None, [])
    return [build(r) for r in roots]


# ---------- board tools (taskboard UI) ----------

EDITABLE_FIELDS = ("title", "description", "agent_model", "harness", "effort", "permission_mode", "status", "color",
                   "order_index")


def _check_expected(conn, task_id, expected):
    """Reject the edit when a field it changes was modified by someone else since the client saw it."""
    if not expected:
        return
    row = _task(conn, task_id)
    conflicts = {field: row[field] for field, value in expected.items() if field in EDITABLE_FIELDS and row[field] != value}
    if conflicts:
        raise ValueError("conflict: " + json.dumps({"task_id": task_id, "current": conflicts}))


@mcp.tool()
def edit_task(task_id: int, changes: dict, expected: dict) -> dict:
    """Taskboard edit with conflict check: apply `changes` only if every field in `expected` still has that value.
    Editable fields: title, description, agent_model, status, color (color null = follow status), order_index
    (board-wide rank; kanban columns and siblings are sorted by it)."""
    unknown = set(changes) - set(EDITABLE_FIELDS)
    if unknown:
        raise ValueError(f"not editable: {', '.join(sorted(unknown))}")
    if "status" in changes:
        changes = {**changes, "status": _checked_status(changes["status"])}
    if changes.get("effort") not in (None, *sessions.EFFORTS):
        raise ValueError(f"effort must be one of {sessions.EFFORTS}")
    if "agent_model" in changes:
        sessions.model_argument(changes["agent_model"])
    if "harness" in changes:
        sessions.validate_harness(changes["harness"])
    if changes.get("permission_mode") not in (None, *sessions.PERMISSION_MODES):
        raise ValueError(f"permission_mode must be one of {tuple(sessions.PERMISSION_MODES)}")

    with connect() as conn:
        conn.execute("BEGIN IMMEDIATE")
        try:
            _check_expected(conn, task_id, expected)
            if "agent_model" in changes or "harness" in changes:
                row = _task(conn, task_id)
                sessions.validate_model_harness(changes.get("agent_model", row["agent_model"]),
                                                changes.get("harness", row["harness"]))
            if changes:
                record_field_changes(conn, task_id, changes, "taskboard")
                sets = ", ".join(f"{field}=?" for field in changes)
                conn.execute(f"UPDATE tasks SET {sets}, updated_at=CURRENT_TIMESTAMP WHERE id=?",
                             (*changes.values(), task_id))
                described = {k: v for k, v in changes.items() if k not in ("description", "color")}
                if "description" in changes:
                    described["description"] = "(edited)"
                log_event(conn, task_id, "edit", ", ".join(f"{k}={v}" for k, v in described.items()))
            conn.execute("COMMIT")
        except Exception:
            conn.execute("ROLLBACK")
            raise
        return _full(conn, task_id)

BOARD_COLUMNS = ("id, parent_id, title, description, status, agent_model, harness, effort, permission_mode, order_index, result, "
                 "x, y, width, body_height, collapsed, color, session_state, session_detail, session_updated_at, "
                 "session_launch")


def _would_create_parent_cycle(conn, task_id, parent_id):
    row = conn.execute(
        """WITH RECURSIVE ancestors(id) AS (
               SELECT ? UNION SELECT t.parent_id FROM tasks t JOIN ancestors a ON t.id = a.id
               WHERE t.parent_id IS NOT NULL)
           SELECT 1 FROM ancestors WHERE id = ?""",
        (parent_id, task_id),
    ).fetchone()
    return row is not None


@mcp.tool()
def get_board(at: float | None = None) -> dict:
    """Whole board in one call for the taskboard UI: tasks with layout, dependencies, palette and revision.
    Every row version is kept (copy-on-write history), so `at` (Unix seconds) returns the board exactly as it was
    at that moment: deleted tasks are back, later edits are undone. `history` gives the {first, last} moments
    that can be viewed. The palette is not versioned; a past board shows the current palette."""
    if at is None:
        sessions.reconcile()
    with connect() as conn:
        board = _board_at(conn, at) if at is not None else _live_board(conn)
        board["palette"] = [r[0] for r in conn.execute("SELECT color FROM board_palette ORDER BY position")]
        board["history"] = history.span(conn)
        return board


def _live_board(conn):
    return {
        "revision": board_revision(conn),
        "tasks": [{**dict(r), "workspace": sessions.session_directory(r["id"])}
                  for r in conn.execute(f"SELECT {BOARD_COLUMNS} FROM tasks ORDER BY id")],
        "dependencies": [{"task_id": r[0], "depends_on_id": r[1]} for r in conn.execute(
            "SELECT task_id, depends_on_id FROM task_deps ORDER BY task_id, depends_on_id")],
    }


def _board_at(conn, moment):
    rows = history.tasks_at(conn, moment)
    columns = [column.strip() for column in BOARD_COLUMNS.split(",")]
    tasks = []
    for task_id, row in rows.items():
        task = {column: row.get(column) for column in columns}
        if task["parent_id"] not in rows:
            task["parent_id"] = None
        task["workspace"] = sessions.session_directory(task_id)
        tasks.append(task)
    return {"revision": f"at:{moment}", "at": moment, "tasks": tasks,
            "dependencies": history.dependencies_at(conn, moment, set(rows))}


@mcp.tool()
def get_history_moments() -> list[float]:
    """Every distinct moment (Unix seconds, ascending) at which the board changed, for the taskboard timeline."""
    with connect() as conn:
        return history.change_moments(conn)


@mcp.tool()
def get_field_history(task_id: int, field: str | None = None) -> list[dict]:
    """Every value a task field had before it was replaced, newest first."""
    with connect() as conn:
        query = "SELECT field, old_value, new_value, source, created_at FROM task_field_history WHERE task_id=?"
        arguments = [task_id]
        if field:
            query += " AND field=?"
            arguments.append(field)
        return [dict(r) for r in conn.execute(query + " ORDER BY id DESC", arguments)]


@mcp.tool()
def get_board_revision() -> str:
    """Cheap change marker; the UI polls this and reloads the board when it differs. Also notices dead sessions."""
    sessions.reconcile()
    with connect() as conn:
        return board_revision(conn)


@mcp.tool()
def set_parent(task_id: int, parent_id: int | None = None) -> dict:
    """Set or clear (parent_id omitted/null) a task's parent. Hierarchy cycles are rejected."""
    with connect() as conn:
        _task(conn, task_id)
        if parent_id is not None:
            _task(conn, parent_id)
            if parent_id == task_id or _would_create_parent_cycle(conn, task_id, parent_id):
                raise ValueError("parent would create a cycle")
        conn.execute("UPDATE tasks SET parent_id=?, updated_at=CURRENT_TIMESTAMP WHERE id=?", (parent_id, task_id))
        log_event(conn, task_id, "edit", f"parent={parent_id}")
        return _full(conn, task_id)


@mcp.tool()
def set_layout(task_id: int, x: float, y: float, width: float, body_height: float, collapsed: bool) -> str:
    """Store a task block's position, size and collapsed state (UI only; not logged as an event)."""
    with connect() as conn:
        _task(conn, task_id)
        conn.execute(
            "UPDATE tasks SET x=?, y=?, width=?, body_height=?, collapsed=?, updated_at=CURRENT_TIMESTAMP WHERE id=?",
            (x, y, width, body_height, int(collapsed), task_id))
        return "ok"


@mcp.tool()
def set_color(task_id: int, color: str | None = None) -> str:
    """Set a task's color as '#rrggbb', or clear it (color omitted/null) to follow its status."""
    with connect() as conn:
        _task(conn, task_id)
        conn.execute("UPDATE tasks SET color=?, updated_at=CURRENT_TIMESTAMP WHERE id=?", (color, task_id))
        return "ok"


@mcp.tool()
def set_palette(colors: list[str]) -> str:
    """Replace the board's shared color palette ('#rrggbb' entries, in display order)."""
    with connect() as conn:
        conn.execute("BEGIN IMMEDIATE")
        conn.execute("DELETE FROM board_palette")
        conn.executemany("INSERT INTO board_palette(position, color) VALUES (?, ?)", list(enumerate(colors)))
        conn.execute("COMMIT")
        return "ok"


def _create_board_task(conn, entry):
    fields = {k: entry.get(k) for k in ("title", "description", "agent_model", "harness", "effort",
                                          "permission_mode", "status", "color")}
    fields["title"] = fields["title"] or "New task"
    fields["description"] = fields["description"] or ""
    fields["agent_model"] = fields["agent_model"] or "claude-opus-5-5"
    fields["harness"] = fields["harness"] or "claude-code"
    fields["status"] = _checked_status(fields["status"] or "backlog")
    sessions.model_argument(fields["agent_model"])
    sessions.validate_model_harness(fields["agent_model"], fields["harness"])
    cur = conn.execute(
        """INSERT INTO tasks(title, description, agent_model, harness, effort, permission_mode, status, color,
                             x, y, width, body_height, collapsed, order_index)
           VALUES (:title, :description, :agent_model, :harness, :effort, :permission_mode, :status, :color,
                   :x, :y, :width, :body_height, :collapsed, :order_index)""",
        {**fields, **{k: entry.get(k) for k in ("x", "y", "width", "body_height")},
         "collapsed": int(bool(entry.get("collapsed"))), "order_index": int(entry.get("order_index") or 0)})
    log_event(conn, cur.lastrowid, "created", fields["title"])
    return cur.lastrowid


def _apply_edit(conn, task_id, changes, expected, conflicts):
    row = _task(conn, task_id)
    clashes = {f: row[f] for f, v in expected.items() if f in EDITABLE_FIELDS and row[f] != v}
    if clashes:
        conflicts.append({"task_id": task_id, "current": clashes})
        return
    record_field_changes(conn, task_id, changes, "taskboard")
    sets = ", ".join(f"{field}=?" for field in changes)
    conn.execute(f"UPDATE tasks SET {sets}, updated_at=CURRENT_TIMESTAMP WHERE id=?", (*changes.values(), task_id))
    log_event(conn, task_id, "edit", ", ".join(sorted(changes)))


@mcp.tool()
def save_board(created: list[dict] | None = None, edits: list[dict] | None = None, layouts: list[dict] | None = None,
               deleted: list[int] | None = None, links_added: list[dict] | None = None,
               links_removed: list[dict] | None = None, palette: list[str] | None = None) -> dict:
    """Apply a taskboard save atomically.
    created: [{ref, title, ..., x, y, width, body_height, collapsed}] - `ref` is a client id (negative) used in links.
    edits: [{task_id, changes:{field: value}, expected:{field: value}}] - a field whose server value differs from
    `expected` is not written and is returned in `conflicts` (other fields of the same edit still apply).
    layouts: [{task_id, x, y, width, body_height, collapsed}]. links: [{kind: parent|dependency, from, to}] where
    parent links child `to` under `from` and dependency makes `to` depend on `from`; ids may be refs.
    Returns {ids: {ref: new_id}, conflicts: [...], revision}."""
    created, edits, layouts = created or [], edits or [], layouts or []
    deleted, links_added, links_removed = deleted or [], links_added or [], links_removed or []
    ids, conflicts, stopped = {}, [], []

    with connect() as conn:
        conn.execute("BEGIN IMMEDIATE")
        try:
            for entry in created:
                ids[str(entry["ref"])] = _create_board_task(conn, entry)

            def resolve(value):
                return ids.get(str(value), value)

            for edit in edits:
                task_id = resolve(edit["task_id"])
                changes = edit.get("changes", {})
                if "status" in changes:
                    changes = {**changes, "status": _checked_status(changes["status"])}
                expected = edit.get("expected", {})
                unknown = set(changes) - set(EDITABLE_FIELDS)
                if unknown:
                    raise ValueError(f"not editable: {', '.join(sorted(unknown))}")
                if "agent_model" in changes:
                    sessions.model_argument(changes["agent_model"])
                if "harness" in changes:
                    sessions.validate_harness(changes["harness"])
                if "agent_model" in changes or "harness" in changes:
                    row = _task(conn, task_id)
                    sessions.validate_model_harness(changes.get("agent_model", row["agent_model"]),
                                                    changes.get("harness", row["harness"]))
                for field, value in changes.items():
                    field_conflicts = []
                    _apply_edit(conn, task_id, {field: value},
                                {field: expected[field]} if field in expected else {}, field_conflicts)
                    for conflict in field_conflicts:
                        conflicts.append({**conflict, "field": field, "mine": value})

            for layout in layouts:
                conn.execute("UPDATE tasks SET x=?, y=?, width=?, body_height=?, collapsed=? WHERE id=?",
                             (layout["x"], layout["y"], layout["width"], layout["body_height"],
                              int(bool(layout["collapsed"])), resolve(layout["task_id"])))

            for link in links_removed:
                source, target = resolve(link["from"]), resolve(link["to"])
                if link["kind"] == "parent":
                    conn.execute("UPDATE tasks SET parent_id=NULL WHERE id=? AND parent_id=?", (target, source))
                else:
                    conn.execute("DELETE FROM task_deps WHERE task_id=? AND depends_on_id=?", (target, source))

            for link in links_added:
                source, target = resolve(link["from"]), resolve(link["to"])
                if link["kind"] == "parent":
                    if source == target or _would_create_parent_cycle(conn, target, source):
                        raise ValueError(f"parent link {source}->{target} would create a cycle")
                    conn.execute("UPDATE tasks SET parent_id=? WHERE id=?", (source, target))
                else:
                    if source == target or would_create_cycle(conn, target, source):
                        raise ValueError(f"dependency {source}->{target} would create a cycle")
                    conn.execute("INSERT OR IGNORE INTO task_deps VALUES (?, ?)", (target, source))

            for task_id in deleted:
                stopped.extend(_subtree_ids(conn, resolve(task_id)))
                conn.execute("DELETE FROM tasks WHERE id=?", (resolve(task_id),))

            if palette is not None:
                conn.execute("DELETE FROM board_palette")
                conn.executemany("INSERT INTO board_palette(position, color) VALUES (?, ?)", list(enumerate(palette)))
            conn.execute("COMMIT")
        except Exception:
            conn.execute("ROLLBACK")
            raise
        revision = board_revision(conn)
    _stop_sessions(stopped)
    return {"ids": ids, "conflicts": conflicts, "revision": revision}


@mcp.tool()
def get_options() -> dict:
    """Models, harnesses and effort levels the taskboard offers. agent_model takes the model id and
    harness takes the harness id; claude-code runs Claude models only, dsh runs Claude or DeepSeek."""
    return {"models": [{"id": model, "label": label} for model, label in sessions.MODELS.items()],
            "harnesses": [{"id": h, "label": label} for h, label in sessions.HARNESSES.items()],
            "efforts": list(sessions.EFFORTS),
            "permission_modes": list(sessions.PERMISSION_MODES)}


@mcp.tool()
def get_workspace(task_id: int) -> dict:
    """The task's working directory and its conventions. Agents: write reports to reports/ (end with reports/summary.md),
    notes to notes/, scratch data to tmp/ and deliverables to artifacts/. Lists the files already there.
    Every task's files live in its own directory by default. The 'parent' and 'children' entries give the
    workspace directories of related tasks — all are sibling directories, so you can read a parent's or child's
    work directly from its path."""
    space = sessions.workspace(task_id)
    with connect() as conn:
        row = _task(conn, task_id)
        if row["parent_id"]:
            space["parent"] = {"task_id": row["parent_id"], "directory": sessions.session_directory(row["parent_id"])}
        space["children"] = [{"task_id": r["id"], "directory": sessions.session_directory(r["id"])}
                             for r in conn.execute("SELECT id FROM tasks WHERE parent_id=? ORDER BY order_index, id", (task_id,))]
    return space


@mcp.tool()
def start_session(task_id: int) -> dict:
    """Start the task's agent session headless (or resume it). Runs Claude Code, or the DeepSeek
    Harness web chat when harness is 'dsh'. It keeps running with no terminal attached."""
    return sessions.start(task_id)


@mcp.tool()
def restart_session(task_id: int, fresh: bool = False) -> dict:
    """Restart the task's session with its current model/effort/mode. fresh=False continues the same conversation
    under the new settings; fresh=True starts a new conversation."""
    return sessions.restart(task_id, fresh)


@mcp.tool()
def session_status(task_id: int) -> dict:
    """State of the task's session: starting, working, waiting_input, idle (turn finished) or ended."""
    sessions.reconcile()
    return sessions.describe(task_id)


@mcp.tool()
def send_session_message(task_id: int, message: str) -> dict:
    """Type a message into the task's running session, as if the human typed it."""
    return sessions.send(task_id, message)


@mcp.tool()
def stop_session(task_id: int) -> dict:
    """End the task's session process. The conversation can be resumed later with start_session."""
    return sessions.stop(task_id)


@mcp.tool()
def session_screen(task_id: int, lines: int = 60) -> str:
    """Last lines of the session terminal, for a quick look without attaching."""
    return sessions.screen(task_id, lines)


@mcp.tool()
def get_subrouter_status() -> dict:
    """Subrouter subscription accounts and their cooldown state.
    Shows each Anthropic (and other provider) account in the rotation pool, whether it is cooling down
    (rate-limited), and when it becomes available again. Use 'subrouter login anthropic' to add accounts.
    Note: this reflects rate-limit cooldowns tracked by subrouter, not Claude.ai monthly usage quotas."""
    return sessions.subrouter_status()


# ---------- agent tools (sessions) ----------

@mcp.tool()
def get_task(task_id: int) -> dict:
    """Full task: fields, children, dependencies, event log. Agents call this first. Includes the task's workspace
    directory; get_workspace also exposes the parent's and children's workspaces so related work is readable."""
    with connect() as conn:
        t = _full(conn, task_id)
        if t["parent_id"]:
            p = _task(conn, t["parent_id"])
            t["parent"] = {"id": p["id"], "title": p["title"], "description": p["description"]}
        t["dependency_results"] = [dict(r) for r in conn.execute(
            "SELECT t.id, t.title, t.result FROM task_deps d JOIN tasks t ON t.id=d.depends_on_id WHERE d.task_id=?",
            (task_id,))]
        return t


@mcp.tool()
def get_context(task_id: int, include: list[str] | None = None, max_depth: int | None = None) -> dict:
    """Inherited context in one call: walk the parent chain from this task up to the root and, for each ancestor,
    return its title and description plus the chosen workspace documents, so an agent reads the whole hierarchy
    without a get_task round-trip per level. 'include' selects what to pull per task (default ['summary']):
    'summary' reads reports/summary.md, 'notes' every file under notes/, 'artifacts' every file under artifacts/.
    'ancestors' is ordered root-first; 'self' is this task. max_depth limits how many ancestors to climb (omit for
    the full chain to the root). For a single task's documents, call get_context(that_id, include=[...], max_depth=0).
    Treat nearer-ancestor guidelines as overriding farther ones, and your own task as overriding all."""
    parts = include if include is not None else ["summary"]

    def entry(conn, tid):
        row = _task(conn, tid)
        node = {"id": row["id"], "title": row["title"], "description": row["description"],
                "workspace": sessions.session_directory(tid)}
        docs = sessions.read_context_docs(tid, parts)
        if docs:
            node["docs"] = docs
        return node, row["parent_id"]

    with connect() as conn:
        this, parent_id = entry(conn, task_id)
        ancestors = []
        climbed = 0
        while parent_id and (max_depth is None or climbed < max_depth):
            node, parent_id = entry(conn, parent_id)
            ancestors.append(node)
            climbed += 1
    ancestors.reverse()
    return {"self": this, "ancestors": ancestors, "included": parts}


@mcp.tool()
def add_note(task_id: int, message: str) -> str:
    """Append a progress note to the task log."""
    with connect() as conn:
        _task(conn, task_id)
        log_event(conn, task_id, "note", message)
        return "ok"


@mcp.tool()
def create_subtask(parent_id: int, title: str, description: str = "", agent_model: str | None = None,
                   depends_on: list[int] | None = None) -> dict:
    """Agent splits work: creates a 'queued' child, last in the board-wide order, inheriting the
    parent's model, harness, effort and permission mode."""
    with connect() as conn:
        p = _task(conn, parent_id)
        cur = conn.execute(
            """INSERT INTO tasks(title, description, parent_id, agent_model, harness, effort, permission_mode, status,
                                 order_index)
               VALUES (?, ?, ?, ?, ?, ?, ?, 'queued', ?)""",
            (title, description, parent_id, agent_model or p["agent_model"],
             p["harness"], p["effort"], p["permission_mode"], next_order_index(conn)))
        tid = cur.lastrowid
        log_event(conn, tid, "created", f"subtask of {parent_id}")
        for d in depends_on or []:
            conn.execute("INSERT OR IGNORE INTO task_deps VALUES (?, ?)", (tid, d))
        return _full(conn, tid)


@mcp.tool()
def complete_task(task_id: int, result: str, needs_review: bool = True) -> str:
    """Agent finished. Stores result; status -> review (default) or done."""
    with connect() as conn:
        conn.execute("UPDATE tasks SET result=? WHERE id=?", (result, task_id))
        _set_status(conn, task_id, "review" if needs_review else "done", "completed by agent")
    return "ok"


@mcp.tool()
def fail_task(task_id: int, reason: str, blocked: bool = False) -> str:
    """Agent cannot finish: the task becomes 'blocked' with the reason in its log. `blocked` is kept for older
    callers and no longer changes anything."""
    with connect() as conn:
        _set_status(conn, task_id, "blocked", reason)
        return "ok"


# ---------- skills (reusable instruction packs, versioned with the code) ----------

@mcp.tool()
def list_skills() -> list[dict]:
    """Every reusable skill with its name and description. Skills are instruction packs versioned in the
    Taskboard module; an agent whose task names a skill loads its full text with get_skill before acting.
    A skill may list the secret keys it needs; fetch each value at run time with get_secret."""
    return skills.list_skills()


@mcp.tool()
def get_skill(name: str) -> dict:
    """The full SKILL.md body for one skill, by folder name, plus its parsed name and description."""
    return skills.get_skill(name)


# ---------- secrets (credentials agents fetch at run time, never stored in the board) ----------

@mcp.tool()
def list_secrets() -> list[dict]:
    """Every stored secret key with its description - never the value. Use it to see what credentials exist
    and which key to request; read the value itself with get_secret."""
    return secret_store.list_secrets()


@mcp.tool()
def get_secret(key: str) -> dict:
    """The value for one secret key. Agents call this at run time to retrieve a credential a skill or task
    named, instead of embedding it in a prompt, skill, or commit."""
    return secret_store.get_secret(key)


@mcp.tool()
def set_secret(key: str, value: str, description: str | None = None) -> dict:
    """Create or replace a secret. The description is kept when omitted on an update. Returns the key and
    description only, never the value."""
    return secret_store.set_secret(key, value, description)


@mcp.tool()
def delete_secret(key: str) -> dict:
    """Remove a secret by key. Returns whether a row existed."""
    return secret_store.delete_secret(key)


if __name__ == "__main__":
    mcp.run()  # stdio transport
