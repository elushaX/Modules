export const HANDOFF_GRACE_MS = 3000;

export function taskTabPath(taskId) {
	return taskId ? `/?tbTask=${encodeURIComponent(taskId)}` : "/";
}

export function openPage({ taskId, launchToken, handoff }) {
	const target = JSON.stringify(taskTabPath(taskId));
	const token = JSON.stringify(launchToken || "");
	const delay = handoff ? HANDOFF_GRACE_MS : 0;
	const label = taskId ? `task #${taskId}` : "dsh";
	return `<!doctype html>
<html><head><meta charset="utf-8"><title>Opening ${label}</title>
<style>html,body{margin:0;height:100%;background:#1e1e1e;color:#888;font:14px sans-serif;display:grid;place-items:center}</style>
</head><body>Opening ${label}…
<script>
(async () => {
	const token = ${token};
	if (token) await fetch("/?token=" + encodeURIComponent(token), { credentials: "same-origin" }).catch(() => {});
	setTimeout(() => location.replace(${target}), ${delay});
})();
</script></body></html>`;
}
