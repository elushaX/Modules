#include "DemoBoard.hpp"

using namespace tp;
using namespace tp::taskboard;

void tp::taskboard::populateDemoBoard(TaskBoard& board) {
	const ui::NodeId website =
		board.addTask({ "Build website", "Static site with a blog", TaskStatus::Wip, 1, "claude:opus" }, { 40, 220 });
	const ui::NodeId design =
		board.addTask({ "Design layout", "Pick a grid and typography", TaskStatus::Done, 1, "claude:sonnet" }, { 360, 40 });
	const ui::NodeId posts =
		board.addTask({ "Write posts", "Three introductory posts", TaskStatus::Review, 2, "claude:haiku" }, { 360, 260 });
	const ui::NodeId deploy =
		board.addTask({ "Deploy", "Publish to the home server", TaskStatus::Queued, 1, "codex" }, { 700, 150 });

	board.setParent(design, website);
	board.setParent(posts, website);
	board.setParent(deploy, website);
	board.addDependency(deploy, design);
	board.addDependency(deploy, posts);
}
