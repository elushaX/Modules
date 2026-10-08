"""Claude Code hook: record the task session's state. Invoked as `session_hook.py <HookEvent>` with JSON on stdin."""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from sessions import HOOK_EVENTS, set_state  # noqa: E402


def detail_for(event, payload):
    if event in ("PreToolUse", "PostToolUse"):
        return f"tool: {payload.get('tool_name', '?')}"
    if event == "Notification":
        return payload.get("message") or "needs your input"
    if event == "PermissionRequest":
        return f"permission: {payload.get('tool_name', '?')}"
    if event == "StopFailure":
        error = payload.get("error") or payload.get("error_details") or payload.get("message") or "the turn failed"
        return str(error)[:300]
    if event == "SessionEnd":
        return f"ended: {payload.get('reason', 'exit')}"
    return None


def main():
    event = sys.argv[1] if len(sys.argv) > 1 else ""
    task_id = os.environ.get("TASK_ID")
    if event not in HOOK_EVENTS or not task_id:
        return

    try:
        payload = json.load(sys.stdin)
    except (json.JSONDecodeError, ValueError):
        payload = {}
    set_state(int(task_id), HOOK_EVENTS[event], detail_for(event, payload))


if __name__ == "__main__":
    main()
