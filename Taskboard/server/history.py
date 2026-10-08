import hashlib
import json
import time

from statuses import current_status

HISTORY_SCHEMA = """
CREATE TABLE IF NOT EXISTS board_history (
    version       INTEGER PRIMARY KEY AUTOINCREMENT,
    changed_at    REAL NOT NULL,
    kind          TEXT NOT NULL CHECK (kind IN ('task', 'dependency')),
    operation     TEXT NOT NULL CHECK (operation IN ('create', 'update', 'delete')),
    task_id       INTEGER NOT NULL,
    depends_on_id INTEGER,
    snapshot      TEXT
);
CREATE INDEX IF NOT EXISTS board_history_rows ON board_history(kind, task_id, depends_on_id, version);
CREATE INDEX IF NOT EXISTS board_history_time ON board_history(changed_at);
"""

NOW_SQL = "(julianday('now') - 2440587.5) * 86400.0"

UNVERSIONED_TASK_COLUMNS = ("updated_at",)

TRIGGER_PREFIX = "board_history_"


def task_columns(conn):
    return [row[1] for row in conn.execute("PRAGMA table_info(tasks)")]


def _snapshot_sql(columns, row_alias):
    pairs = ", ".join(f"'{column}', {row_alias}.{column}" for column in columns)
    return f"json_object({pairs})"


def _changed_sql(columns):
    versioned = [column for column in columns if column not in UNVERSIONED_TASK_COLUMNS]
    return " OR ".join(f"OLD.{column} IS NOT NEW.{column}" for column in versioned)


def _task_triggers(columns):
    signature = hashlib.sha1(",".join(columns).encode()).hexdigest()[:10]
    insert = f"""INSERT INTO board_history(changed_at, kind, operation, task_id, snapshot)"""
    return {
        f"{TRIGGER_PREFIX}task_create_{signature}": f"""AFTER INSERT ON tasks BEGIN
            {insert} VALUES ({NOW_SQL}, 'task', 'create', NEW.id, {_snapshot_sql(columns, 'NEW')}); END""",
        f"{TRIGGER_PREFIX}task_update_{signature}": f"""AFTER UPDATE ON tasks WHEN {_changed_sql(columns)} BEGIN
            {insert} VALUES ({NOW_SQL}, 'task', 'update', NEW.id, {_snapshot_sql(columns, 'NEW')}); END""",
        f"{TRIGGER_PREFIX}task_delete_{signature}": f"""AFTER DELETE ON tasks BEGIN
            {insert} VALUES ({NOW_SQL}, 'task', 'delete', OLD.id, {_snapshot_sql(columns, 'OLD')}); END""",
    }


def _dependency_triggers():
    insert = "INSERT INTO board_history(changed_at, kind, operation, task_id, depends_on_id)"
    return {
        f"{TRIGGER_PREFIX}dependency_create": f"""AFTER INSERT ON task_deps BEGIN
            {insert} VALUES ({NOW_SQL}, 'dependency', 'create', NEW.task_id, NEW.depends_on_id); END""",
        f"{TRIGGER_PREFIX}dependency_delete": f"""AFTER DELETE ON task_deps BEGIN
            {insert} VALUES ({NOW_SQL}, 'dependency', 'delete', OLD.task_id, OLD.depends_on_id); END""",
    }


def install(conn):
    conn.executescript(HISTORY_SCHEMA)
    wanted = {**_task_triggers(task_columns(conn)), **_dependency_triggers()}
    existing = {row[0] for row in conn.execute(
        "SELECT name FROM sqlite_master WHERE type='trigger' AND name LIKE ?", (TRIGGER_PREFIX + "%",))}
    if existing == set(wanted):
        return

    conn.execute("BEGIN IMMEDIATE")
    try:
        for name in existing - set(wanted):
            conn.execute(f"DROP TRIGGER IF EXISTS {name}")
        for name, body in wanted.items():
            conn.execute(f"CREATE TRIGGER IF NOT EXISTS {name} {body}")
        _backfill(conn)
        conn.execute("COMMIT")
    except Exception:
        conn.execute("ROLLBACK")
        raise


def _epoch(conn, sql_timestamp):
    return conn.execute("SELECT (julianday(?) - 2440587.5) * 86400.0", (sql_timestamp,)).fetchone()[0]


def _typed_like(current, text):
    if text is None or current is None or isinstance(current, str):
        return text
    try:
        return type(current)(float(text)) if isinstance(current, int) else type(current)(text)
    except ValueError:
        return text


def _append(conn, changed_at, kind, operation, task_id, depends_on_id=None, snapshot=None):
    conn.execute(
        "INSERT INTO board_history(changed_at, kind, operation, task_id, depends_on_id, snapshot) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        (changed_at, kind, operation, task_id, depends_on_id, None if snapshot is None else json.dumps(snapshot)))


def _backfill(conn):
    if conn.execute("SELECT 1 FROM board_history LIMIT 1").fetchone():
        return

    columns = task_columns(conn)
    versions = []
    for row in conn.execute("SELECT * FROM tasks ORDER BY id").fetchall():
        current = dict(row)
        changes = conn.execute(
            "SELECT field, old_value, new_value, created_at FROM task_field_history WHERE task_id=? ORDER BY id",
            (row["id"],)).fetchall()
        known = [change for change in changes if change["field"] in columns]

        state = dict(current)
        for change in reversed(known):
            state[change["field"]] = _typed_like(current[change["field"]], change["old_value"])
        versions.append((_epoch(conn, row["created_at"]), "create", row["id"], dict(state)))

        for change in known:
            state[change["field"]] = _typed_like(current[change["field"]], change["new_value"])
            versions.append((_epoch(conn, change["created_at"]), "update", row["id"], dict(state)))

        if state != current:
            versions.append((time.time(), "update", row["id"], current))

    for changed_at, operation, task_id, snapshot in sorted(versions, key=lambda version: version[0]):
        _append(conn, changed_at, "task", operation, task_id, snapshot=snapshot)

    for dependency in conn.execute(
            "SELECT d.task_id, d.depends_on_id, t.created_at FROM task_deps d JOIN tasks t ON t.id = d.task_id"):
        _append(conn, _epoch(conn, dependency["created_at"]), "dependency", "create",
                dependency["task_id"], dependency["depends_on_id"])


def span(conn):
    first, last = conn.execute("SELECT MIN(changed_at), MAX(changed_at) FROM board_history").fetchone()
    return {"first": first, "last": last}


def change_moments(conn):
    return [row[0] for row in conn.execute(
        "SELECT DISTINCT changed_at FROM board_history ORDER BY changed_at")]


def tasks_at(conn, moment):
    rows = conn.execute(
        """SELECT h.task_id, h.snapshot FROM board_history h
           JOIN (SELECT task_id, MAX(version) AS version FROM board_history
                 WHERE kind = 'task' AND changed_at <= ? GROUP BY task_id) latest
             ON latest.version = h.version
           WHERE h.operation != 'delete'
           ORDER BY h.task_id""",
        (moment,))
    return {row["task_id"]: _with_current_status(json.loads(row["snapshot"])) for row in rows}


def _with_current_status(snapshot):
    snapshot["status"] = current_status(snapshot.get("status"))
    return snapshot


def dependencies_at(conn, moment, existing_tasks):
    rows = conn.execute(
        """SELECT h.task_id, h.depends_on_id FROM board_history h
           JOIN (SELECT task_id, depends_on_id, MAX(version) AS version FROM board_history
                 WHERE kind = 'dependency' AND changed_at <= ? GROUP BY task_id, depends_on_id) latest
             ON latest.version = h.version
           WHERE h.operation = 'create'
           ORDER BY h.task_id, h.depends_on_id""",
        (moment,))
    return [{"task_id": row["task_id"], "depends_on_id": row["depends_on_id"]} for row in rows
            if row["task_id"] in existing_tasks and row["depends_on_id"] in existing_tasks]
