import os from "node:os";
import path from "node:path";
import fs from "node:fs";
import { execFile } from "node:child_process";
import { fileURLToPath } from "node:url";
import { BoardDatabase } from "./board-db.mjs";
import { openPage } from "./open-page.mjs";
import { TaskSessionRegistry } from "./task-sessions.mjs";

const ROUTE_PREFIX = "/plugins/taskboard-deeplink";
const TASK_PATH = `${ROUTE_PREFIX}/task`;
const TASK_SESSION_PATH = `${ROUTE_PREFIX}/task-session`;
const OPEN_PATH = `${ROUTE_PREFIX}/open`;
const BOARD_PATH = `${ROUTE_PREFIX}/board`;
const OPEN_TASK_PATH = `${ROUTE_PREFIX}/open-task`;
const HOME_PATH = `${ROUTE_PREFIX}/home`;
const DSH_APP = fileURLToPath(new URL("../dsh-app", import.meta.url));
const OPEN_TASK_TIMEOUT_MS = 15000;
const DEBUG_PATH = `${ROUTE_PREFIX}/debug`;
const DEBUG_FILE = path.join(os.tmpdir(), "taskboard-deeplink-debug.log");
const name = "taskboard-deeplink";
const inject = ["webServer"];

const SESSIONS_DIR = process.env.AGENTS_SESSIONS_DIR
	|| path.join(os.homedir(), "agents", "manager", "sessions");
const TASKS_DB = process.env.TASKS_DB || path.join(os.homedir(), "agents", "manager", "tasks.db");
const REGISTRY_FILE = process.env.TASKBOARD_DEEPLINK_REGISTRY
	|| path.join(process.env.DSH_HOME || path.join(os.homedir(), ".dsh"), "task-board", "deeplink-sessions.json");

const CLAUDE_ALIASES = { opus: "claude-opus-5-5", sonnet: "claude-sonnet-4-6", haiku: "claude-haiku-4-5-20251001" };
const MODEL_ALIASES = { "claude-haiku-4-5": "claude-haiku-4-5-20251001" };

const registry = new TaskSessionRegistry(REGISTRY_FILE);
const board = new BoardDatabase(TASKS_DB);

function providerForModel(model) {
	return model && model.startsWith("deepseek") ? "deepseek-official" : "claude";
}

function modelIdFor(model) {
	if (!model) return model;
	if (model.startsWith("claude:")) model = CLAUDE_ALIASES[model.slice(7)] || model;
	return MODEL_ALIASES[model] || model;
}

function sessionTitle(id, row) {
	return row && row.title ? `#${id} ${row.title}` : `#${id}`;
}

function sessionModel(row) {
	if (!row || !row.agent_model) return {};

	const model = { provider: providerForModel(row.agent_model), model: modelIdFor(row.agent_model) };
	if (row.effort && row.effort !== "default") model.reasoningEffort = row.effort;
	return model;
}

function taskDetails(id) {
	try {
		const row = board.task(id);
		return { title: sessionTitle(id, row), ...sessionModel(row) };
	} catch (error) {
		return { title: sessionTitle(id, null), modelError: String(error && error.message || error) };
	}
}

function workerPrompt(id) {
	return `You are the agent for task #${id} on the task-manager board (MCP server "task-manager").
Start with get_task(${id}) to read the task, its parent and dependency results.
Your task workspace is the current directory; get_workspace(${id}) describes it. Keep your files there:
  reports/    findings and final write-ups (finish with reports/summary.md)
  notes/      running notes and plans
  tmp/        scratch and intermediate data, safe to delete
  artifacts/  outputs other tasks or the human should use
get_workspace(${id}) also lists your parent's and children's workspace directories — sibling folders you can
read directly to pick up their work (and they can read yours).
Report progress with add_note, split work with create_subtask.
Finish with complete_task(${id}, result=<summary>) or fail_task(${id}, reason=..., blocked=true if you need the human).
The human may type into this session at any time: follow their instructions.`;
}

function taskInfo(id) {
	const cwd = path.join(SESSIONS_DIR, `task-${id}`);
	fs.mkdirSync(cwd, { recursive: true });
	return { task: String(id), cwd, prompt: workerPrompt(id), sessionId: registry.sessionOf(id), ...taskDetails(id) };
}

function queryOf(req) {
	return new URL(req.url ?? "/", "http://dsh.internal").searchParams;
}

function taskIdFrom(req) {
	const id = queryOf(req).get("id") ?? "";
	return /^\d+$/.test(id) ? id : null;
}

function sendJson(res, status, body) {
	res.writeHead(status, { "content-type": "application/json; charset=utf-8", "cache-control": "no-store" });
	res.end(JSON.stringify(body));
}

async function readBody(req) {
	let body = "";
	for await (const chunk of req) body += chunk;
	return body;
}

async function handleTask(req, res) {
	const id = taskIdFrom(req);
	if (!id) return sendJson(res, 400, { error: "missing or invalid task id" });
	sendJson(res, 200, taskInfo(id));
}

async function handleTaskSession(req, res) {
	if (req.method !== "POST") return sendJson(res, 405, { error: "POST {task, sessionId}" });

	const { task, sessionId } = JSON.parse(await readBody(req));
	if (!/^\d+$/.test(String(task)) || typeof sessionId !== "string" || !sessionId) {
		return sendJson(res, 400, { error: "task and sessionId are required" });
	}
	registry.remember(String(task), sessionId);
	sendJson(res, 200, { task: String(task), sessionId });
}

function boardRevision(sessions) {
	return `${board.revision()}:${Object.keys(sessions).length}:${registry.modifiedMs()}`;
}

async function handleBoard(req, res) {
	const known = queryOf(req).get("since");
	const sessions = registry.all();
	const revision = boardRevision(sessions);
	if (known === revision) return sendJson(res, 200, { revision, unchanged: true });

	const groups = board.dshTasksByStatus();
	for (const tasks of Object.values(groups)) {
		for (const task of tasks) task.sessionId = sessions[String(task.id)] ?? null;
	}
	sendJson(res, 200, { revision, groups, taskSessions: taskOfSession(sessions) });
}

function taskOfSession(sessions) {
	return Object.fromEntries(Object.entries(sessions).map(([task, sessionId]) => [sessionId, task]));
}

function servingPort(req) {
	return String(req.socket.localPort);
}

function runDshApp(task, port) {
	const environment = { ...process.env, DSH_APP_PORT: port };
	return new Promise((resolve) => {
		execFile(DSH_APP, ["--task", task], { timeout: OPEN_TASK_TIMEOUT_MS, env: environment }, (error, _stdout, stderr) => {
			resolve(error ? { ok: false, error: String(stderr || error.message).trim() } : { ok: true });
		});
	});
}

async function handleOpenTask(req, res) {
	if (req.method !== "POST") return sendJson(res, 405, { error: "POST {task}" });

	const { task } = JSON.parse(await readBody(req));
	if (!/^\d+$/.test(String(task))) return sendJson(res, 400, { error: "task id is required" });
	const result = await runDshApp(String(task), servingPort(req));
	sendJson(res, result.ok ? 200 : 502, result);
}

async function handleOpen(req, res) {
	const query = queryOf(req);
	const id = query.get("id");
	if (id !== null && !/^\d+$/.test(id)) return sendJson(res, 400, { error: "invalid task id" });

	res.writeHead(200, { "content-type": "text/html; charset=utf-8", "cache-control": "no-store" });
	res.end(openPage({ taskId: id, launchToken: query.get("token"), handoff: query.has("handoff") }));
}

async function handleHome(req, res) {
	sendJson(res, 200, { home: os.homedir() });
}

async function handleDebug(req, res) {
	if (req.method === "POST") {
		fs.appendFileSync(DEBUG_FILE, `${new Date().toISOString()} ${await readBody(req)}\n`);
		res.writeHead(204);
		res.end();
		return;
	}
	res.writeHead(200, { "content-type": "text/plain; charset=utf-8" });
	res.end(fs.existsSync(DEBUG_FILE) ? fs.readFileSync(DEBUG_FILE, "utf8") : "");
}

function guarded(handler) {
	return async (req, res) => {
		try {
			await handler(req, res);
		} catch (error) {
			sendJson(res, 500, { error: error instanceof Error ? error.message : String(error) });
		}
	};
}

const ROUTES = [
	[TASK_PATH, handleTask],
	[TASK_SESSION_PATH, handleTaskSession],
	[OPEN_PATH, handleOpen],
	[BOARD_PATH, handleBoard],
	[OPEN_TASK_PATH, handleOpenTask],
	[HOME_PATH, handleHome],
	[DEBUG_PATH, handleDebug],
];

function apply(ctx) {
	ctx.effect(() => {
		const disposers = ROUTES.map(([routePath, handler]) =>
			ctx.webServer.register({ kind: "exact", path: routePath, handler: guarded(handler) }));
		return () => disposers.forEach((dispose) => dispose());
	}, "taskboard-deeplink: routes");
}

export { TASK_PATH, apply, inject, name };
