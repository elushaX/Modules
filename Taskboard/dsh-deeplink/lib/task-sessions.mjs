import fs from "node:fs";
import path from "node:path";

export class TaskSessionRegistry {
	constructor(file) {
		this.file = file;
	}

	sessionOf(task) {
		return this.read()[String(task)] ?? null;
	}

	all() {
		return this.read();
	}

	modifiedMs() {
		return fs.existsSync(this.file) ? Math.trunc(fs.statSync(this.file).mtimeMs) : 0;
	}

	remember(task, sessionId) {
		const sessions = this.read();
		sessions[String(task)] = sessionId;
		this.write(sessions);
	}

	read() {
		if (!fs.existsSync(this.file)) return {};
		return JSON.parse(fs.readFileSync(this.file, "utf8"));
	}

	write(sessions) {
		fs.mkdirSync(path.dirname(this.file), { recursive: true });
		const staging = `${this.file}.${process.pid}.tmp`;
		fs.writeFileSync(staging, JSON.stringify(sessions, null, "\t"));
		fs.renameSync(staging, this.file);
	}
}
