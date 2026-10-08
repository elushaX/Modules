---
name: elvees-programming
description: Build, configure, and board-test workflow for Elvees runtime and modules repositories. Use whenever working in an /elvees/ repository — especially the runtime or modules directories — to build all platforms, configure, run tests on a hardware board, and guard against regressions.
---

# Elvees programming workflow

Apply this whenever the working path contains an `/elvees/` segment, and always
when touching the `runtime` or `modules` directories. This complements the
general [[programming]] conventions — it does not replace them.

## Repository locations (check first — do not assume `src/elvees/modules`)

The primary, up-to-date checkouts live under `/home/auser/src/elvees/er_platform/`:

- **modules** superproject: `/home/auser/src/elvees/er_platform/modules` (branch
  `dev`). `runtime` is its submodule at `extern/runtime`, and there is also a
  top-level `infrastructure` submodule. `dev` is the development branch in modules,
  runtime and infrastructure alike; pull all three to their `dev` tips when asked
  to update.
- Other checkouts exist (`src/elvees/modules-worktrees/*`, `src/elvees/tmp/*`,
  `src/elvees/experiments/...`) — these are stale/experimental, not the main tree.
  `git worktree list` from the main checkout shows the real worktrees.
- Dependency source clones (DDS stack, drivers, dmrlib, mbelib, ambe, etc.) are at
  `/home/auser/src/elvees/libs/extern/dds/*` and `/home/auser/src/elvees/libs/intern/*`.

Always `pwd` / `ls` to confirm the checkout before building; do not hardcode an
assumed path.

## Squadus access (do NOT notify anyone automatically)

Do **not** notify anyone (including Nikolay Motin / `nmotin`) before using the
USRP or a board. Coordination messages are not required; reserve the board in the
Jenkins pool (below) and use it. Only send a Squadus message if the user
explicitly asks you to in the current conversation. **If you do send one, write
it in the first person as the user (Ilya / ishurupov); never identify yourself as
an AI, assistant, or bot** — the account is the user's.

Squadus is a Rocket.Chat instance; authenticate with a personal access token via
the `X-Auth-Token` / `X-User-Id` headers (reachable directly, no SOCKS proxy):

    SQ='https://im-squadus.elvees.com'
    H_AUTH='-H X-Auth-Token:<get_secret("elvees.squadus_token")>'
    H_USER='-H X-User-Id:<get_secret("elvees.squadus_user_id")>'

    curl -s $H_AUTH $H_USER "$SQ/api/v1/me"                                  # whoami
    curl -s $H_AUTH $H_USER -H 'Content-Type:application/json' \
        -d '{"channel":"@nmotin","text":"..."}' "$SQ/api/v1/chat.postMessage"
    curl -s $H_AUTH $H_USER "$SQ/api/v1/im.history?username=nmotin&count=20"  # poll DMs

Token (secret — Elvees Squadus, user `ishurupov` / id `<get_secret("elvees.squadus_user_id")>`):
`<get_secret("elvees.squadus_token")>`. Regenerate at
`https://im-squadus.elvees.com/account/tokens` if it stops working.

## Jira access (jira.elvees.com — REST works, the web form does not)

Jira is the authoritative task tracker for the SRISE work; the task-board tickets
usually point at it. Verified working from this host:

- **Reachable directly** (`192.168.65.101:443`) — **no SOCKS proxy needed**, unlike
  Jenkins and the GitLab web UI. Do not open a tunnel for Jira.
- **Auth is the blocker, not the network.** Anonymous `/rest/api/2/myself` → `401`;
  a `Bearer` PAT or any other service token (Jenkins, GitLab) → `401`; Jira Server
  has no PAT here. Only **HTTP basic auth with the account password** works:

      JIRA='https://jira.elvees.com'
      AUTH='-u ishurupov:<get_secret("elvees.jira_password")>'
      curl -sk $AUTH "$JIRA/rest/api/2/myself"        # 200 = authenticated

  Password: `<get_secret("elvees.jira_password")>` (secret — Elvees Jira, user
  `ishurupov`). Do **not** script `/login.jsp` (`os_username`/`os_password` +
  captcha resources); the REST API is the way in.
- Server: Jira Server 9.4.14, "RnD Center ELVEES". Statuses and board columns are
  **Russian**: `В работе` = In Progress (`status id 3`), `БЭКЛОГ` = backlog,
  `РЕВЬЮ` = review, `ВЫПОЛНЕНО` = done.

### Boards, filters, quick filters

- My board (the one the human links to) is `rapidView=714`, name `Мотин Н.А.`,
  type kanban. Its board filter is `id 19117`, JQL:

      project in (SRISE, SRISE2) AND issuetype in (Task, Epic, Sub-task)
        AND assignee in (dpetrakov, kalyamovskiy, kkizin, ibaranov, ikorotkov,
                         akhazov, zstepanov, vpleshakov, anarizhnyy, akolmykov,
                         gbasov, nmotin, ishurupov, agoremykina, asunitskiy,
                         aavlin, EMPTY) ORDER BY Rank ASC

- `GET /rest/agile/1.0/board/714/configuration` gives the columns and their status
  ids: `БЭКЛОГ` = 10100/11700/1/11706/10900/10000, **`В РАБОТЕ` = 3**,
  `РЕВЬЮ` = 10002, `ВЫПОЛНЕНО` = 10001/11003.
- **Quick filters are not in `/rest/api/2`** (`/rest/api/2/quickfilter/<id>` → 404).
  Read them from the greenhopper config:
  `GET /rest/greenhopper/1.0/rapidviewconfig/editmodel?rapidViewId=714` →
  `quickFilters[]`. On board 714: `3023` = "Только мои задачи"
  (`assignee = currentUser()`), `3024` = "Последние обновления"
  (`updatedDate >= -1d`). Reproduce the UI by appending that JQL to filter 19117.

### Reading the work items

    # everything in progress on my board
    curl -sk $AUTH -G "$JIRA/rest/api/2/search" \
      --data-urlencode 'jql=filter=19117 AND status=3 ORDER BY updated DESC' \
      --data-urlencode 'fields=key,summary,status,assignee,issuetype,created,updated,description'
    # fastest way to one issue with history
    curl -sk $AUTH "$JIRA/rest/api/2/issue/SRISE2-98?expand=changelog"

**The description is often just a GitLab MR note link** — e.g.
`https://git.elvees.com/dis/sunrise/sunrise-platform/modules/-/merge_requests/141#note_296913`.
That comment is the actual task statement, so resolve it with the GitLab token
before reporting on the ticket:

    G=https://git.elvees.com/api/v4/projects/dis%2Fsunrise%2Fsunrise-platform%2Fmodules
    curl -sk -H 'PRIVATE-TOKEN: <get_secret("elvees.gitlab_token")>' \
      "$G/merge_requests/141/notes/296913"      # body, author, file:line

## CoreManager package location

The built CoreManager / CoreManagerTests deploy packages are under
`modules/build/packages/` (`CoreManager`, `CoreManagerTests`, each with
`x64`/`cpu`/`core1`/`core2`).

## Always test on a board

Changes to `runtime` or `modules` must be validated on hardware. Do not treat a
change as done based on a Linux/host build alone. When developing in `runtime` or
`modules`, always try to run the tests on one of the boards from the pool — any
free board of the right kind will do, you are not tied to one specific board.

- Before running board tests, determine the board and **reserve it first** (see
  below). If the conversation already names a board to use, use that one.
  Otherwise reserve **any FREE board** from the pool (ask the user only if the
  board kind is ambiguous) — do not skip the board run just because a particular
  named board is busy.
- **Never automatically select or use `OpenWRT_ASSET-2815`.** Do not reserve,
  connect to, deploy to, or run anything on that board unless the user explicitly
  asks for `OpenWRT_ASSET-2815` in the current conversation. Its FREE status does
  not count as permission.
- Never ignore the on-board tests. A green host build is not a substitute for a
  passing board run.
- Never mask a defect by resetting external state. Do not reboot or power-cycle
  hardware, restart services or drivers, clear persistent state, or add retries
  merely to make a failing or flaky operation pass. Find and fix the root cause.
  Perform such recovery or maintenance actions only when the user explicitly
  requests or approves them for their own operational purpose.

## Reserve the board before using it, unreserve after

Boards are shared. **Always reserve a board in the Jenkins lockable-resources
pool before using it, and always unreserve it after you are done** — otherwise
you collide with another user's run or leave a board locked out of the pool.
Reserve → use → unreserve is mandatory, every time, including for quick checks.

- Reservation UI: https://zeld-lpor-ci.elvees.com/lockable-resources/ — a Jenkins
  "Lockable Resources" page. Each board shows a status of FREE, RESERVED or
  LOCKED. Pick a **FREE** resource; its name (e.g. `OpenWRT_ASSET-2815`,
  `OpenWRT_ASSET-7777`, `NVCOM05-GUARDIAN-CB-001`, `zel_419_1`) is what you target
  for the board run. Labels tell you the kind, e.g. `hardware:mcom03` (Arm /
  Elcore50) vs `hardware:nvcom05`.
- Credentials: user `ishurupov`. The plain password is **not** accepted by the
  API (returns HTTP 401 / resolves to anonymous). Authenticate with the Jenkins
  **API token** via basic auth, e.g. `-u ishurupov:<JENKINS_TOKEN>`. Token
  (secret — Elvees CI): `<get_secret("elvees.jenkins_ci_token")>`. If it stops working,
  regenerate at `https://zeld-lpor-ci.elvees.com/user/ishurupov/configure`.

The host is only reachable through an SSH SOCKS proxy. Open a dynamic tunnel first
(if port 1080 is already bound, an earlier tunnel may exist — reuse it or pick
another local port):

    ssh -D 1080 ishurupov@supermarket-wm

A GUI browser is usually not available here, so drive the pool headless with curl
over the proxy. The site uses a private CA, so `-k` is required, and the reserve /
unreserve actions are CSRF-protected POSTs that need a crumb + session cookie
(this whole flow is verified working):

    PROXY='--socks5-hostname 127.0.0.1:1080'
    AUTH='-u ishurupov:<get_secret("elvees.jenkins_ci_token")>'   # API token, not password
    BASE='https://zeld-lpor-ci.elvees.com'

    # list resources and their free/reserved state
    curl -sk $PROXY $AUTH "$BASE/lockable-resources/api/json?depth=1"

    # fetch a CSRF crumb into a cookie jar
    CRUMB=$(curl -sk $PROXY $AUTH -c /tmp/jk_cookies.txt \
        "$BASE/crumbIssuer/api/json" | python3 -c 'import sys,json;print(json.load(sys.stdin)["crumb"])')

    # reserve (a 302 response means success)
    curl -sk $PROXY $AUTH -b /tmp/jk_cookies.txt -H "Jenkins-Crumb: $CRUMB" \
        -X POST "$BASE/lockable-resources/reserve?resource=OpenWRT_ASSET-7777"

    # ... run the board tests ...

    # unreserve when done (also a 302) — refetch the crumb first if needed
    curl -sk $PROXY $AUTH -b /tmp/jk_cookies.txt -H "Jenkins-Crumb: $CRUMB" \
        -X POST "$BASE/lockable-resources/unreserve?resource=OpenWRT_ASSET-7777"

Confirm the state change via the `api/json` query (`reserved`/`free` fields).
With a real browser the equivalent is
`google-chrome-stable --proxy-server="socks5://localhost:1080"`.

### SSH to the board itself

Once a board is reserved, log into it directly over SSH as root — no SOCKS proxy
and no special key setup are needed from this environment:

    ssh root@asset-2818        # board hostname = lowercase of the reserved resource

The board hostname is the lowercased resource name (reserved `ASSET-2818` →
`asset-2818`, resolves to its `*.elvees.com` address). The mcom03 boards run ALT
Linux aarch64. If a key is ever rejected, the board trusts the CI `jenkins_test`
deploy key; ask the user to authorize access rather than power-cycling.

The repo's own `./tools/reservation.sh` / `./tools/test.sh` automate this via a
`jenkins_lockable_resources` CLI in a `.myenv` venv plus `JENKINS_URL`/
`JENKINS_USER`/`JENKINS_TOKEN` env vars; when that env is not set up, use the curl
flow above.

## Commands

Run these from the repository root.

Configure the build trees:

    ./tools/configure.sh

Build all platforms (Linux, Arm, Elcore50, Nvcom05):

    ./tools/build.sh

Run the test suite on a specific board (`TESTING_BOARD=1` makes tests actually
execute on the hardware rather than being skipped):

    TESTING_BOARD=1 ./tools/test.sh board_name

`board_name` is the board hostname (for example `root@asset_name`).

Both `configure.sh` and `build.sh` build **all** platforms and use `set -e`, so a
single unavailable platform toolchain (or a host-only build break) aborts the
whole run before the one you care about. To build just one target when the full
run is blocked for unrelated reasons, build that preset directly, e.g.
`cmake --build build/RPi4-Release --target bundledRM_rpi4 --parallel N` after
sourcing `~/.mcom03sdk` and exporting `CMAKE_PREFIX_PATH=$HOME/.local` and
`STAGING_DIR=$ESDR_RPI4_BIN_ROOT`.

## SDK (ALT SDK) and the runtime-from-submodules build

The build moved to the **ALT SDK** (`SDK-ALT-v2`). Symptom of a stale/old SDK:
`configure.sh` fails on the `Arm` preset with
`Could not find toolchain file: .../ARM-BIN-ROOT/share/cmake/aarch64-alt-linux-toolchain.cmake`.
The authoritative, reference install order is `docker/Dockerfile` in the modules
repo — read it rather than guessing. Key facts:

- Env lives in `~/.mcom03sdk` (sourced by `.bashrc`/`.bash_profile`). **Source it
  in every build shell**; a fresh non-login shell has stale/absent vars.
- Install the SDK with its own installer (non-destructive — it archives the old
  SDK and rewrites `~/.mcom03sdk`):

      curl -sk --socks5-hostname 127.0.0.1:1080 -C - --retry 5 \
        -o SDK-ALT-v2.tar.gz \
        https://zeld-lpor-ftp.elvees.com/artifactory/voskhod/SDK-ALT-v2.tar.gz
      # 1.8 GB over the proxy; verify sha256 (see Dockerfile SDR_SDK_SHA256), it
      # often drops mid-download so resume with -C - until the size matches.
      mkdir -p sdk && tar -xzf SDK-ALT-v2.tar.gz -C sdk && cd sdk && ./install.sh SDK.tar.gz

  New ALT roots are `ARM-ALT-BIN-ROOT` / `ELCORE50-ALT-BIN-ROOT` (glibc buildroot,
  not the old musl/openwrt); `MCOM03_ARM_BIN_ROOT` now points at `ARM-ALT-BIN-ROOT`.
- The fresh SDK ships **toolchains only** — it does NOT contain the runtime
  packages (`radio_manager`, `blocks_factory`, `esdr_base_modules`) or the DDS
  stack. So `modules ./tools/configure.sh` (default `USE_SUBMODULES=OFF`,
  `find_package`) fails with "Could not find radio_manager 0.0.2" until you either:
  - **build the runtime from submodules**: `modules ./tools/configure.sh on`
    (`USE_SUBMODULES=ON` → `add_subdirectory(extern/runtime)`); OR
  - **build+install the runtime dev into the SDK first** (then modules OFF path
    works): in `extern/runtime`, `./tools/configure.sh && ./tools/build.sh &&
    ./tools/install.sh`.
  Either way the runtime needs the **DDS stack** (`microxrcedds_client`,
  `fast-dds`, `fast-cdr`, `micro-cdr`, `tinyxml2`, `spdlog`, `memory`, agent) —
  not optional. If the Linux-host DDS is already in `~/.local` but the target root
  lacks it, build each DDS repo for that preset in dependency order
  (tinyxml2 → fast-cdr → memory → fast-dds → micro-cdr → micro-xrce-dds-client →
  spdlog → micro-xrce-dds-agent), `cmake --install` into the SDK root, and copy
  the header-only `asio` into the target sysroot `usr/include` first. Clear any
  stale `build/<Preset>` dir (old cache paths) before configuring.

## Code style: clang-format and clang-tidy (must be clean at every commit)

These repos ship their own `.clang-format` and `.clang-tidy` at the repo root
(the `.clang-tidy` enables `-warnings-as-errors`, so CI treats every finding as
an error). **After each commit, the committed C/C++ must produce zero errors
from both clang-format and clang-tidy.** Do not rely on the editor's clangd-lsp
plugin for this — it does not diagnose formatting at all and typically does not
run clang-tidy (and ignores `WarningsAsErrors` when it does), so it silently
misses what CI enforces. Always check with the standalone binaries against the
repo-root configs.

clang-format — check a file (empty diff = clean), or fix in place:

    clang-format --style=file <file> | diff -u <file> -   # check
    clang-format --style=file -i <file>                   # fix

clang-tidy — needs a compile database; use one of the `build/*/compile_commands.json`
trees produced by `./tools/configure.sh` (e.g. `build/Linux-Release`):

    clang-tidy -p build/Linux-Release <file>

Fix everything until both are clean before considering a change done — the
`readability-identifier-naming` rule (const locals in `UPPER_CASE`),
`bugprone-implicit-widening-of-multiplication-result`, and the format rules are
the common offenders. Run these on every C/C++ file you touch, as part of the
same guard as the board tests below.

## GitLab access (submodules, worktrees)

The Elvees repos live on `git.elvees.com` and pull nested submodules
(`extern/args`, `extern/fkYAML`, `plog`, `extern/code-style-convention`, …) over
SSH (`git@git.elvees.com:...`). A fresh `git worktree` does not populate these, so
`git submodule update --init --recursive` is needed — but SSH auth/DNS often
fails from this environment.

Use this GitLab personal access token over HTTPS instead. Rewrite the SSH remotes
to token HTTPS for the session, then init submodules:

    git config url."https://oauth2:<get_secret("elvees.gitlab_token")>@git.elvees.com/".insteadOf "git@git.elvees.com:"
    git submodule update --init --recursive

Token: `<get_secret("elvees.gitlab_token")>` (secret — Elvees
GitLab). If `git.elvees.com` itself fails to resolve, that is a transient
DNS/VPN issue on the host, not an auth problem.

Note: not every submodule lives on `git.elvees.com`. `extern/fkYAML` clones from
`github.com`, so the elvees token rewrite does **not** help it, and github access
is frequently reset/blocked from this environment. When the network route fails,
do not keep retrying it — seed those submodules locally instead (see below).

## Create a linked worktree (isolate shared-repo work)

The `runtime` checkout is a submodule of the `modules` superproject; its working
copy is `/home/auser/src/elvees/er_platform/modules/extern/runtime`. That directory
is usually **shared** (another session/branch mid-task), so per the [[programming]]
convention do not edit it in place — make a linked worktree and work there.

Convention: keep runtime worktrees under the sibling
`/home/auser/src/elvees/runtime-worktrees/<name>` directory.

1. Create the worktree on a new branch off the current base commit. Run this from
   the shared runtime checkout:

       cd /home/auser/src/elvees/er_platform/modules/extern/runtime
       git worktree add -b <branch> /home/auser/src/elvees/runtime-worktrees/<name> HEAD

   `git worktree list` from any checkout shows all existing worktrees and their
   branches — check it first to reuse/avoid a name.

2. Populate submodules — `git worktree add` never does this. First try the normal
   token-HTTPS init inside the new worktree:

       cd /home/auser/src/elvees/runtime-worktrees/<name>
       git config url."https://oauth2:<get_secret("elvees.gitlab_token")>@git.elvees.com/".insteadOf "git@git.elvees.com:"
       git submodule update --init --recursive

3. If that init stalls or aborts (common — it times out, and `extern/fkYAML` from
   github.com fails with "Connection reset"), **seed the missing submodule working
   trees from an already-populated checkout** (the shared runtime checkout normally
   has them all). This is fully local, no network. For each submodule path still
   empty (only a `.git` file, or nothing):

       SEED=/home/auser/src/elvees/modules/extern/runtime          # a populated checkout
       cd /home/auser/src/elvees/runtime-worktrees/<name>
       for sm in extern/args extern/fkYAML extern/code-style-convention \
                 extern/utils/components/logger/plog; do
         [ -n "$(ls -A "$sm" 2>/dev/null | grep -v '^\.git$')" ] && continue   # already populated
         mkdir -p "$sm"
         ( cd "$SEED/$sm" && tar -c --exclude=.git . ) | ( cd "$sm" && tar -x )
       done

   Discover the exact set to seed with `git submodule status --recursive` (lines
   prefixed `-` are uninitialized). Verify afterwards that key headers exist, e.g.
   `extern/args/args.hxx` and `extern/fkYAML/single_include/fkYAML/node.hpp`.

   Seeded submodules will show as dirty content (` m` / ` M`) in `git status`.
   That is expected: **never stage or commit submodule pointer/content changes** —
   only add your own files. `git add` explicit paths, not `git add -A`.

4. The worktree shares no build state with other checkouts. Configure and build it
   fresh (`./tools/configure.sh && ./tools/build.sh`). The Jenkins reservation
   `.myenv` venv is also per-checkout and absent in a new worktree; when you need
   it, drive the lockable-resources pool with the curl flow above instead.

## Guard against regressions

Every change must be checked for regressions, not just for the new behavior.

- After a change, re-run the relevant board tests and confirm previously passing
  tests still pass.
- When fixing a bug or adding behavior, prefer running the surrounding suite on
  the board rather than only the single affected test.
- If a board is unavailable, say so explicitly and flag that regression coverage
  is incomplete — never quietly ship untested.
