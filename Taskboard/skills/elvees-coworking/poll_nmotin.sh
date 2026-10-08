#!/usr/bin/env bash
# Background poller for the Squadus (Rocket.Chat) DM with Nikolay Motin (nmotin).
# Reads the personal access token from the elvees-programming skill (single
# source of truth — do not duplicate the secret here). Polls the DM history and
# exits as soon as a new message from nmotin (after $1, an ISO ts baseline)
# appears, printing it. Run it with the harness's run_in_background so you are
# notified on exit; then read the new message, respond as the user, and relaunch
# with the new baseline.
#
# usage: poll_nmotin.sh <iso-ts-baseline>   e.g. 2026-08-19T09:58:46.000Z

set -u
SQ='https://im-squadus.elvees.com'
SKILL="/home/auser/.claude/skills/elvees-programming/SKILL.md"
TOK=$(grep -oP 'X-Auth-Token:\K[A-Za-z0-9]+' "$SKILL" | head -1)
UID_ME=$(grep -oP 'X-User-Id:\K[A-Za-z0-9]+' "$SKILL" | head -1)
BASE="${1:?need an ISO ts baseline, e.g. 2026-08-19T09:58:46.000Z}"
[ -n "$TOK" ] || { echo "no Squadus token in $SKILL"; exit 3; }

for i in $(seq 1 80); do          # ~40 min of coverage at 30s cadence
  sleep 30
  curl -s --max-time 15 -H "X-Auth-Token: $TOK" -H "X-User-Id: $UID_ME" \
    "$SQ/api/v1/im.history?username=nmotin&count=15" -o /tmp/sq_hist.json 2>/dev/null
  NEW=$(UID_ME="$UID_ME" BASE="$BASE" python3 -c '
import json, os
base = os.environ["BASE"]; me = os.environ["UID_ME"]
try:
    d = json.load(open("/tmp/sq_hist.json"))
except Exception:
    raise SystemExit
ms = [m for m in d.get("messages", [])
      if m.get("u", {}).get("_id") != me and m.get("ts", "") > base]
for m in sorted(ms, key=lambda x: x.get("ts", "")):
    print("[" + m.get("ts", "") + "] " + m.get("u", {}).get("username", "?") + ": " + m.get("msg", ""))
' 2>/dev/null)
  if [ -n "$NEW" ]; then
    echo "=== NEW FROM NMOTIN ==="; echo "$NEW"; exit 0
  fi
done
echo "=== poll window elapsed (~40 min), no new message ==="; exit 0
