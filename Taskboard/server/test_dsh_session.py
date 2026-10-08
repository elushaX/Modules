"""DeepSeek Harness wiring (run: uv run python test_dsh_session.py).

A dsh task is not a board-owned process: it runs on your shared dsh web server and the board records a
deep link carrying the task id (the taskboard-deeplink plugin resolves it). This checks the model/harness
split, the deep-link URL, and that starting/stopping a dsh task updates state without launching tmux."""
import os
import sys
import tempfile


def main():
    directory = tempfile.mkdtemp()
    os.environ["TASKS_DB"] = os.path.join(directory, "tasks.db")
    os.environ["AGENTS_SESSIONS_DIR"] = os.path.join(directory, "sessions")
    os.environ["AGENTS_TMUX_SOCKET"] = f"dsh-unit-{os.getpid()}"
    os.environ["AGENTS_DSH_WEB_PORT"] = "3080"
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import db as dbmod
    import sessions

    # Provider derivation + compatibility (claude-code is Claude-only; dsh runs either).
    assert sessions.provider_for_model("claude-opus-5-5") == "claude"
    assert sessions.provider_for_model("deepseek-chat") == "deepseek-official"
    assert set(sessions.HARNESSES) == {"claude-code", "dsh"}
    sessions.validate_model_harness("claude-opus-5-5", "claude-code")
    sessions.validate_model_harness("deepseek-chat", "dsh")
    for bad in (lambda: sessions.validate_model_harness("deepseek-chat", "claude-code"),
                lambda: sessions.validate_harness("nope"),
                lambda: sessions.model_argument("gpt-9")):
        try:
            bad()
            assert False, "expected ValueError"
        except ValueError:
            pass

    # Deep-link URL carries the task id and the shared-server port.
    assert sessions.dsh_deeplink_url(55) == "http://127.0.0.1:3080/plugins/taskboard-deeplink/open?id=55"

    with dbmod.connect() as conn:
        conn.execute("INSERT INTO tasks(id, title, agent_model, harness, status) "
                     "VALUES (7, 'dsh task', 'claude-opus-5-5', 'dsh', 'queued')")

    # Starting a dsh task records the link and marks it wip — without any tmux session.
    started = sessions.start(7)
    assert started["alive"] is True and started["state"] == "working"
    assert started["detail"] == "http://127.0.0.1:3080/plugins/taskboard-deeplink/open?id=7"
    assert not sessions.is_alive(7), "a dsh task must not create a tmux session"
    with dbmod.connect() as conn:
        assert conn.execute("SELECT status FROM tasks WHERE id=7").fetchone()["status"] == "wip"

    # Stopping ends the link; reconcile leaves dsh tasks alone (no process to vanish).
    assert sessions.stop(7)["state"] == "ended"
    sessions.start(7)
    sessions.reconcile()
    assert sessions.describe(7)["alive"] is True, "reconcile must not end a dsh link"

    print("dsh session: all checks passed")


if __name__ == "__main__":
    main()
