"""Seed an example task graph by calling the MCP tool functions directly."""
import server as s

root = s.create_task("Build website", "Static site with blog", agent_model="echo")
design = s.create_task("Design layout", parent_id=root["id"], agent_model="echo", status="queued")
content = s.create_task("Write posts", parent_id=root["id"], agent_model="echo", status="queued")
deploy = s.create_task("Deploy", parent_id=root["id"], agent_model="echo", status="queued")
s.add_dependency(deploy["id"], design["id"])
s.add_dependency(deploy["id"], content["id"])
print("ready now:", [t["title"] for t in s.list_ready()])
