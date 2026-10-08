# Agent task manager (minimal)

Jira-like task DB where the workers are agent sessions. **One task = one agent session** running the task's `agent_model`.

## Layout
Code lives in the Modules repository under `Taskboard/server/`; data lives outside it.

| path | role |
|---|---|
| `paths.py` | the only place locations are decided: code dir, data dir (`AGENTS_DATA_DIR`, default `~/agents/manager`), DB (`TASKS_DB`), sessions (`AGENTS_SESSIONS_DIR`) |
| `db.py` | SQLite schema, ready-query, atomic claim, field history |
| `history.py` | copy-on-write board history: triggers keep every version of every task and dependency row |
| `server.py` | MCP server (stdio) — tools for the taskboard UI, you and agents |
| `sessions.py` | headless Claude Code sessions (one tmux session per task), hooks, launch settings, MCP config for agents |
| `session_hook.py`, `session_exit.py` | record session state / exit reason into the DB |
| `dispatcher.py` | starts sessions for queued tasks |
| `board.py` | curses board |
| `subrouter/token.mjs` | Claude subscription token from the subrouter account pool |
| `supervisor.md`, `start.sh` | supervisor session and tmux launcher |
| `fake_agent.py`, `demo.py`, `test_board_tools.py` | test agent, sample data, end-to-end MCP test |

Data directory (not versioned): `tasks.db` and `sessions/task-<id>/` workspaces.

## Model
- task: `parent_id`, `order_index` (one board-wide rank: kanban columns, siblings in the tree and the dispatcher all sort by it), `status`, `agent_model`, `result`, `session_id`
- statuses: `backlog → queued → wip → review → done` (+ `blocked`, and `daemon` beside `wip`). `daemon` marks a long-running background session (a watcher that polls something and reports): the dispatcher never starts it and does not count it against its worker slots, and a daemon task whose session ended without completing becomes `blocked` like `wip`. Renamed statuses: `running` → `wip`, `ready` → `queued`, `failed` → `blocked`. Old rows are renamed on connect, old history snapshots are shown with the new names, and the old names are still accepted as input
- **ready to run** = status `queued` AND all dependencies `done`; picked in tree order: siblings by `order_index`, a parent before its children (existing sibling orders were renumbered into one global rank in tree order, once)
- dependency cycles are rejected

## History
Nothing on the board is ever overwritten. SQLite triggers on `tasks` and `task_deps` append each new row state
(create, update, delete) to `board_history`, whichever process writes. `get_board(at=<unix seconds>)` returns
the board exactly as it was then (deleted tasks back, later edits undone); every `get_board` reports the viewable
`history: {first, last}`; `get_history_moments` lists every change moment. In the taskboard, **View → Timeline**
(hidden by default) shows the History bar: a full-width slider stepping by the chosen precision (minute, hour,
day, week), back/forward buttons for one step, tick marks at board changes (grouped when they would crowd), and
**Live** to return to the present. A past board is read-only. Browsing requires no unsaved changes. A database that predates history is seeded once
from `task_field_history`; tasks deleted before that cannot be recovered. The palette is not versioned.

## Tools
Manager: `create_task`, `update_task`, `set_status`, `add_dependency`, `remove_dependency`, `delete_task`, `list_tasks`, `list_ready`, `get_tree`
Agent: `get_task`, `get_workspace`, `add_note`, `create_subtask`, `complete_task`, `fail_task`

## Workspaces
Every task runs in its own directory `sessions/task-<id>/` (reports/ notes/ tmp/ artifacts/) under the data
directory. All task workspaces are siblings, so a session can read its parent's or children's work directly
from their paths — `get_workspace` returns those related directories.

## Architecture
```
tmux session "agents"
 ├─ board       top: board.py (task tree TUI)   bottom: supervisor Claude session (supervisor.md)
 ├─ dispatcher  claims queued tasks, opens a window per task
 └─ task-<id>   agent session for agent_model under harness (claude-code | dsh),
                cwd sessions/task-<id>/, MCP = task-manager
```
- A task picks a **model** and, independently, a **harness** (both from `get_options`):
  - `agent_model`: `claude-opus-5-5` (and the other `claude-*` ids), `deepseek-chat`/`deepseek-flash`, the legacy `claude:opus`/`claude:sonnet`/`claude:haiku`/`claude` forms, or `echo` (test).
  - `harness`: `claude-code` (default) or `dsh` (DeepSeek Harness). **Claude Code runs Claude models only; dsh runs either** — the same Claude model can run under either harness. Invalid pairs (a DeepSeek model under `claude-code`) are rejected by the server.
- `claude-code` worker sessions are normal interactive Claude Code: a terminal session the board's **Open session** attaches to (tmux), and you can jump in and type at any time. Session closed before finishing -> task `blocked`; the board's `R` key resumes it (`claude --resume <id>`).
- `dsh` tasks run on **one shared dsh web server that you run yourself** (`dsh-web.service`, port `$AGENTS_DSH_WEB_PORT`, default `3080`), not a process the board owns. `start_session` records the task's handoff link (`/plugins/taskboard-deeplink/open?id=<id>`) and marks the task wip; `stop_session` ends it; `describe`/`reconcile` leave dsh tasks alone. **Open session** is always available for a dsh task (also when its session ended or hit a limit): the board starts the link if needed and opens `dsh-app://task/<id>`, which raises the single DSH app window on the task's one conversation.
- The link is resolved by the **`taskboard-deeplink`** plugin and the `dsh-app` launcher (`../dsh-deeplink/`, install once with `../dsh-deeplink/install.sh`). The plugin mounts the **task-manager MCP** on the dsh server, keeps one dsh session per task (server-side registry, then the task workspace's existing session, then a new seeded one) and hands the task to the already-open app tab. See `../dsh-deeplink/README.md`.

## Run
```bash
cd Taskboard/server           # in the Modules checkout
claude /login                 # if your token is expired
./start.sh                    # SUPERVISOR_MODEL=opus WORKERS=3 by default
```
In the supervisor pane, say e.g. *"plan a task tree for X, sonnet for code, haiku for docs, then queue it"*.

Board keys: `↑↓/jk` select · `Enter` open that task's Claude session · `r` queue · `d` approve (done)
· `R` resume session · `s` back to board · `q` quit.
tmux: `C-b w` window list, `C-b o` switch pane, `C-b d` detach (everything keeps running).

Test without LLMs: `AGENTS_DATA_DIR=/tmp/demo uv run python demo.py; AGENTS_DATA_DIR=/tmp/demo ./start.sh`
(demo uses `echo` agents). End-to-end MCP test: `uv run python test_board_tools.py`.

Use from your own Claude Code too:
`claude mcp add task-manager -- uv run --project <Modules>/Taskboard/server python <Modules>/Taskboard/server/server.py`

## Permissions
Each task picks a mode: `auto` (no prompts) or `manual` (asks before reads and edits). Sessions load only
project/local Claude settings, so your user settings cannot silently auto-approve. See
`sessions.PERMISSION_MODES`.

## Config
`TASKS_DB=/path/to/tasks.db` overrides the DB location (default `./tasks.db`).
