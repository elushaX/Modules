const REQUEST_TIMEOUT_MS = 10000;
const FOCUS_TIMEOUT_MS = 5000;
const FOCUS_POLL_MS = 50;
const SESSION_END_TIMEOUT_MS = 500;

class BidiSession {
	constructor(socket) {
		this.socket = socket;
		this.nextId = 1;
		this.pending = new Map();
		socket.addEventListener("message", (event) => this.receive(JSON.parse(event.data)));
	}

	static connect(port) {
		return new Promise((resolve, reject) => {
			const socket = new WebSocket(`ws://127.0.0.1:${port}/session`);
			socket.addEventListener("open", () => resolve(new BidiSession(socket)), { once: true });
			socket.addEventListener("error", () => reject(new Error(`no Firefox remote agent on port ${port}`)), { once: true });
		});
	}

	receive(message) {
		const waiter = this.pending.get(message.id);
		if (!waiter) return;
		this.pending.delete(message.id);
		if (message.type === "error") waiter.reject(new Error(`${message.error}: ${message.message}`));
		else waiter.resolve(message.result);
	}

	send(method, params = {}) {
		const id = this.nextId++;
		this.socket.send(JSON.stringify({ id, method, params }));
		return new Promise((resolve, reject) => {
			this.pending.set(id, { resolve, reject });
			setTimeout(() => {
				if (!this.pending.delete(id)) return;
				reject(new Error(`${method} timed out`));
			}, REQUEST_TIMEOUT_MS);
		});
	}

	async close() {
		const ended = this.send("session.end").catch(() => {});
		await Promise.race([ended, sleep(SESSION_END_TIMEOUT_MS)]);
		this.socket.close();
	}
}

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

async function tabHasFocus(session, context) {
	const reply = await session.send("script.evaluate", {
		expression: "document.hasFocus()",
		target: { context },
		awaitPromise: false,
	});
	return reply.result && reply.result.value === true;
}

async function selectTab(session, context) {
	session.send("browsingContext.activate", { context }).catch(() => {});
	const deadline = Date.now() + FOCUS_TIMEOUT_MS;
	while (Date.now() < deadline) {
		if (await tabHasFocus(session, context).catch(() => false)) return;
		await sleep(FOCUS_POLL_MS);
	}
	throw new Error("the tab did not become selected");
}

function taskOfUrl(url, origin) {
	const parsed = URL.canParse(url) ? new URL(url) : null;
	if (!parsed || parsed.origin !== origin || parsed.pathname !== "/") return null;
	return parsed.searchParams.get("tbTask") ?? "";
}

function isHandoffUrl(url, origin) {
	const parsed = URL.canParse(url) ? new URL(url) : null;
	return !!parsed && parsed.origin === origin && parsed.searchParams.has("handoff");
}

async function closeHandoffTabs(session, origin) {
	const deadline = Date.now() + 3000;
	while (Date.now() < deadline) {
		const handoffs = (await topLevelTabs(session)).filter((tab) => isHandoffUrl(tab.url, origin));
		if (handoffs.length) {
			for (const tab of handoffs) await session.send("browsingContext.close", { context: tab.context });
			return handoffs.length;
		}
		await sleep(FOCUS_POLL_MS);
	}
	return 0;
}

async function topLevelTabs(session) {
	const tree = await session.send("browsingContext.getTree", { maxDepth: 0 });
	return tree.contexts;
}

async function openTaskTab(session, url, task) {
	const origin = new URL(url).origin;
	const tabs = await topLevelTabs(session);
	const existing = tabs.find((tab) => taskOfUrl(tab.url, origin) === task);
	if (existing) {
		await selectTab(session, existing.context);
		return { action: "focused", context: existing.context };
	}

	const reusable = tabs.find((tab) => tab.url === "about:blank" || tab.url === "about:newtab");
	const context = reusable ? reusable.context : (await session.send("browsingContext.create", { type: "tab", background: true })).context;
	await session.send("browsingContext.navigate", { context, url, wait: "none" });
	await selectTab(session, context);
	return { action: reusable ? "reused-blank" : "created", context };
}

async function main() {
	const [command, port, url, task] = process.argv.slice(2);
	const session = await BidiSession.connect(port);
	try {
		await session.send("session.new", { capabilities: {} });
		const origin = new URL(url).origin;
		const result = command === "close-handoff"
			? { closed: await closeHandoffTabs(session, origin) }
			: await openTaskTab(session, url, task ?? "");
		console.log(JSON.stringify(result));
	} finally {
		await session.close();
	}
}

main().then(
	() => process.exit(0),
	(error) => {
		console.error(`task-tabs: ${error.message}`);
		process.exit(1);
	},
);
