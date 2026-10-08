"""Headless agent sessions: one detached tmux session per task, rooted under sessions/task-<id>.

A task picks a model and, independently, a harness (tasks.harness): 'claude-code' runs Claude Code,
'dsh' runs the DeepSeek Harness. Claude Code is a headless terminal session in a detached tmux session
the viewer attaches to. A 'dsh' task is NOT a board-owned process: it runs on one shared dsh web server
you run yourself (AGENTS_DSH_WEB_PORT, default 3080); the board only records a deep link carrying the
task id, and the taskboard-deeplink plugin on that server opens the task's session and seeds the prompt.
Session state for Claude Code is written by its hooks (session_hook.py) into tasks.session_state.
"""
import json
import os
import shlex
import shutil
import subprocess
import sys
import uuid

from db import DB_PATH, connect, log_event

from paths import CODE_DIR as HERE, SESSIONS_ROOT  # noqa: E402
HOOK_SCRIPT = os.path.join(HERE, "session_hook.py")
TOKEN_HELPER = os.path.join(HERE, "subrouter", "token.mjs")
SUBROUTER_BIN = os.path.join(HERE, "subrouter", "node_modules", ".bin", "subrouter")

MODELS = {
    "claude-opus-5-5": "Opus 5.5",
    "claude-opus-4-8": "Opus 4.8",
    "claude-sonnet-4-6": "Sonnet 4.6",
    "claude-haiku-4-5": "Haiku 4.5",
    "deepseek-chat": "DeepSeek Chat",
    "deepseek-flash": "DeepSeek Flash",
}
# A task picks a model (above) and, independently, a harness to run it in. Claude Code runs
# Claude models only; the DeepSeek Harness ('dsh') runs Claude models (provider 'claude', via
# dsh's connected Claude accounts) and DeepSeek models (provider 'deepseek-official').
HARNESSES = {
    "claude-code": "Claude Code",
    "dsh": "DeepSeek Harness",
}
DEFAULT_HARNESS = "claude-code"
# A dsh task runs on ONE shared dsh web server that you run yourself (default port 3080). The board
# does not launch a process per dsh task; it just records a deep link carrying the task id. The
# taskboard-deeplink plugin on that server (see ../dsh-deeplink/) resolves the id to the task's
# workspace, opens a session there, seeds the worker prompt, and mounts the task-manager MCP.
DSH_WEB_PORT = int(os.environ.get("AGENTS_DSH_WEB_PORT", "3080"))


def dsh_deeplink_url(task_id):
    return f"http://127.0.0.1:{DSH_WEB_PORT}/plugins/taskboard-deeplink/open?id={int(task_id)}"

# 'default' means "no reasoning-effort override" (use the model's own default); the others map 1:1 to
# Claude's reasoning levels (also what Claude Code's --effort takes).
EFFORTS = ("default", "low", "medium", "high", "xhigh", "max")
PERMISSION_MODES = {
    "auto": "auto",
    "manual": "default",
}
WORKSPACE_FOLDERS = ("reports", "notes", "tmp", "artifacts")
SESSION_STATES = ("starting", "working", "waiting_input", "idle", "error", "ended")
HOOK_EVENTS = {
    "SessionStart": "idle",
    "UserPromptSubmit": "working",
    "PreToolUse": "working",
    "PostToolUse": "working",
    "Notification": "waiting_input",
    "PermissionRequest": "waiting_input",
    "Stop": "idle",
    "StopFailure": "error",
    "SessionEnd": "ended",
}

WORKER_PROMPT = """You are the agent for task #{id} on the task-manager board (MCP server "task-manager").
Start with get_task({id}) to read the task, its immediate parent and dependency results.
Then call get_context({id}) once to get your inherited context: it walks the whole parent chain up to the root and
returns every ancestor's title, description and reports/summary.md in a single call. Parents may keep hierarchy-wide
guidelines, conventions or a design document there; treat nearer-ancestor guidelines as binding and overriding farther
ones, and your own task as overriding all. Pass include=['summary','notes','artifacts'] for more per task, or
get_context(<id>, max_depth=0) to pull one specific task's documents when you want to go deeper.
Your task workspace is the current directory; get_workspace({id}) describes it. Keep your files there:
  reports/    findings and final write-ups (finish with reports/summary.md)
  notes/      running notes and plans
  tmp/        scratch and intermediate data, safe to delete
  artifacts/  outputs other tasks or the human should use
get_workspace({id}) also lists your parent's and children's workspace directories — sibling folders you can
read directly to pick up their work (and they can read yours).
Report progress with add_note, split work with create_subtask.
Finish with complete_task({id}, result=<summary>) or fail_task({id}, reason=...) when you cannot finish (the task becomes blocked).
The task-manager MCP server is developed in-house, so it can grow new tools and conventions. While you work, notice
where it slowed you down or where a tool call, note convention, or workflow shortcut would have made the job faster or
safer. At the end of the task, alongside your result, suggest concrete improvements to this MCP server: new or changed
tools and their arguments, extra fields to return, or habits future agents should adopt. Keep it to the few highest-value
ideas grounded in what this task actually needed; skip it only when nothing would have helped.
The human may type into this session at any time: follow their instructions."""


def session_name(task_id):
    return f"task-{task_id}"


def session_directory(task_id):
    return os.path.join(SESSIONS_ROOT, session_name(task_id))


TMUX_SOCKET = os.environ.get("AGENTS_TMUX_SOCKET")


def tmux_command(*args):
    """All agent sessions live on one tmux server; tests point AGENTS_TMUX_SOCKET at a private one."""
    return ["tmux", *(["-L", TMUX_SOCKET] if TMUX_SOCKET else []), *args]


def tmux(*args, check=True):
    return subprocess.run(tmux_command(*args), check=check, capture_output=True, text=True)


def is_alive(task_id):
    return tmux("has-session", "-t", f"={session_name(task_id)}", check=False).returncode == 0


FAKE_AGENT_MODEL = "echo"


def provider_for_model(agent_model):
    """The dsh provider that serves a model: DeepSeek models via 'deepseek-official',
    Claude models (incl. the legacy 'claude:<alias>' form) via dsh's 'claude' provider."""
    return "deepseek-official" if agent_model.startswith("deepseek") else "claude"


def model_argument(agent_model):
    """Validate agent_model and return the Claude `--model` value, or None when the model takes no
    `--model` flag ('echo' test agent, or a DeepSeek model). agent_model is a full model id
    ('claude-opus-5-5', 'deepseek-chat'), the legacy 'claude:<alias>' form, or 'echo'."""
    if agent_model == FAKE_AGENT_MODEL:
        return None
    if agent_model.startswith("deepseek"):
        return None
    if agent_model.startswith("claude:"):
        return agent_model.split(":", 1)[1] or None
    if agent_model.startswith("claude-"):
        return agent_model
    raise ValueError(f"unknown agent_model {agent_model!r}")


def validate_harness(harness):
    if harness not in HARNESSES:
        raise ValueError(f"harness must be one of {tuple(HARNESSES)}")
    return harness


def validate_model_harness(agent_model, harness):
    """Claude Code can only run Claude models; dsh runs either. Raises on an unsupported pair."""
    validate_harness(harness)
    if harness == "claude-code" and provider_for_model(agent_model) != "claude":
        raise ValueError(f"harness 'claude-code' cannot run model {agent_model!r}; use the 'dsh' harness")


def mcp_config():
    """The task-manager MCP server as agent sessions launch it: this checkout's server.py, same data paths."""
    server = {"command": "uv", "args": ["run", "--project", HERE, "python", os.path.join(HERE, "server.py")],
              "env": {"TASKS_DB": DB_PATH, "AGENTS_SESSIONS_DIR": SESSIONS_ROOT}}
    return {"mcpServers": {"task-manager": server}}


def hook_settings():
    command = f"{shlex.quote(sys.executable)} {shlex.quote(HOOK_SCRIPT)}"
    hooks = {event: [{"hooks": [{"type": "command", "command": f"{command} {event}"}]}] for event in HOOK_EVENTS}
    return {"hooks": hooks}


def oauth_environment():
    """Use the subrouter subscription when it can produce a token; otherwise Claude Code keeps its own login."""
    if not shutil.which("node") or not os.path.exists(TOKEN_HELPER):
        return {}, "claude login"
    result = subprocess.run(["node", TOKEN_HELPER], capture_output=True, text=True)
    if result.returncode != 0 or not result.stdout:
        return {}, f"claude login (subrouter unavailable: {result.stderr.strip()[:160]})"
    return {"CLAUDE_CODE_OAUTH_TOKEN": result.stdout}, "subrouter"


def subrouter_status() -> dict:
    """Return subrouter account pool status as structured data."""
    if not shutil.which("node") or not os.path.exists(SUBROUTER_BIN):
        return {"available": False, "reason": "subrouter not installed"}
    accounts_result = subprocess.run([SUBROUTER_BIN, "account", "list", "--json"],
                                     capture_output=True, text=True)
    status_result = subprocess.run([SUBROUTER_BIN, "status"], capture_output=True, text=True)
    accounts = {}
    if accounts_result.returncode == 0 and accounts_result.stdout.strip():
        try:
            accounts = json.loads(accounts_result.stdout)
        except Exception:
            pass
    return {
        "available": True,
        "accounts": accounts,
        "summary": status_result.stdout.strip() if status_result.returncode == 0 else status_result.stderr.strip(),
        "login_hint": "Run 'subrouter login anthropic' to add or refresh an account. Run it again to add a second account to the rotation pool.",
    }


def task_harness(task):
    """The task's harness, defaulting to Claude Code for rows written before the field existed."""
    try:
        return task["harness"] or DEFAULT_HARNESS
    except (KeyError, IndexError):
        return DEFAULT_HARNESS


def agent_command(task, session_uuid, resume):
    # dsh tasks don't launch a process (see start()); only Claude Code / echo reach here.
    if task["agent_model"] == FAKE_AGENT_MODEL:
        return [sys.executable, os.path.join(HERE, "fake_agent.py")]
    return claude_command(task, session_uuid, resume)


def claude_command(task, session_uuid, resume):
    mode = task["permission_mode"] or "auto"
    if mode not in PERMISSION_MODES:
        raise ValueError(f"permission mode must be one of {tuple(PERMISSION_MODES)}")
    command = ["claude", "--mcp-config", json.dumps(mcp_config()), "--strict-mcp-config", "--setting-sources", "project,local",
               "--permission-mode", PERMISSION_MODES[mode], "--allowedTools", "mcp__task-manager",
               "--settings", json.dumps(hook_settings())]
    model = model_argument(task["agent_model"])
    if model:
        command += ["--model", model]
    if task["effort"] and task["effort"] != "default":
        if task["effort"] not in EFFORTS:
            raise ValueError(f"effort must be one of {EFFORTS}")
        command += ["--effort", task["effort"]]
    if resume:
        return command + ["--resume", session_uuid]
    return command + ["--session-id", session_uuid, "-n", f"task-{task['id']}: {task['title']}",
                      WORKER_PROMPT.format(id=task["id"])]


def prepare_workspace(task_id):
    directory = session_directory(task_id)
    for folder in WORKSPACE_FOLDERS:
        os.makedirs(os.path.join(directory, folder), exist_ok=True)
    return directory


def workspace(task_id):
    directory = session_directory(task_id)
    files = []
    if os.path.isdir(directory):
        for root, _, names in os.walk(directory):
            for name in names:
                path = os.path.join(root, name)
                files.append({"path": os.path.relpath(path, directory), "bytes": os.path.getsize(path)})
    return {
        "directory": directory,
        "folders": {
            "reports": "findings and final write-ups; finish with reports/summary.md",
            "notes": "running notes and plans",
            "tmp": "scratch and intermediate data, safe to delete",
            "artifacts": "outputs other tasks or the human should use",
        },
        "files": sorted(files, key=lambda entry: entry["path"]),
    }


CONTEXT_DOCS = {
    "summary": os.path.join("reports", "summary.md"),
    "notes": "notes",
    "artifacts": "artifacts",
}
CONTEXT_DOC_LIMIT = 20000


def _read_text(path):
    try:
        with open(path, encoding="utf-8", errors="replace") as handle:
            text = handle.read(CONTEXT_DOC_LIMIT + 1)
    except OSError:
        return None
    if len(text) > CONTEXT_DOC_LIMIT:
        return text[:CONTEXT_DOC_LIMIT] + "\n[...truncated]"
    return text


def read_context_docs(task_id, parts):
    """Collect the requested workspace documents for one task. 'summary' is reports/summary.md; 'notes' and
    'artifacts' return every readable file under those folders. Returns a part->content (or part->{path: content})
    map holding only what actually exists."""
    directory = session_directory(task_id)
    docs = {}
    for part in parts:
        relative = CONTEXT_DOCS.get(part)
        if relative is None:
            continue
        target = os.path.join(directory, relative)
        if part == "summary":
            text = _read_text(target)
            if text is not None:
                docs[part] = text
            continue
        if not os.path.isdir(target):
            continue
        folder = {}
        for root, _, names in os.walk(target):
            for name in sorted(names):
                full = os.path.join(root, name)
                text = _read_text(full)
                if text is not None:
                    folder[os.path.relpath(full, directory)] = text
        if folder:
            docs[part] = folder
    return docs


def conversation_path(directory, session_uuid):
    """Claude Code keeps each conversation under ~/.claude/projects/<cwd with '/' and '.' replaced by '-'>."""
    project = os.path.realpath(directory).replace("/", "-").replace(".", "-")
    return os.path.join(os.path.expanduser("~"), ".claude", "projects", project, f"{session_uuid}.jsonl")


def conversation_exists(directory, session_uuid):
    return os.path.exists(conversation_path(directory, session_uuid))


def exit_marker_path(task_id):
    return os.path.join(session_directory(task_id), "tmp", "session-exited")


def exit_log_path(task_id):
    return os.path.join(session_directory(task_id), "tmp", "session-exit.log")


def last_exit_reason(task_id):
    path = exit_log_path(task_id)
    if not os.path.exists(path):
        return None
    with open(path, errors="replace") as log:
        lines = [line.strip() for line in log.read().splitlines() if line.strip()]
    return " | ".join(lines[-3:])[:300] if lines else None


def set_state(task_id, state, detail=None):
    with connect() as conn:
        conn.execute(
            "UPDATE tasks SET session_state=?, session_detail=?, session_updated_at=CURRENT_TIMESTAMP WHERE id=?",
            (state, detail, task_id))


def launch_settings(task):
    return {"agent_model": task["agent_model"], "harness": task_harness(task), "effort": task["effort"] or None,
            "permission_mode": task["permission_mode"] or "auto"}


def restart(task_id, fresh=False):
    """Stop the session and start it again with the task's current model/effort/mode.
    fresh=False keeps the conversation (resume); fresh=True starts a new conversation."""
    stop(task_id)
    if fresh:
        with connect() as conn:
            conn.execute("UPDATE tasks SET claude_session=NULL WHERE id=?", (task_id,))
    return start(task_id)


def start_dsh_link(task_id, task):
    """A dsh task runs on your shared dsh web server (AGENTS_DSH_WEB_PORT, default 3080), not a process
    the board owns. Record the deep link carrying the task id; the taskboard-deeplink plugin on that
    server opens the task's session and seeds the worker prompt. 'Open session' points the browser at it."""
    prepare_workspace(task_id)
    url = dsh_deeplink_url(task_id)
    with connect() as conn:
        conn.execute(
            """UPDATE tasks SET session_state='working', session_detail=?, session_updated_at=CURRENT_TIMESTAMP,
                   session_launch=?,
                   status=CASE WHEN status IN ('backlog', 'queued', 'blocked') THEN 'wip' ELSE status END,
                   updated_at=CURRENT_TIMESTAMP
               WHERE id=?""",
            (url, json.dumps(launch_settings(task)), task_id))
        log_event(conn, task_id, "session", f"dsh link on shared server: {url}")
    return describe(task_id)


def start(task_id):
    """Start the task's session headless, or resume it if it ended. Returns the session description."""
    with connect() as conn:
        task = conn.execute("SELECT * FROM tasks WHERE id=?", (task_id,)).fetchone()
    if task is None:
        raise ValueError(f"task {task_id} not found")
    if describe(task_id)["alive"]:
        return describe(task_id)
    if task_harness(task) == "dsh":
        return start_dsh_link(task_id, task)
    if is_alive(task_id):
        tmux("kill-session", "-t", f"={session_name(task_id)}", check=False)

    directory = prepare_workspace(task_id)
    if os.path.exists(exit_marker_path(task_id)):
        os.remove(exit_marker_path(task_id))
    resume = task["claude_session"] is not None and conversation_exists(directory, task["claude_session"])
    session_uuid = task["claude_session"] if resume else str(uuid.uuid4())

    command = agent_command(task, session_uuid, resume)
    environment, auth = oauth_environment()
    environment.update({"TASK_ID": str(task_id), "TASKS_DB": DB_PATH})
    if os.environ.get("AGENTS_EXIT_FIFO"):
        environment["AGENTS_EXIT_FIFO"] = os.environ["AGENTS_EXIT_FIFO"]
    exit_log = exit_log_path(task_id)
    on_exit = (f"{shlex.quote(sys.executable)} {shlex.quote(os.path.join(HERE, 'session_exit.py'))} "
               f"{task_id} \"$status\" {shlex.quote(exit_log)}")
    shell = f"{shlex.join(command)} 2> >(tee {shlex.quote(exit_log)} >&2); status=$?; {on_exit}"

    with connect() as conn:
        conn.execute(
            """UPDATE tasks SET claude_session=?, tmux_window=?, session_state='starting', session_detail=?,
                   session_updated_at=CURRENT_TIMESTAMP, session_launch=?,
                   status=CASE WHEN status IN ('backlog', 'queued', 'blocked') THEN 'wip' ELSE status END,
                   updated_at=CURRENT_TIMESTAMP
               WHERE id=?""",
            (session_uuid, session_name(task_id), f"auth: {auth}", json.dumps(launch_settings(task)), task_id))
        log_event(conn, task_id, "session",
                  f"{'resumed' if resume else 'started'} {task['agent_model']} effort={task['effort'] or 'default'} "
                  f"mode={task['permission_mode'] or 'auto'} "
                  f"in tmux {session_name(task_id)} ({auth})")

    arguments = ["new-session", "-d", "-s", session_name(task_id), "-c", directory, "-x", "200", "-y", "50"]
    for key, value in environment.items():
        arguments += ["-e", f"{key}={value}"]
    tmux(*arguments, "bash", "-c", shell)
    return describe(task_id)


def send(task_id, message):
    """Type a message into the running session as if the human typed it."""
    if not describe(task_id)["alive"]:
        raise ValueError(f"session for task {task_id} is not running; start it first")
    target = f"={session_name(task_id)}:"
    tmux("send-keys", "-t", target, "-l", message)
    tmux("send-keys", "-t", target, "Enter")
    with connect() as conn:
        log_event(conn, task_id, "message", message)
    return describe(task_id)


def stop(task_id):
    if is_alive(task_id):
        tmux("kill-session", "-t", f"={session_name(task_id)}", check=False)
    if describe(task_id)["state"] not in (None, "ended"):
        set_state(task_id, "ended", "stopped from the board")
    return describe(task_id)


def stop_for_deleted_task(task_id):
    """Kill the session of a task that no longer exists; its state row is gone with the task."""
    if is_alive(task_id):
        tmux("kill-session", "-t", f"={session_name(task_id)}", check=False)


def screen(task_id, lines=60):
    if not is_alive(task_id):
        return ""
    result = tmux("capture-pane", "-p", "-t", f"={session_name(task_id)}:", "-S", f"-{lines}", check=False)
    return result.stdout


def describe(task_id):
    with connect() as conn:
        row = conn.execute(
            "SELECT session_state, session_detail, session_updated_at, session_launch, harness FROM tasks WHERE id=?",
            (task_id,)).fetchone()
    state = row["session_state"] if row and row["session_state"] else None
    if row and (row["harness"] or DEFAULT_HARNESS) == "dsh":
        # dsh tasks run on your shared dsh server, not a board-owned process; "alive" = a live link.
        alive = state not in (None, "ended")
    else:
        agent_exited = os.path.exists(exit_marker_path(task_id))
        alive = is_alive(task_id) and not agent_exited
        if not alive and state not in (None, "ended"):
            state = "ended"
    return {
        "task_id": task_id,
        "alive": alive,
        "state": state,
        "detail": row["session_detail"] if row else None,
        "updated_at": row["session_updated_at"] if row else None,
        "launched_with": json.loads(row["session_launch"]) if row and row["session_launch"] else None,
        "tmux_target": session_name(task_id),
        "directory": session_directory(task_id),
    }


def reconcile():
    """Sessions whose tmux session vanished are ended (e.g. after a reboot). dsh tasks have no
    board-owned process (they live on your shared dsh server), so they are left untouched."""
    with connect() as conn:
        rows = conn.execute(
            "SELECT id FROM tasks WHERE session_state IS NOT NULL AND session_state != 'ended' "
            "AND (harness IS NULL OR harness != 'dsh')").fetchall()
    for row in rows:
        if not is_alive(row["id"]):
            set_state(row["id"], "ended", "session process is gone")
