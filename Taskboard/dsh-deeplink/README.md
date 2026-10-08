# taskboard-deeplink (dsh web plugin + DSH app window)

Ties taskboard `dsh` tasks to **one shared dsh web server you run yourself** (`dsh-web.service`, port 3080)
and to **one DSH app window** (a chromeless Firefox profile) with **one tab per task**. "Open session" on
a dsh task raises that window on the task's tab: if the tab is already open it is selected (not reloaded),
otherwise it is opened. Each tab always shows its task's single dsh session.

## Flow
```
board "Open session"  ──xdg-open dsh-app://task/<id>  (with an xdg-activation token)
        │
        ▼
dsh-app
  window not running → firefox --remote-debugging-port $DSH_APP_REMOTE_PORT  …/open?id=<id>[&token=…]
  window running     → lib/task-tabs.mjs over WebDriver BiDi (loopback):
                         select the tab whose URL is /?tbTask=<id>, or open it in a new tab;
                       firefox <url>&handoff=1   (remoted to the running instance → raises the window);
                       lib/task-tabs.mjs closes that handoff tab again
        ▼
/plugins/taskboard-deeplink/open → (enters with the launch token if given) → /?tbTask=<id>
        ▼
client half in the tab: resolve the task's session, ctx.uiWorkspace.openSession(sessionId);
re-selects it whenever the tab becomes visible (dsh shares its current-session selection between tabs)
```

Opens are serialized with `flock` on `$DSH_APP_PROFILE/.dsh-app-router.lock`, because the Firefox remote
agent accepts one BiDi session at a time.

### One task, one session
The client resolves the session in this order, after the session list is loaded (`list.phase === "ready"`):
1. **registry** — the session remembered for the task on the server
   (`$DSH_HOME/task-board/deeplink-sessions.json`, override `$TASKBOARD_DEEPLINK_REGISTRY`);
2. **workspace** — the most recently updated top-level session whose cwd is the task workspace;
3. **created** — a new workspace session, then the model/effort from the board and the worker prompt.

Only a *created* session is seeded, so re-opening never re-sends the prompt. Every open also names the
session `#<id> <task title>` when its title differs (an explicit rename, which pins the title so dsh's
automatic titling leaves it alone), so existing sessions and board renames are picked up too.

## Board sidebar
The plugin replaces dsh's left sidebar list (`sidebar.workspaces`, priority -2, so it shadows dsh's
workspace browser and `dsh-project-tree`) with collapsible groups:

- **wip**, **daemon**, **review**, **done**: the board's dsh tasks (newest first, session state as a dot). They come from
  the same `tasks.db` the task-manager MCP server writes, polled through `/board?since=<revision>` every 2 s.
  The revision combines the MCP server's `board_state.revision` with the task→session registry, so a board
  change or a newly linked task session shows up within one poll; an unchanged one costs a one-line reply.
  Clicking a task opens its session in the current tab (`?tbTask=<id>` is updated in place); the
  `dsh-app://task/<id>` links from the board still select or open a task's own tab.
- **other sessions**: every top-level dsh session not linked to a board task: sessions started without the
  board, sessions of deleted tasks, and older ones. It follows dsh's own session list live; clicking one
  opens it in the current tab.

**+ New session** at the top starts a plain dsh session in the server user's home directory (no board
task) and opens it. Every row has an archive button on hover: it archives the session in dsh (stopping
running work) and the session leaves the panel. A board task whose session was archived gets a fresh
session the next time it is opened. Collapsed groups are remembered in localStorage.

## Plugin routes (`lib/index.mjs`)
| route | purpose |
|---|---|
| `GET /plugins/taskboard-deeplink/task?id=N` | `{task, title, cwd, prompt, sessionId, provider, model, reasoningEffort?}` |
| `POST /plugins/taskboard-deeplink/task-session` | `{task, sessionId}` → records the task's session |
| `GET /plugins/taskboard-deeplink/open?id=N[&token=T][&handoff]` | enters dsh (with the launch token) and goes to `/?tbTask=N`; `handoff` waits so the router can close it first |
| `GET /plugins/taskboard-deeplink/board[?since=R]` | `{revision, groups: {wip, daemon, review, done}, taskSessions: {sessionId: task}}`; groups are listed in display order, or `{revision, unchanged: true}` |
| `POST /plugins/taskboard-deeplink/open-task` | `{task}` → runs `dsh-app --task` for this server's app window |
| `GET /plugins/taskboard-deeplink/home` | `{home}`: the server user's home directory, where **+ New session** starts |
| `GET|POST /plugins/taskboard-deeplink/debug` | progress log sink (`$TMPDIR/taskboard-deeplink-debug.log`) |

The task-manager MCP server is mounted on the same dsh server by `cordis.patch.yml`.

## Install (one-time, per machine)
```
./install.sh
systemctl --user restart dsh-web
```
It links the plugin into the dsh web profile, links `~/.local/bin/dsh-app` to `./dsh-app` and installs
`dsh-app.desktop` as the `x-scheme-handler/dsh-app` handler. Close the DSH app window once so the next
launch starts it with the remote agent.

`dsh-app` options: `--task <id>`, `dsh-app://task/<id>`, `--kiosk`; env `DSH_APP_PORT`, `DSH_APP_PROFILE`,
`DSH_APP_REMOTE_PORT` (default 9271, loopback only). Path overrides: `$TASKBOARD_SERVER_DIR`, `$TASKS_DB`,
`$AGENTS_SESSIONS_DIR`, `$DSH_HOME`.

After changing the plugin, restart the dsh server (plugin modules are cached) and reload the app window.

## Verification
Task #96 `tmp/rounds.py` opens tasks through `xdg-open dsh-app://task/<id>` in three rounds
(`92 93 96 97`, `97 92 96 93`, `96 96 93 92 97 92`) and checks after every open, over BiDi: the task's tab
is the selected one, it shows exactly the registered session, no tab was reloaded; at the end exactly one
tab per task exists and no new dsh session was created. Passed on an isolated server/profile and twice on
the live window.
