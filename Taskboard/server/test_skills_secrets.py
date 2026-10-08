"""End-to-end check of the skill and secret tools over a real MCP stdio session.

Run: uv run python test_skills_secrets.py
"""
import asyncio
import json
import os
import sys
import tempfile

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

HERE = os.path.dirname(os.path.abspath(__file__))


async def call(session, tool, **arguments):
    result = await session.call_tool(tool, arguments)
    if result.isError:
        return {"error": result.content[0].text}
    if result.structuredContent is not None:
        return result.structuredContent.get("result", result.structuredContent)
    return json.loads(result.content[0].text) if result.content else None


def seed_skill(skills_dir, name, title, description, body):
    folder = os.path.join(skills_dir, name)
    os.makedirs(folder, exist_ok=True)
    front = f"---\nname: {title}\ndescription: {description}\n---\n\n{body}\n"
    with open(os.path.join(folder, "SKILL.md"), "w", encoding="utf-8") as handle:
        handle.write(front)


async def check_skills(session):
    listed = await call(session, "list_skills")
    by_name = {s["name"]: s for s in listed}
    assert set(by_name) == {"elvees", "generic"}, by_name
    assert by_name["elvees"]["title"] == "Elvees access"
    assert "GitLab" in by_name["elvees"]["description"]

    full = await call(session, "get_skill", name="elvees")
    assert full["name"] == "elvees" and full["title"] == "Elvees access"
    assert "fetch it with get_secret" in full["body"]
    assert full["truncated"] is False

    assert "error" in await call(session, "get_skill", name="missing")
    assert "error" in await call(session, "get_skill", name="../paths")


async def check_secrets(session):
    assert await call(session, "list_secrets") == []

    created = await call(session, "set_secret", key="elvees.gitlab_token", value="glpat-xyz",
                         description="Elvees GitLab PAT")
    assert created == {"key": "elvees.gitlab_token", "description": "Elvees GitLab PAT"}

    listed = await call(session, "list_secrets")
    assert [s["key"] for s in listed] == ["elvees.gitlab_token"]
    assert listed[0]["description"] == "Elvees GitLab PAT"
    assert "value" not in listed[0]

    got = await call(session, "get_secret", key="elvees.gitlab_token")
    assert got == {"key": "elvees.gitlab_token", "value": "glpat-xyz"}

    # Updating the value keeps the description when it is omitted.
    await call(session, "set_secret", key="elvees.gitlab_token", value="glpat-new")
    assert (await call(session, "get_secret", key="elvees.gitlab_token"))["value"] == "glpat-new"
    assert (await call(session, "list_secrets"))[0]["description"] == "Elvees GitLab PAT"

    assert "error" in await call(session, "get_secret", key="absent")
    assert "error" in await call(session, "set_secret", key="bad key", value="x")

    deleted = await call(session, "delete_secret", key="elvees.gitlab_token")
    assert deleted == {"key": "elvees.gitlab_token", "deleted": True}
    assert await call(session, "delete_secret", key="elvees.gitlab_token") == {
        "key": "elvees.gitlab_token", "deleted": False}
    assert await call(session, "list_secrets") == []


async def check_secrets_stay_out_of_the_board(session, tasks_db, secrets_db):
    await call(session, "create_task", title="A task that forces tasks.db to exist")
    await call(session, "set_secret", key="only.in.secrets", value="supersecret")
    assert os.path.exists(secrets_db) and os.path.exists(tasks_db)
    with open(tasks_db, "rb") as handle:
        assert b"supersecret" not in handle.read()
    await call(session, "delete_secret", key="only.in.secrets")


async def main():
    with tempfile.TemporaryDirectory() as directory:
        skills_dir = os.path.join(directory, "skills")
        os.makedirs(skills_dir)
        seed_skill(skills_dir, "elvees", "Elvees access",
                   "GitLab and board access for Elvees work.",
                   "Needs the elvees.gitlab_token secret; fetch it with get_secret.")
        seed_skill(skills_dir, "generic", "Generic conventions",
                   "Task-agnostic coding conventions.", "Keep functions small.")

        tasks_db = os.path.join(directory, "tasks.db")
        secrets_db = os.path.join(directory, "secrets.db")
        environment = {**os.environ, "TASKS_DB": tasks_db, "AGENTS_SECRETS_DB": secrets_db,
                       "AGENTS_SKILLS_DIR": skills_dir, "AGENTS_TMUX_SOCKET": f"agents-test-{os.getpid()}",
                       "AGENTS_SESSIONS_DIR": os.path.join(directory, "sessions")}
        server = StdioServerParameters(command=sys.executable, args=[os.path.join(HERE, "server.py")], env=environment)
        async with stdio_client(server) as (read, write), ClientSession(read, write) as session:
            await session.initialize()
            await check_skills(session)
            await check_secrets(session)
            await check_secrets_stay_out_of_the_board(session, tasks_db, secrets_db)

    print("skills and secrets tools: all checks passed")


asyncio.run(main())
