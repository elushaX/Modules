import { createRequire } from "node:module";

const require = createRequire(import.meta.url);

export const BOARD_STATUSES = ["wip", "daemon", "review", "done"];
export const BOARD_HARNESS = "dsh";

export class BoardDatabase {
	constructor(file) {
		this.file = file;
	}

	read(query) {
		const { DatabaseSync } = require("node:sqlite");
		const db = new DatabaseSync(this.file, { readOnly: true });
		try {
			return query(db);
		} finally {
			db.close();
		}
	}

	task(id) {
		return this.read((db) =>
			db.prepare("SELECT id, title, agent_model, effort FROM tasks WHERE id = ?").get(Number(id)) ?? null);
	}

	revision() {
		return this.read((db) => {
			const row = db.prepare("SELECT value FROM board_state WHERE key = 'revision'").get();
			return row ? String(row.value) : "0";
		});
	}

	dshTasksByStatus() {
		const placeholders = BOARD_STATUSES.map(() => "?").join(", ");
		const rows = this.read((db) => db.prepare(
			`SELECT id, title, status, session_state, parent_id, updated_at
			   FROM tasks
			  WHERE harness = ? AND status IN (${placeholders})
			  ORDER BY updated_at DESC, id DESC`,
		).all(BOARD_HARNESS, ...BOARD_STATUSES));

		const groups = Object.fromEntries(BOARD_STATUSES.map((status) => [status, []]));
		for (const row of rows) {
			groups[row.status].push({
				id: row.id,
				title: row.title,
				sessionState: row.session_state,
				parentId: row.parent_id,
				updatedAt: row.updated_at,
			});
		}
		return groups;
	}
}
