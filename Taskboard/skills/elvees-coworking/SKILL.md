---
name: elvees-coworking
description: Background message-watcher job for the Squadus DM with Nikolay Motin (nmotin) while sharing the Elvees radio stand. Use when the user asks to poll/watch nmotin for new messages, or to keep a running watch on the coordination thread.
---

# Elvees coworking — nmotin message watcher

Coordination with **nmotin** over Squadus happens while sharing the Elvees radio
stand. The Squadus token, the send/read API, and the coordination rules live in
[[elvees-programming]]; the stand topology is in [[elvees-radio-stand]]. This
skill holds just the **background polling job** that watches the DM for new
replies.

## The poll job

`poll_nmotin.sh <iso-ts-baseline>` (in this skill dir) watches the DM and exits
as soon as nmotin sends a message newer than the baseline, printing it. It reads
the Squadus token from [[elvees-programming]] at runtime (no secret duplicated
here).

Run it with the harness **run_in_background** so you are notified when it exits:

    bash /home/auser/.claude/skills/elvees-coworking/poll_nmotin.sh 2026-08-19T09:58:46.000Z

Loop: after sending or handling a message, note its `ts`; launch the poller with
that `ts` as the baseline (a hair past it — e.g. `...:46.000Z` for a `...:45.9Z`
message — so the same message isn't re-caught); on notification, read the new
message and handle it; then relaunch with the newest `ts` to keep watching. Each
run self-limits to ~40 min; relaunch to continue.

`/tmp` is volatile across relogins, but this script lives in the skill dir, so it
persists. If you ever need it elsewhere, copy it from here.
