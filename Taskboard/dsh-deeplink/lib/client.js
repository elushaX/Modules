window.__ModuleLoader__.load({
	id: "taskboard-deeplink",
	factory: (require) => {
		const module = { exports: {} };
		const react = require("react");
		const element = react.createElement;

		const LOG = "[taskboard-deeplink]";
		const ROUTE_PREFIX = "/plugins/taskboard-deeplink";
		const LIST_READY_MS = 15000;
		const STEP_TIMEOUT_MS = 8000;
		const state = (window.__taskboardDeeplink ||= { opening: {} });

		function debug(...parts) {
			const message = parts.map((part) => (typeof part === "string" ? part : safeJson(part))).join(" ");
			console.log(LOG, message);
			fetch(`${ROUTE_PREFIX}/debug`, { method: "POST", body: message, keepalive: true }).catch(() => {});
		}

		function safeJson(value) {
			try {
				return JSON.stringify(value);
			} catch {
				return String(value);
			}
		}

		function errorText(error) {
			return String((error && (error.stack || error.message)) || error);
		}

		const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

		function withTimeout(promise, ms, label) {
			return Promise.race([
				Promise.resolve(promise),
				new Promise((_, reject) => setTimeout(() => reject(new Error("timeout " + label)), ms)),
			]);
		}

		function taskOfThisTab() {
			return new URLSearchParams(window.location.search).get("tbTask");
		}

		async function fetchTaskInfo(taskId) {
			const response = await fetch(`${ROUTE_PREFIX}/task?id=${encodeURIComponent(taskId)}`, {
				headers: { accept: "application/json" },
				cache: "no-store",
			});
			if (!response.ok) throw new Error(`task route ${response.status}`);
			return response.json();
		}

		async function rememberTaskSession(taskId, sessionId) {
			const response = await fetch(`${ROUTE_PREFIX}/task-session`, {
				method: "POST",
				headers: { "content-type": "application/json" },
				body: JSON.stringify({ task: taskId, sessionId }),
			});
			if (!response.ok) throw new Error(`task-session route ${response.status}`);
		}

		function sessionList(ctx) {
			return ctx.sessions.list.getSnapshot();
		}

		async function waitForSessionList(ctx) {
			const deadline = Date.now() + LIST_READY_MS;
			let refreshed = false;
			while (Date.now() < deadline) {
				const list = sessionList(ctx);
				if (list && list.phase === "ready") return list;
				if (!refreshed) {
					refreshed = true;
					ctx.sessions.refresh().catch(() => {});
				}
				await sleep(150);
			}
			throw new Error("session list did not become ready");
		}

		function isArchived(ctx, sessionId) {
			const workspaces = ctx.workspaces && ctx.workspaces.list.getSnapshot();
			return !!workspaces && (workspaces.archivedSessionIds || []).includes(sessionId);
		}

		function isListed(list, sessionId) {
			return !!sessionId && list.ids.includes(sessionId);
		}

		function newestSessionInDirectory(ctx, list, cwd) {
			const candidates = list.ids
				.map((id) => list.byId[id])
				.filter((summary) => summary && summary.cwd === cwd && !summary.parentId && summary.origin !== "subagent")
				.filter((summary) => !isArchived(ctx, summary.id));
			candidates.sort((left, right) => (right.updatedAt || 0) - (left.updatedAt || 0));
			return candidates.length ? candidates[0].id : null;
		}

		async function waitUntilListed(ctx, sessionId) {
			const deadline = Date.now() + STEP_TIMEOUT_MS;
			while (Date.now() < deadline) {
				if (isListed(sessionList(ctx), sessionId)) return;
				await ctx.sessions.refresh().catch(() => {});
				await sleep(250);
			}
		}

		async function createWorkspaceSession(ctx, cwd) {
			const view = await withTimeout(ctx.workspaces.create({ path: cwd }), STEP_TIMEOUT_MS, "workspaces.create");
			const workspaceId = view.workspaceId;
			const sessionId = await withTimeout(ctx.uiWorkspace.connectWorkspace(workspaceId), STEP_TIMEOUT_MS, "connectWorkspace");
			await waitUntilListed(ctx, sessionId);
			return sessionId;
		}

		async function resolveTaskSession(ctx, info) {
			const list = await waitForSessionList(ctx);
			if (isListed(list, info.sessionId) && !isArchived(ctx, info.sessionId)) {
				return { sessionId: info.sessionId, origin: "registry" };
			}

			const existing = newestSessionInDirectory(ctx, list, info.cwd);
			if (existing) return { sessionId: existing, origin: "workspace" };

			return { sessionId: await createWorkspaceSession(ctx, info.cwd), origin: "created" };
		}

		async function sessionFace(ctx, sessionId) {
			const deadline = Date.now() + STEP_TIMEOUT_MS;
			while (Date.now() < deadline) {
				const face = ctx.sessions.sessionOf(ctx.sessions.scope(sessionId));
				if (face) return face;
				await sleep(200);
			}
			throw new Error("session face did not materialize");
		}

		async function selectModel(ctx, sessionId, info) {
			if (!info.model) return;
			const request = { sessionId, provider: info.provider, model: info.model };
			if (info.reasoningEffort) request.reasoningEffort = info.reasoningEffort;
			await ctx.remote.session.selectModel(request);
			debug("model set", request);
		}

		function currentTitle(ctx, sessionId) {
			const summary = sessionList(ctx).byId[sessionId];
			return summary ? summary.title : undefined;
		}

		async function nameAfterTask(ctx, sessionId, title) {
			if (currentTitle(ctx, sessionId) === title) return;
			const result = await ctx.remote.session.rename({ sessionId, title });
			if (result && result.ok === false) throw new Error(`rename failed: ${safeJson(result.error)}`);
			debug("named", { sessionId, title });
		}

		async function seedPrompt(ctx, sessionId, prompt) {
			const face = await sessionFace(ctx, sessionId);
			await ctx.conversation.sendSession(face, prompt, [], "queue");
			debug("seeded the worker prompt", { sessionId });
		}

		async function openTask(ctx, taskId) {
			if (state.opening[taskId]) return state.opening[taskId];
			state.opening[taskId] = (async () => {
				const info = await fetchTaskInfo(taskId);
				const { sessionId, origin } = await resolveTaskSession(ctx, info);
				ctx.uiWorkspace.openSession(sessionId);
				debug("opened task", { task: taskId, sessionId, origin });

				if (origin !== "registry") await rememberTaskSession(taskId, sessionId);
				await nameAfterTask(ctx, sessionId, info.title);
				if (origin === "created") {
					await selectModel(ctx, sessionId, info);
					await seedPrompt(ctx, sessionId, info.prompt);
				}
				return sessionId;
			})();
			try {
				return await state.opening[taskId];
			} catch (error) {
				debug("FAILED", { task: taskId, error: errorText(error) });
			} finally {
				delete state.opening[taskId];
			}
		}

		function keepTaskSessionShown(ctx) {
			if (state.reselecting) return;
			state.reselecting = true;
			document.addEventListener("visibilitychange", () => {
				if (document.visibilityState !== "visible" || !state.shownTaskSession) return;
				ctx.uiWorkspace.openSession(state.shownTaskSession.sessionId);
				debug("reselected", state.shownTaskSession);
			});
		}

		function markTaskOfThisTab(taskId) {
			const url = new URL(window.location.href);
			if (taskId) url.searchParams.set("tbTask", String(taskId));
			else url.searchParams.delete("tbTask");
			window.history.replaceState(window.history.state, "", url);
			window.dispatchEvent(new Event(TAB_TASK_EVENT));
		}

		async function showTaskHere(ctx, taskId) {
			if (!ctx) throw new Error("dsh is still connecting, try again in a moment");
			const sessionId = await openTask(ctx, String(taskId));
			if (!sessionId) throw new Error(`could not open task #${taskId}`);
			markTaskOfThisTab(taskId);
			state.shownTaskSession = { task: String(taskId), sessionId };
			keepTaskSessionShown(ctx);
			return sessionId;
		}

		function showOtherSessionHere(ctx, sessionId) {
			markTaskOfThisTab(null);
			state.shownTaskSession = null;
			ctx.uiWorkspace.openSession(sessionId);
		}

		const BOARD_POLL_MS = 2000;
		const BOARD_STYLE_ID = "taskboard-deeplink-style";
		const BOARD_COLLAPSED_KEY = "taskboard-deeplink:collapsed";
		const BOARD_STYLE = [
			".tbd{display:flex;flex-direction:column;height:100%;overflow:auto;font-family:var(--dsw-font-family);color:var(--dsw-alias-label-primary);font-size:13px;padding:6px 12px 24px 0}",
			".tbd__notice{margin:4px 8px;padding:6px 8px;border-radius:6px;color:var(--dsw-alias-label-tertiary);font-size:12px}",
			".tbd__group{display:flex;align-items:center;gap:6px;height:28px;padding-left:6px;border-radius:6px;cursor:pointer;user-select:none;color:var(--dsw-alias-label-secondary);font-size:11px;font-weight:600;text-transform:uppercase;letter-spacing:.05em}",
			".tbd__group:hover{background:var(--dsw-alias-interactive-bg-hover)}",
			".tbd__count{margin-left:auto;margin-right:8px;color:var(--dsw-alias-label-tertiary);font-weight:500}",
			".tbd__twist{display:inline-flex;width:12px;transition:transform .12s ease}",
			".tbd__task{display:flex;align-items:center;gap:8px;height:30px;padding:0 8px 0 24px;border-radius:6px;cursor:pointer;white-space:nowrap}",
			".tbd__task:hover{background:var(--dsw-alias-interactive-bg-hover)}",
			".tbd__task[data-active=true]{background:var(--dsw-alias-interactive-bg-active,rgba(127,127,127,.18))}",
			".tbd__id{color:var(--dsw-alias-label-tertiary);font-variant-numeric:tabular-nums}",
			".tbd__title{overflow:hidden;text-overflow:ellipsis;flex:1 1 auto;min-width:0}",
			".tbd__dot{width:7px;height:7px;border-radius:50%;flex:none;background:var(--dsw-alias-label-quaternary,#666)}",
			".tbd__dot[data-state=working],.tbd__dot[data-state=starting]{background:#3fb950}",
			".tbd__dot[data-state=waiting_input],.tbd__dot[data-state=idle]{background:#d29922}",
			".tbd__dot[data-state=error]{background:#f85149}",
			".tbd__empty{padding:4px 0 6px 30px;color:var(--dsw-alias-label-tertiary);font-size:12px}",
			".tbd__header{display:flex;align-items:center;justify-content:flex-end;padding:0 8px 6px 6px}",
			".tbd__new{display:flex;align-items:center;gap:6px;height:28px;padding:0 10px;border-radius:6px;border:1px solid var(--dsw-alias-border-primary,rgba(127,127,127,.3));background:transparent;color:var(--dsw-alias-label-primary);font:inherit;font-size:12px;cursor:pointer}",
			".tbd__new:hover{background:var(--dsw-alias-interactive-bg-hover)}",
			".tbd__archive{display:none;flex:none;align-items:center;justify-content:center;width:22px;height:22px;margin-left:4px;border:0;border-radius:5px;background:transparent;color:var(--dsw-alias-label-tertiary);cursor:pointer;padding:0}",
			".tbd__task:hover .tbd__archive{display:flex}",
			".tbd__archive:hover{background:var(--dsw-alias-interactive-bg-hover);color:var(--dsw-alias-label-primary)}",
			".tbd__where{color:var(--dsw-alias-label-tertiary);font-size:11px;margin-left:auto;padding-left:8px;overflow:hidden;text-overflow:ellipsis;max-width:45%}",
		].join("\n");
		const OTHER_GROUP = "other sessions";
		const OWN_SESSION_DIRECTORY = null;
		const TAB_TASK_EVENT = "taskboard-deeplink:tab-task";

		function injectBoardStyle() {
			if (document.getElementById(BOARD_STYLE_ID)) return;
			const style = document.createElement("style");
			style.id = BOARD_STYLE_ID;
			style.textContent = BOARD_STYLE;
			document.head.appendChild(style);
		}

		function readCollapsed() {
			try {
				return JSON.parse(window.localStorage.getItem(BOARD_COLLAPSED_KEY) || "{}");
			} catch {
				return {};
			}
		}

		async function fetchBoard(since) {
			const query = since ? `?since=${encodeURIComponent(since)}` : "";
			const response = await fetch(`${ROUTE_PREFIX}/board${query}`, { cache: "no-store" });
			if (!response.ok) throw new Error(`board route ${response.status}`);
			return response.json();
		}

		function useTaskOfThisTab() {
			const [taskId, setTaskId] = react.useState(taskOfThisTab);
			react.useEffect(() => {
				const changed = () => setTaskId(taskOfThisTab());
				window.addEventListener(TAB_TASK_EVENT, changed);
				return () => window.removeEventListener(TAB_TASK_EVENT, changed);
			}, []);
			return taskId;
		}

		async function archiveFromPanel(ctx, sessionId) {
			await ctx.uiWorkspace.archiveSession(sessionId, { stopActivity: true });
			debug("archived", { sessionId });
		}

		function useBoard() {
			const [board, setBoard] = react.useState(null);
			const [taskSessions, setTaskSessions] = react.useState({});
			const [problem, setProblem] = react.useState(null);
			const revision = react.useRef(null);

			react.useEffect(() => {
				let stopped = false;
				let timer = null;
				const poll = async () => {
					try {
						const reply = await fetchBoard(revision.current);
						if (stopped) return;
						if (!reply.unchanged) {
							revision.current = reply.revision;
							setBoard(reply.groups);
							setTaskSessions(reply.taskSessions || {});
						}
						setProblem(null);
					} catch (error) {
						if (!stopped) setProblem(String(error.message || error));
					}
					if (!stopped) timer = setTimeout(poll, BOARD_POLL_MS);
				};
				poll();
				return () => {
					stopped = true;
					clearTimeout(timer);
				};
			}, []);

			return { board, taskSessions, problem };
		}

		function useCollapsedGroups() {
			const [collapsed, setCollapsed] = react.useState(readCollapsed);
			const toggle = react.useCallback((status) => {
				setCollapsed((current) => {
					const next = { ...current, [status]: !current[status] };
					window.localStorage.setItem(BOARD_COLLAPSED_KEY, JSON.stringify(next));
					return next;
				});
			}, []);
			return [collapsed, toggle];
		}

		function twist(open) {
			return element("span", { className: "tbd__twist", style: { transform: open ? "rotate(90deg)" : "none" } },
				element("svg", { width: 12, height: 12, viewBox: "0 0 16 16", fill: "none", stroke: "currentColor", strokeWidth: 1.6 },
					element("path", { d: "M6 4l4 4-4 4" })));
		}

		function ArchiveButton({ sessionId, onArchive }) {
			if (!sessionId) return null;
			return element("button", {
				className: "tbd__archive",
				type: "button",
				title: "Archive this session",
				"aria-label": "Archive session",
				onClick: (event) => {
					event.stopPropagation();
					onArchive(sessionId);
				},
			}, element("svg", { width: 13, height: 13, viewBox: "0 0 16 16", fill: "none", stroke: "currentColor", strokeWidth: 1.4 }, [
				element("rect", { key: "lid", x: 2, y: 3, width: 12, height: 3, rx: 0.8 }),
				element("path", { key: "box", d: "M3.2 6v6.2c0 .5.4.8.8.8h8c.4 0 .8-.3.8-.8V6" }),
				element("path", { key: "slot", d: "M6.5 9h3" }),
			]));
		}

		function TaskRow({ task, active, onOpen, onArchive }) {
			return element("div", {
				className: "tbd__task",
				"data-active": active ? "true" : "false",
				title: `#${task.id} ${task.title}`,
				onClick: () => onOpen(task.id),
			}, [
				element("span", { key: "dot", className: "tbd__dot", "data-state": task.sessionState || "none" }),
				element("span", { key: "id", className: "tbd__id" }, `#${task.id}`),
				element("span", { key: "title", className: "tbd__title" }, task.title),
				element(ArchiveButton, { key: "archive", sessionId: task.sessionId, onArchive }),
			]);
		}

		function SessionRow({ session, active, onOpen, onArchive }) {
			const folder = session.cwd ? session.cwd.split("/").filter(Boolean).pop() : "";
			return element("div", {
				className: "tbd__task",
				"data-active": active ? "true" : "false",
				title: session.cwd ? `${session.displayTitle}\n${session.cwd}` : session.displayTitle,
				onClick: () => onOpen(session.id),
			}, [
				element("span", { key: "dot", className: "tbd__dot", "data-state": session.running ? "working" : "none" }),
				element("span", { key: "title", className: "tbd__title" }, session.displayTitle),
				folder && element("span", { key: "where", className: "tbd__where" }, folder),
				element(ArchiveButton, { key: "archive", sessionId: session.id, onArchive }),
			]);
		}

		function Group({ name, count, open, onToggle, empty, children }) {
			return element("div", { className: "tbd__section" }, [
				element("div", { key: "header", className: "tbd__group", onClick: () => onToggle(name) }, [
					element(react.Fragment, { key: "twist" }, twist(open)),
					element("span", { key: "label" }, name),
					element("span", { key: "count", className: "tbd__count" }, String(count)),
				]),
				open && (count ? children : element("div", { key: "empty", className: "tbd__empty" }, empty)),
			]);
		}

		function useSource(source) {
			const subscribe = react.useCallback((listener) => source.subscribe(listener), [source]);
			const snapshot = react.useCallback(() => source.getSnapshot(), [source]);
			return react.useSyncExternalStore(subscribe, snapshot);
		}

		function archivedSetOf(workspaces) {
			return new Set((workspaces && workspaces.archivedSessionIds) || []);
		}

		async function startOwnSessionIn(ctx, directory) {
			const response = await fetch(`${ROUTE_PREFIX}/home`, { cache: "no-store" });
			if (!response.ok) throw new Error(`home route ${response.status}`);
			const cwd = directory || (await response.json()).home;
			const sessionId = await createWorkspaceSession(ctx, cwd);
			ctx.uiWorkspace.openSession(sessionId);
			debug("started own session", { sessionId, cwd });
			return sessionId;
		}

		function otherSessionsOf(list, taskSessions, archived) {
			if (!list || !list.ids) return [];
			return list.ids
				.map((id) => list.byId[id])
				.filter((summary) => summary && !summary.parentId && summary.origin !== "subagent")
				.filter((summary) => !taskSessions[summary.id] && !archived.has(summary.id))
				.sort((left, right) => (right.updatedAt || 0) - (left.updatedAt || 0));
		}




		function BoardPanel() {
			injectBoardStyle();
			const ctx = state.ctx;
			const { board, taskSessions, problem } = useBoard();
			const sessions = useSource(ctx.sessions.list);
			const archived = archivedSetOf(useSource(ctx.workspaces.list));
			const [collapsed, toggle] = useCollapsedGroups();
			const [openProblem, setOpenProblem] = react.useState(null);
			const [shownSession, setShownSession] = react.useState(null);
			const activeTask = useTaskOfThisTab();

			const report = react.useCallback((promise) => {
				setOpenProblem(null);
				Promise.resolve(promise).catch((error) => setOpenProblem(String(error.message || error)));
			}, []);
			const open = react.useCallback((taskId) => {
				setShownSession(null);
				report(showTaskHere(state.taskCtx, taskId));
			}, [report]);
			const openOther = react.useCallback((sessionId) => {
				setShownSession(sessionId);
				report(showOtherSessionHere(ctx, sessionId));
			}, [ctx, report]);
			const archive = react.useCallback((sessionId) => report(archiveFromPanel(ctx, sessionId)), [ctx, report]);
			const startOwnSession = react.useCallback(() => {
				report(startOwnSessionIn(ctx, OWN_SESSION_DIRECTORY).then((sessionId) => {
					markTaskOfThisTab(null);
					state.shownTaskSession = null;
					setShownSession(sessionId);
				}));
			}, [ctx, report]);

			const others = otherSessionsOf(sessions, taskSessions, archived);
			const notice = openProblem || problem || (board ? null : "loading board…");
			return element("div", { className: "tbd" }, [
				element("div", { key: "header", className: "tbd__header" },
					element("button", { className: "tbd__new", type: "button", title: "Start a dsh session that is not a board task", onClick: startOwnSession }, "+ New session")),
				notice && element("div", { key: "notice", className: "tbd__notice" }, notice),
				board && Object.keys(board).map((status) => {
					const tasks = board[status];
					return element(Group, {
						key: status,
						name: status,
						count: tasks.length,
						open: !collapsed[status],
						onToggle: toggle,
						empty: "no tasks",
					}, tasks.map((task) => element(TaskRow, {
						key: task.id,
						task,
						active: String(task.id) === activeTask,
						onOpen: open,
						onArchive: archive,
					})));
				}),
				element(Group, {
					key: OTHER_GROUP,
					name: OTHER_GROUP,
					count: others.length,
					open: !collapsed[OTHER_GROUP],
					onToggle: toggle,
					empty: sessions && sessions.phase === "ready" ? "no other sessions" : "loading sessions…",
				}, others.map((session) => element(SessionRow, {
					key: session.id,
					session,
					active: !activeTask && session.id === shownSession,
					onOpen: openOther,
					onArchive: archive,
				}))),
			]);
		}

		function registerBoardPanel(ctx) {
			ctx.slots.inject("sidebar.workspaces", () => ctx.slots.register({
				name: "sidebar.workspaces",
				priority: -2,
				children: {},
			}, BoardPanel));
		}

		function apply(ctx) {
			if (state.applied) return;
			state.applied = true;
			ctx.inject(["workspaces", "uiWorkspace", "sessions"], (scoped) => {
				state.ctx = scoped;
				registerBoardPanel(ctx);
			});
			ctx.inject(["workspaces", "uiWorkspace", "sessions", "conversation", "remote", "remote.session"], (scoped) => {
				state.taskCtx = scoped;
				const taskId = taskOfThisTab();
				if (!taskId) return;
				showTaskHere(scoped, taskId).catch((error) => debug("FAILED", { task: taskId, error: errorText(error) }));
			});
		}

		module.exports.apply = apply;
		module.exports.inject = ["sessions", "slots"];
		return module.exports;
	},
});
