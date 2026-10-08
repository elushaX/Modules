"""End-to-end check of the board tools over a real MCP stdio session (run: uv run python test_board_tools.py)."""
import asyncio
import json
import os
import sys
import tempfile

import sessions
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

HERE = os.path.dirname(os.path.abspath(__file__))


def read_exit_fifo(path):
    with open(path) as pipe:
        return pipe.readline().strip()


async def check_tree_order(session):
    root = await call(session, "create_task", title="Order root", status="queued")
    first = await call(session, "create_task", title="First", parent_id=root["id"], status="ready")
    second = await call(session, "create_subtask", parent_id=root["id"], title="Second")
    assert first["status"] == "queued" and second["status"] == "queued"
    assert second["order_index"] == first["order_index"] + 1

    ready = [t["id"] for t in await call(session, "list_ready")]
    assert ready.index(root["id"]) < ready.index(first["id"]) < ready.index(second["id"])

    await call(session, "edit_task", task_id=second["id"], changes={"order_index": -1},
               expected={"order_index": second["order_index"]})
    ready = [t["id"] for t in await call(session, "list_ready")]
    assert ready.index(second["id"]) < ready.index(first["id"])


async def check_global_order_and_statuses(session):
    one = await call(session, "create_task", title="Rank one")
    other_parent = await call(session, "create_task", title="Rank parent")
    nested = await call(session, "create_task", title="Rank nested", parent_id=other_parent["id"])
    assert one["order_index"] < other_parent["order_index"] < nested["order_index"]

    wip = await call(session, "set_status", task_id=one["id"], status="running")
    assert wip["status"] == "wip"
    queued = await call(session, "edit_task", task_id=one["id"], changes={"status": "queued"}, expected={})
    assert queued["status"] == "queued"
    for old, current in (("ready", "queued"), ("failed", "blocked")):
        renamed = await call(session, "set_status", task_id=one["id"], status=old)
        assert renamed["status"] == current
    created = await call(session, "create_task", title="Old name", status="failed")
    assert created["status"] == "blocked"
    assert "error" in await call(session, "create_task", title="Bogus", status="bogus")
    await call(session, "fail_task", task_id=created["id"], reason="stuck")
    assert (await call(session, "get_task", task_id=created["id"]))["status"] == "blocked"
    saved = await call(session, "save_board", edits=[{"task_id": one["id"], "changes": {"status": "running"}}])
    assert not saved.get("conflicts")
    board = await call(session, "get_board")
    assert next(t for t in board["tasks"] if t["id"] == one["id"])["status"] == "wip"
    assert "error" in await call(session, "save_board", edits=[{"task_id": one["id"], "changes": {"status": "bogus"}}])


def tmux_on(socket, *args):
    import subprocess
    return subprocess.run(["tmux", "-L", socket, *args], capture_output=True, text=True)


def session_alive(socket, task_id):
    return tmux_on(socket, "has-session", "-t", f"=task-{task_id}").returncode == 0


async def check_deleting_stops_sessions(session, socket):
    parent = await call(session, "create_task", title="Doomed parent")
    child = await call(session, "create_task", title="Doomed child", parent_id=parent["id"])
    board_deleted = await call(session, "create_task", title="Deleted from the board")
    for task in (parent, child, board_deleted):
        tmux_on(socket, "new-session", "-d", "-s", f"task-{task['id']}", "sleep 600")
        assert session_alive(socket, task["id"])

    await call(session, "delete_task", task_id=parent["id"])
    assert not session_alive(socket, parent["id"])
    assert not session_alive(socket, child["id"])

    await call(session, "save_board", deleted=[board_deleted["id"]])
    assert not session_alive(socket, board_deleted["id"])


async def call(session, name, **arguments):
    result = await session.call_tool(name, arguments)
    if result.isError:
        return {"error": result.content[0].text}
    if result.structuredContent is not None:
        return result.structuredContent.get("result", result.structuredContent)
    return json.loads(result.content[0].text) if result.content else None


async def check_context_chain(session, sessions_root):
    root = await call(session, "create_task", title="Context root", description="root guidelines")
    middle = await call(session, "create_task", title="Context middle", parent_id=root["id"])
    leaf = await call(session, "create_task", title="Context leaf", parent_id=middle["id"])

    reports = os.path.join(sessions_root, f"task-{root['id']}", "reports")
    os.makedirs(reports, exist_ok=True)
    with open(os.path.join(reports, "summary.md"), "w") as handle:
        handle.write("Follow the root guideline.")

    context = await call(session, "get_context", task_id=leaf["id"])
    assert context["self"]["id"] == leaf["id"]
    assert [a["id"] for a in context["ancestors"]] == [root["id"], middle["id"]]
    assert context["ancestors"][0]["docs"]["summary"] == "Follow the root guideline."
    assert "docs" not in context["ancestors"][1]

    nearest = await call(session, "get_context", task_id=leaf["id"], max_depth=1)
    assert [a["id"] for a in nearest["ancestors"]] == [middle["id"]]

    just_root = await call(session, "get_context", task_id=root["id"], max_depth=0)
    assert just_root["ancestors"] == []
    assert just_root["self"]["docs"]["summary"] == "Follow the root guideline."


def task_in(board, task_id):
    return next((task for task in board["tasks"] if task["id"] == task_id), None)


async def check_board_history(session):
    parent = await call(session, "create_task", title="History parent")
    child = await call(session, "create_task", title="History child", parent_id=parent["id"])
    await call(session, "add_dependency", task_id=child["id"], depends_on_id=parent["id"])
    created = (await call(session, "get_board"))["history"]["last"]

    await call(session, "edit_task", task_id=child["id"], changes={"title": "Renamed", "status": "done"}, expected={})
    edited = (await call(session, "get_board"))["history"]["last"]
    await call(session, "delete_task", task_id=parent["id"])

    live = await call(session, "get_board")
    assert task_in(live, parent["id"]) is None and task_in(live, child["id"]) is None

    before_edit = await call(session, "get_board", at=created)
    assert task_in(before_edit, child["id"])["title"] == "History child"
    assert task_in(before_edit, child["id"])["status"] == "backlog"
    assert task_in(before_edit, child["id"])["parent_id"] == parent["id"]
    assert {"task_id": child["id"], "depends_on_id": parent["id"]} in before_edit["dependencies"]

    after_edit = await call(session, "get_board", at=edited)
    assert task_in(after_edit, child["id"])["title"] == "Renamed"
    assert task_in(after_edit, child["id"])["status"] == "done"
    assert task_in(await call(session, "get_board", at=created - 3600), parent["id"]) is None

    moments = await call(session, "get_history_moments")
    assert moments == sorted(moments) and created in moments and edited in moments


async def main():
    with tempfile.TemporaryDirectory() as directory:
        exit_fifo = os.path.join(directory, "exit.fifo")
        os.mkfifo(exit_fifo)
        environment = {**os.environ, "TASKS_DB": os.path.join(directory, "tasks.db"), "AGENTS_EXIT_FIFO": exit_fifo,
                       "AGENTS_TMUX_SOCKET": f"agents-test-{os.getpid()}",
                       "AGENTS_SESSIONS_DIR": os.path.join(directory, "sessions"), "FAKE_DELAY": "0.2"}
        server = StdioServerParameters(command=sys.executable, args=[os.path.join(HERE, "server.py")], env=environment)
        async with stdio_client(server) as (read, write), ClientSession(read, write) as session:
            await session.initialize()

            first = await call(session, "create_task", title="A")
            second = await call(session, "create_task", title="B")
            assert first["id"] == 1 and second["id"] == 2

            assert "error" not in await call(session, "set_parent", task_id=2, parent_id=1)
            assert "cycle" in (await call(session, "set_parent", task_id=1, parent_id=2))["error"]
            assert "cycle" in (await call(session, "set_parent", task_id=1, parent_id=1))["error"]

            revision_before = await call(session, "get_board_revision")
            assert await call(session, "set_layout", task_id=1, x=10, y=20, width=240, body_height=80, collapsed=True) == "ok"
            assert await call(session, "set_color", task_id=1, color="#ff8800") == "ok"
            assert await call(session, "set_palette", colors=["#ff0000", "#00ff00"]) == "ok"
            assert await call(session, "get_board_revision") != revision_before

            before_rename = await call(session, "get_board_revision")
            await call(session, "update_task", task_id=2, title="B2")
            after_rename = await call(session, "get_board_revision")
            assert after_rename != before_rename
            await call(session, "update_task", task_id=2, title="B3")
            assert await call(session, "get_board_revision") != after_rename

            board = await call(session, "get_board")
            task = next(t for t in board["tasks"] if t["id"] == 1)
            assert (task["x"], task["y"], task["width"], task["body_height"], task["collapsed"]) == (10, 20, 240, 80, 1)
            assert task["color"] == "#ff8800"
            assert next(t for t in board["tasks"] if t["id"] == 2)["parent_id"] == 1
            assert board["palette"] == ["#ff0000", "#00ff00"]

            await call(session, "add_dependency", task_id=2, depends_on_id=1)
            board = await call(session, "get_board")
            assert board["dependencies"] == [{"task_id": 2, "depends_on_id": 1}]

            assert "error" not in await call(session, "set_parent", task_id=2)
            assert await call(session, "set_color", task_id=1) == "ok"
            board = await call(session, "get_board")
            assert next(t for t in board["tasks"] if t["id"] == 2)["parent_id"] is None
            assert next(t for t in board["tasks"] if t["id"] == 1)["color"] is None

            fresh = await call(session, "edit_task", task_id=1, changes={"title": "Mine"}, expected={"title": "A"})
            assert fresh["title"] == "Mine"
            conflict = await call(session, "edit_task", task_id=1, changes={"title": "Lost"}, expected={"title": "A"})
            assert "conflict:" in conflict["error"]
            details = json.loads(conflict["error"].split("conflict: ", 1)[1])
            assert details == {"task_id": 1, "current": {"title": "Mine"}}
            board = await call(session, "get_board")
            assert next(t for t in board["tasks"] if t["id"] == 1)["title"] == "Mine"

            other = await call(session, "edit_task", task_id=1, changes={"order_index": 5}, expected={"order_index": 0})
            assert other["order_index"] == 5 and other["title"] == "Mine"
            assert "error" in await call(session, "edit_task", task_id=1, changes={"priority": 1}, expected={})
            assert "error" in await call(session, "edit_task", task_id=1, changes={"parent_id": 2}, expected={})
            assert "error" in await call(session, "edit_task", task_id=1, changes={"status": "bogus"}, expected={})
            cleared = await call(session, "edit_task", task_id=1, changes={"color": None}, expected={"color": None})
            assert cleared["color"] is None

            await call(session, "update_task", task_id=1, title="Agent")
            history = await call(session, "get_field_history", task_id=1, field="title")
            assert [h["old_value"] for h in history] == ["Mine", "A"]
            assert [h["source"] for h in history] == ["update_task", "taskboard"]

            options = await call(session, "get_options")
            assert "claude-opus-5-5" in [m["id"] for m in options["models"]]
            assert "deepseek-chat" in [m["id"] for m in options["models"]]
            assert [h["id"] for h in options["harnesses"]] == ["claude-code", "dsh"]
            # A DeepSeek model needs the dsh harness; claude-code must reject it.
            assert "error" in await call(session, "edit_task", task_id=1,
                                         changes={"agent_model": "deepseek-chat", "harness": "claude-code"}, expected={})
            assert "error" not in await call(session, "edit_task", task_id=1,
                                             changes={"agent_model": "deepseek-chat", "harness": "dsh"}, expected={})
            assert "error" not in await call(session, "edit_task", task_id=1,
                                             changes={"agent_model": "claude-opus-5-5", "harness": "claude-code"}, expected={})
            assert options["efforts"] == ["default", "low", "medium", "high", "xhigh", "max"]
            assert options["permission_modes"] == ["auto", "manual"]
            assert "error" in await call(session, "edit_task", task_id=1, changes={"effort": "huge"}, expected={})
            assert "error" in await call(session, "edit_task", task_id=1, changes={"permission_mode": "yolo"}, expected={})
            edited = await call(session, "edit_task", task_id=1,
                                changes={"agent_model": "claude-opus-5-5", "effort": "xhigh", "permission_mode": "manual"},
                                expected={})
            assert (edited["agent_model"], edited["effort"], edited["permission_mode"]) == ("claude-opus-5-5", "xhigh", "manual")

            fake = await call(session, "create_task", title="Fake", agent_model="echo")
            started = await call(session, "start_session", task_id=fake["id"])
            assert started["directory"].endswith(f"sessions/task-{fake['id']}")
            workspace = await call(session, "get_workspace", task_id=fake["id"])
            assert set(workspace["folders"]) == {"reports", "notes", "tmp", "artifacts"}

            exit_note = await asyncio.wait_for(asyncio.to_thread(read_exit_fifo, exit_fifo), timeout=60)
            assert exit_note == f"{fake['id']} 0"
            status = await call(session, "session_status", task_id=fake["id"])
            assert status["state"] == "ended" and not status["alive"]
            finished = await call(session, "get_task", task_id=fake["id"])
            assert finished["status"] == "done"
            assert finished["workspace"] == started["directory"]
            workspace = await call(session, "get_workspace", task_id=fake["id"])
            assert "reports/summary.md" in [f["path"] for f in workspace["files"]]
            assert "error" in await call(session, "send_session_message", task_id=fake["id"], message="hi")

            saved = await call(session, "save_board",
                               created=[{"ref": -1, "title": "Saved child", "x": 5, "y": 6, "width": 220,
                                         "body_height": 90, "collapsed": False}],
                               edits=[{"task_id": 2, "changes": {"title": "Batch"}, "expected": {"title": "B3"}},
                                      {"task_id": 1, "changes": {"title": "Lose"}, "expected": {"title": "nope"}}],
                               links_added=[{"kind": "parent", "from": 2, "to": -1},
                                            {"kind": "dependency", "from": 1, "to": -1}],
                               palette=["#112233"])
            child = saved["ids"]["-1"]
            assert [c["field"] for c in saved["conflicts"]] == ["title"]
            board = await call(session, "get_board")
            by_id = {t["id"]: t for t in board["tasks"]}
            assert by_id[2]["title"] == "Batch" and by_id[1]["title"] == "Agent"
            assert by_id[child]["parent_id"] == 2 and by_id[child]["x"] == 5
            assert {"task_id": child, "depends_on_id": 1} in board["dependencies"]
            assert board["palette"] == ["#112233"]

            cyclic = await call(session, "save_board", links_added=[{"kind": "parent", "from": child, "to": 2}],
                                edits=[{"task_id": 2, "changes": {"title": "Rolled back"}, "expected": {"title": "Batch"}}])
            assert "cycle" in cyclic["error"]
            assert next(t for t in (await call(session, "get_board"))["tasks"] if t["id"] == 2)["title"] == "Batch"

            removed = await call(session, "save_board", links_removed=[{"kind": "dependency", "from": 1, "to": child}],
                                 deleted=[child])
            assert removed["conflicts"] == []
            assert child not in [t["id"] for t in (await call(session, "get_board"))["tasks"]]

            assert finished["status"] == "done"
            status = await call(session, "session_status", task_id=fake["id"])
            assert status["launched_with"] == {"agent_model": "echo", "harness": "claude-code", "effort": None, "permission_mode": "auto"}
            board = await call(session, "get_board")
            assert json.loads(next(t for t in board["tasks"] if t["id"] == fake["id"])["session_launch"])["agent_model"] == "echo"

            await call(session, "edit_task", task_id=fake["id"], changes={"effort": "max"}, expected={})
            await call(session, "restart_session", task_id=fake["id"], fresh=True)
            assert await asyncio.wait_for(asyncio.to_thread(read_exit_fifo, exit_fifo), timeout=60) == f"{fake['id']} 0"
            status = await call(session, "session_status", task_id=fake["id"])
            assert status["launched_with"]["effort"] == "max"


            await check_deleting_stops_sessions(session, environment["AGENTS_TMUX_SOCKET"])

            await check_tree_order(session)
            await check_global_order_and_statuses(session)
            await check_board_history(session)

            await check_context_chain(session, environment["AGENTS_SESSIONS_DIR"])

    print("board tools: all checks passed")


asyncio.run(main())
