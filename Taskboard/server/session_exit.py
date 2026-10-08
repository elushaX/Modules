"""Runs in the task's tmux session after the agent process exits: records why it ended."""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from sessions import exit_marker_path, last_exit_reason, set_state  # noqa: E402


def main():
    task_id, status = int(sys.argv[1]), sys.argv[2]
    reason = last_exit_reason(task_id)
    detail = f"exited with code {status}" + (f": {reason}" if reason else "")
    set_state(task_id, "ended", "finished" if status == "0" and not reason else detail)
    with open(exit_marker_path(task_id), "w") as marker:
        marker.write(status)
    notify_waiter(os.environ.get("AGENTS_EXIT_FIFO"), task_id, status)


def notify_waiter(fifo, task_id, status):
    """Tests block on a FIFO instead of polling; nothing listens in normal use."""
    if not fifo or not os.path.exists(fifo):
        return
    with open(fifo, "w") as pipe:
        pipe.write(f"{task_id} {status}\n")


if __name__ == "__main__":
    main()
