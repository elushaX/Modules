#include "McpClient.hpp"
#include "TaskBoard.hpp"
#include "TaskServerSync.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <map>
#include <string>

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

namespace {
	std::string managerDirectory() { return TASKBOARD_SERVER_DIR; }

	bool serverAvailable() { return std::filesystem::exists(managerDirectory() + "/server.py"); }

	struct ServerScene {
		std::filesystem::path directory;
		McpClient client;
		TaskBoard board;
		std::optional<TaskServerSync> sync;

		ServerScene() :
			directory(std::filesystem::temp_directory_path() / ("taskboard-test-" + std::to_string(::getpid()))) {
			std::filesystem::create_directories(directory);

			McpServerCommand command;
			command.executable = "uv";
			command.arguments = { "run", "--project", managerDirectory(), "python", managerDirectory() + "/server.py" };
			command.environment = { { "AGENTS_DATA_DIR", directory.string() },
															{ "TASKS_DB", (directory / "tasks.db").string() },
															{ "AGENTS_SESSIONS_DIR", (directory / "sessions").string() },
															{ "AGENTS_TMUX_SOCKET", "taskboard-test-" + std::to_string(::getpid()) },
															{ "FAKE_DELAY", "0.2" } };
			command.stderrPath = (directory / "server.log").string();

			std::string error;
			REQUIRE CHECK(client.start(command, error));
			sync.emplace(board, client);
		}

		~ServerScene() {
			sync.reset();
			client.stop();
			std::system(("tmux -L taskboard-test-" + std::to_string(::getpid()) + " kill-server 2>/dev/null").c_str());
			std::filesystem::remove_all(directory);
		}

		McpToolResult tool(const std::string& name, picojson::object arguments = {}) {
			return client.callToolBlocking(name, std::move(arguments));
		}

		NodeId createOnServer(const std::string& title) {
			picojson::object arguments;
			arguments["title"] = picojson::value(title);
			return NodeId(tool("create_task", arguments).value.get<picojson::object>().at("id").get<double>());
		}

		picojson::object serverTask(NodeId id) {
			picojson::object arguments;
			arguments["task_id"] = picojson::value(double(id));
			return tool("get_task", arguments).value.get<picojson::object>();
		}

		void settle() { client.waitForIdle(); }

		void saveAndSettle() {
			sync->save();
			settle();
		}

		void pollAndSettle() {
			sync->pollForChanges();
			settle();
		}

		void serverEdit(NodeId id, const std::string& field, picojson::value value) {
			picojson::object arguments;
			arguments["task_id"] = picojson::value(double(id));
			arguments[field] = std::move(value);
			REQUIRE CHECK(tool("update_task", arguments).ok);
		}

		double lastHistoryMoment() {
			return tool("get_board")
				.value.get<picojson::object>()
				.at("history")
				.get<picojson::object>()
				.at("last")
				.get<double>();
		}

		void deleteOnServer(NodeId id) {
			picojson::object arguments;
			arguments["task_id"] = picojson::value(double(id));
			REQUIRE CHECK(tool("delete_task", arguments).ok);
		}

		size_t serverTaskCount() {
			return tool("get_board").value.get<picojson::object>().at("tasks").get<picojson::array>().size();
		}
	};
}

SUITE(TaskColorText) {
	TEST(HexRoundTrip) {
		const RGBA color(0.2f, 0.6f, 1.0f, 1.0f);
		const std::optional<RGBA> parsed = colorFromHex(colorToHex(color));
		REQUIRE CHECK(parsed.has_value());
		CHECK_EQUAL("#3399ff", colorToHex(*parsed));
		CHECK(!colorFromHex("red").has_value());
		CHECK(!colorFromHex("#12345").has_value());
	}

	TEST(StatusNamesRoundTrip) {
		for (TaskStatus status : kTaskStatuses)
			CHECK(statusFromName(statusName(status)) == status);
		CHECK(!statusFromName("nope").has_value());
		CHECK(statusFromName("running") == TaskStatus::Wip);
		CHECK(statusFromName("ready") == TaskStatus::Queued);
		CHECK(statusFromName("failed") == TaskStatus::Blocked);
		CHECK_EQUAL(7u, kTaskStatuses.size());
		CHECK_EQUAL("wip", std::string(statusName(TaskStatus::Wip)));
		CHECK_EQUAL("queued", std::string(statusName(TaskStatus::Queued)));
		CHECK_EQUAL("WIP", std::string(statusLabel(TaskStatus::Wip)));
		CHECK_EQUAL("daemon", std::string(statusName(TaskStatus::Daemon)));
		CHECK_EQUAL("Daemon", std::string(statusLabel(TaskStatus::Daemon)));
	}
}

SUITE(BoardSnapshotDiff) {
	TEST(IdenticalBoardsHaveNoChanges) {
		TaskBoard board;
		board.addTask({ .title = "A" }, { 0, 0 });
		const BoardSnapshot snapshot = BoardSnapshot::capture(board);
		CHECK(BoardChanges::between(snapshot, snapshot).empty());
	}

	TEST(EveryKindOfEditIsDetectedOnce) {
		TaskBoard board;
		const NodeId kept = board.addTask({ .title = "Kept" }, { 0, 0 });
		const NodeId removed = board.addTask({ .title = "Removed" }, { 300, 0 });
		const NodeId other = board.addTask({ .title = "Other" }, { 600, 0 });
		board.addDependency(other, kept);
		const BoardSnapshot saved = BoardSnapshot::capture(board);

		board.task(kept)->title = "Renamed";
		board.task(kept)->effort = "max";
		board.graph().moveNode(kept, Vec2F(10.0f, 10.0f));
		board.graph().removeNode(removed);
		const NodeId created = board.addTask({ .title = "New" }, { 0, 300 });
		board.setParent(created, kept);
		board.graph().disconnectPort(TaskBoard::input(other, TaskPort::DependsOn));
		board.setColorPalette({ RGBA(1, 0, 0, 1) });

		const BoardChanges changes = BoardChanges::between(saved, BoardSnapshot::capture(board));
		CHECK(changes.created == std::vector<NodeId>{ created });
		CHECK(changes.deleted == std::vector<NodeId>{ removed });
		REQUIRE CHECK_EQUAL(2u, changes.edits.size());
		CHECK(changes.edits[0].field == TaskField::Title);
		CHECK_EQUAL("Kept", changes.edits[0].expected.to_str());
		CHECK(changes.edits[1].field == TaskField::Effort);
		CHECK(changes.layouts == std::vector<NodeId>{ kept });
		REQUIRE CHECK_EQUAL(1u, changes.linksAdded.size());
		CHECK(changes.linksAdded[0].kind == LinkKind::Parent);
		REQUIRE CHECK_EQUAL(1u, changes.linksRemoved.size());
		CHECK(changes.linksRemoved[0].kind == LinkKind::Dependency);
		CHECK(changes.palette.has_value());
		CHECK_EQUAL(8u, changes.count());
	}

	TEST(UndoingAnEditMakesItSavedAgain) {
		TaskBoard board;
		const NodeId id = board.addTask({ .title = "A" }, { 0, 0 });
		const BoardSnapshot saved = BoardSnapshot::capture(board);

		board.task(id)->title = "B";
		CHECK_EQUAL(1u, BoardChanges::between(saved, BoardSnapshot::capture(board)).count());
		board.task(id)->title = "A";
		CHECK(BoardChanges::between(saved, BoardSnapshot::capture(board)).empty());
	}

	TEST(LinksOfDeletedTasksAreNotReportedSeparately) {
		TaskBoard board;
		const NodeId parent = board.addTask({}, { 0, 0 });
		const NodeId child = board.addTask({}, { 300, 0 });
		board.setParent(child, parent);
		const BoardSnapshot saved = BoardSnapshot::capture(board);

		board.graph().removeNode(child);
		const BoardChanges changes = BoardChanges::between(saved, BoardSnapshot::capture(board));
		CHECK_EQUAL(1u, changes.deleted.size());
		CHECK(changes.linksRemoved.empty());
	}
}

SUITE(TaskServerSync) {
	TEST(LoadsEachTaskWorkspaceDirectory) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId id = scene.createOnServer("Agent work");

		REQUIRE CHECK(scene.sync->load());
		const std::string expected = (scene.directory / "sessions" / ("task-" + std::to_string(id))).string();
		CHECK_EQUAL(expected, scene.board.task(id)->workspace);
		CHECK(scene.sync->unsavedChanges().empty());
	}

	TEST(LoadsTasksHierarchyDependenciesAndLayout) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId parent = scene.createOnServer("Parent");
		const NodeId child = scene.createOnServer("Child");

		picojson::object link;
		link["task_id"] = picojson::value(double(child));
		link["parent_id"] = picojson::value(double(parent));
		scene.tool("set_parent", link);
		picojson::object dependency;
		dependency["task_id"] = picojson::value(double(child));
		dependency["depends_on_id"] = picojson::value(double(parent));
		scene.tool("add_dependency", dependency);
		picojson::object layout;
		layout["task_id"] = picojson::value(double(parent));
		layout["x"] = picojson::value(120.0);
		layout["y"] = picojson::value(80.0);
		layout["width"] = picojson::value(260.0);
		layout["body_height"] = picojson::value(70.0);
		layout["collapsed"] = picojson::value(true);
		scene.tool("set_layout", layout);

		REQUIRE CHECK(scene.sync->load());
		REQUIRE CHECK(scene.board.task(parent) != nullptr);
		CHECK_EQUAL("Child", scene.board.task(child)->title);
		CHECK(scene.board.parentOf(child) == parent);
		CHECK_EQUAL(1u, scene.board.dependenciesOf(child).size());
		CHECK(!scene.sync->hasUnsavedChanges());

		const Node& node = *scene.board.graph().findNode(parent);
		CHECK(node.position == Vec2F(120.0f, 80.0f));
		CHECK_CLOSE(260, node.width, 0.01f);
		CHECK(node.collapsed);
	}

	TEST(EditsStayLocalUntilSave) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId id = scene.createOnServer("Before");
		REQUIRE CHECK(scene.sync->load());

		Task& task = *scene.board.task(id);
		task.title = "After";
		task.description = "line one\nline two";
		task.status = TaskStatus::Queued;
		task.color = RGBA(1, 0, 0, 1);
		task.effort = "max";
		scene.board.graph().moveNode(id, Vec2F(300.0f, 200.0f));
		scene.settle();

		CHECK_EQUAL("Before", scene.serverTask(id).at("title").to_str());
		CHECK_EQUAL(6u, scene.sync->unsavedChanges().count());

		scene.saveAndSettle();
		const picojson::object stored = scene.serverTask(id);
		CHECK_EQUAL("After", stored.at("title").to_str());
		CHECK_EQUAL("line one\nline two", stored.at("description").to_str());
		CHECK_EQUAL("queued", stored.at("status").to_str());
		CHECK_EQUAL("#ff0000", stored.at("color").to_str());
		CHECK_EQUAL("max", stored.at("effort").to_str());
		CHECK_CLOSE(300, stored.at("x").get<double>(), 0.01);
		CHECK(!scene.sync->hasUnsavedChanges());
	}

	TEST(PollingDoesNotWriteAnything) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId id = scene.createOnServer("Server");
		REQUIRE CHECK(scene.sync->load());

		scene.board.task(id)->title = "Local only";
		for (int round = 0; round < 3; round++)
			scene.pollAndSettle();

		CHECK_EQUAL("Server", scene.serverTask(id).at("title").to_str());
		CHECK_EQUAL("Local only", scene.board.task(id)->title);
		CHECK_EQUAL(1u, scene.sync->unsavedChanges().count());
	}

	TEST(NewTasksLinksAndDeletesSaveAtomicallyWithServerIds) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId parent = scene.createOnServer("Parent");
		const NodeId doomed = scene.createOnServer("Doomed");
		REQUIRE CHECK(scene.sync->load());

		const NodeId child = scene.board.addTask({ .title = "Child" }, { 50, 300 });
		scene.board.setParent(child, parent);
		const NodeId grandchild = scene.board.addTask({ .title = "Grandchild" }, { 50, 600 });
		scene.board.setParent(grandchild, child);
		scene.board.addDependency(grandchild, parent);
		scene.board.graph().removeNode(doomed);
		CHECK_EQUAL(2u, scene.serverTaskCount());

		scene.saveAndSettle();
		CHECK(!scene.sync->hasUnsavedChanges());
		CHECK_EQUAL(3u, scene.serverTaskCount());

		const picojson::object board = scene.tool("get_board").value.get<picojson::object>();
		std::map<std::string, picojson::object> byTitle;
		for (const picojson::value& row : board.at("tasks").get<picojson::array>())
			byTitle[row.get<picojson::object>().at("title").to_str()] = row.get<picojson::object>();
		const double childId = byTitle.at("Child").at("id").get<double>();
		CHECK_EQUAL(double(parent), byTitle.at("Child").at("parent_id").get<double>());
		CHECK_EQUAL(childId, byTitle.at("Grandchild").at("parent_id").get<double>());
		CHECK_CLOSE(600, byTitle.at("Grandchild").at("y").get<double>(), 0.01);
		CHECK_EQUAL(1u, board.at("dependencies").get<picojson::array>().size());

		REQUIRE CHECK(scene.board.task(NodeId(childId)) != nullptr);
		CHECK(scene.board.parentOf(NodeId(childId)) == parent);
		CHECK_EQUAL(3u, scene.board.graph().nodes().size());
	}

	TEST(FailedSaveWritesNothingAndKeepsWork) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId id = scene.createOnServer("Task");
		REQUIRE CHECK(scene.sync->load());

		std::string reported;
		scene.sync->setOnError([&](const std::string& error) { reported = error; });
		scene.board.task(id)->title = "Edited";
		scene.board.task(id)->agentModel = "gpt-who-knows";
		scene.saveAndSettle();

		CHECK(reported.find("nothing was written") != std::string::npos);
		CHECK_EQUAL("Task", scene.serverTask(id).at("title").to_str());
		CHECK_EQUAL("Edited", scene.board.task(id)->title);
		CHECK_EQUAL(2u, scene.sync->unsavedChanges().count());
	}

	TEST(AgentChangesMergeIntoUnsavedBoard) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId mine = scene.createOnServer("Mine");
		const NodeId theirs = scene.createOnServer("Theirs");
		REQUIRE CHECK(scene.sync->load());

		scene.board.task(mine)->description = "my unsaved text";
		scene.serverEdit(theirs, "description", picojson::value("agent text"));
		scene.serverEdit(mine, "title", picojson::value("Agent renamed mine"));
		scene.createOnServer("Agent subtask");
		scene.pollAndSettle();

		CHECK_EQUAL("my unsaved text", scene.board.task(mine)->description);
		CHECK_EQUAL("Agent renamed mine", scene.board.task(mine)->title);
		CHECK_EQUAL("agent text", scene.board.task(theirs)->description);
		CHECK_EQUAL(3u, scene.board.graph().nodes().size());
		CHECK_EQUAL(1u, scene.sync->unsavedChanges().count());
		CHECK_EQUAL(0u, scene.sync->conflicts().size());

		scene.saveAndSettle();
		const picojson::object stored = scene.serverTask(mine);
		CHECK_EQUAL("my unsaved text", stored.at("description").to_str());
		CHECK_EQUAL("Agent renamed mine", stored.at("title").to_str());
	}

	TEST(SameFieldChangedOnBothSidesIsAConflictNotALoss) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId id = scene.createOnServer("T");
		REQUIRE CHECK(scene.sync->load());

		scene.board.task(id)->description = "mine";
		scene.serverEdit(id, "description", picojson::value("theirs"));
		scene.pollAndSettle();

		REQUIRE CHECK_EQUAL(1u, scene.sync->conflicts().size());
		const FieldConflict conflict = scene.sync->conflicts().front();
		CHECK_EQUAL("mine", conflict.mine.to_str());
		CHECK_EQUAL("theirs", conflict.theirs.to_str());
		CHECK_EQUAL("mine", scene.board.task(id)->description);

		scene.sync->resolveConflict(conflict, true);
		CHECK(scene.sync->hasUnsavedChanges());
		scene.saveAndSettle();
		CHECK_EQUAL("mine", scene.serverTask(id).at("description").to_str());

		picojson::object history;
		history["task_id"] = picojson::value(double(id));
		history["field"] = picojson::value("description");
		bool theirsPreserved = false;
		const McpToolResult kept = scene.tool("get_field_history", history);
		for (const picojson::value& entry : kept.value.get<picojson::array>())
			theirsPreserved |= entry.get<picojson::object>().at("old_value").to_str() == "theirs";
		CHECK(theirsPreserved);
	}

	TEST(UseTheirsDiscardsLocalChange) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId id = scene.createOnServer("T");
		REQUIRE CHECK(scene.sync->load());

		scene.board.task(id)->title = "mine";
		scene.serverEdit(id, "title", picojson::value("theirs"));
		scene.pollAndSettle();
		REQUIRE CHECK_EQUAL(1u, scene.sync->conflicts().size());

		scene.sync->resolveConflict(scene.sync->conflicts().front(), false);
		CHECK_EQUAL("theirs", scene.board.task(id)->title);
		CHECK(!scene.sync->hasUnsavedChanges());
	}

	TEST(SaveRacingAnAgentEditBecomesConflict) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId id = scene.createOnServer("Original");
		REQUIRE CHECK(scene.sync->load());

		scene.serverEdit(id, "title", picojson::value("Agent title"));
		scene.board.task(id)->title = "My title";
		scene.board.task(id)->order = 7;
		scene.saveAndSettle();

		const picojson::object stored = scene.serverTask(id);
		CHECK_EQUAL("Agent title", stored.at("title").to_str());
		CHECK_EQUAL(7, int(stored.at("order_index").get<double>()));
		REQUIRE CHECK_EQUAL(1u, scene.sync->conflicts().size());
		CHECK(scene.sync->conflicts().front().field == TaskField::Title);
		CHECK_EQUAL("My title", scene.board.task(id)->title);
	}

	TEST(TaskDeletedByAgentDisappearsUnlessEditedLocally) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId untouched = scene.createOnServer("Untouched");
		REQUIRE CHECK(scene.sync->load());

		picojson::object arguments;
		arguments["task_id"] = picojson::value(double(untouched));
		scene.tool("delete_task", arguments);
		scene.pollAndSettle();
		CHECK(scene.board.task(untouched) == nullptr);
		CHECK(!scene.sync->hasUnsavedChanges());
	}

	TEST(SessionStateReachesBoardAndUnsavedTaskCannotStart) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId id = scene.createOnServer("Agent");
		scene.serverEdit(id, "agent_model", picojson::value("echo"));
		REQUIRE CHECK(scene.sync->load());

		std::string reported;
		scene.sync->setOnError([&](const std::string& error) { reported = error; });
		const NodeId unsaved = scene.board.addTask({}, { 0, 0 });
		scene.sync->startSession(unsaved);
		CHECK(reported.find("save the task") != std::string::npos);

		scene.sync->startSession(id);
		scene.settle();
		CHECK(scene.board.task(id)->session.state != SessionState::None);
		CHECK(scene.board.task(unsaved) != nullptr);
	}

	TEST(ViewingAPastMomentShowsDeletedTasksAndEarlierValues) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId kept = scene.createOnServer("Kept");
		const NodeId removed = scene.createOnServer("Removed later");
		const double before = scene.lastHistoryMoment();
		scene.serverEdit(kept, "title", picojson::value("Renamed later"));
		scene.deleteOnServer(removed);
		REQUIRE CHECK(scene.sync->load());
		REQUIRE CHECK(scene.sync->historySpan().last.has_value());
		CHECK(scene.board.task(removed) == nullptr);

		REQUIRE CHECK(scene.sync->viewHistoryAt(before));
		scene.settle();
		CHECK(scene.sync->isViewingHistory());
		REQUIRE CHECK(scene.board.task(removed) != nullptr);
		CHECK_EQUAL("Kept", scene.board.task(kept)->title);
		CHECK(!scene.sync->hasUnsavedChanges());

		REQUIRE CHECK(scene.sync->viewHistoryAt(std::nullopt));
		scene.settle();
		CHECK(!scene.sync->isViewingHistory());
		CHECK(scene.board.task(removed) == nullptr);
		CHECK_EQUAL("Renamed later", scene.board.task(kept)->title);
	}

	TEST(PastBoardIsReadOnlyAndBrowsingNeedsSavedWork) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId id = scene.createOnServer("Original");
		const double created = scene.lastHistoryMoment();
		REQUIRE CHECK(scene.sync->load());

		std::string reported;
		scene.sync->setOnError([&](const std::string& error) { reported = error; });
		scene.board.task(id)->title = "Unsaved";
		CHECK(!scene.sync->viewHistoryAt(created));
		CHECK(reported.find("save or discard") != std::string::npos);
		CHECK(!scene.sync->isViewingHistory());

		scene.saveAndSettle();
		REQUIRE CHECK(scene.sync->viewHistoryAt(created));
		scene.settle();
		CHECK_EQUAL("Original", scene.board.task(id)->title);

		scene.board.task(id)->title = "Edited in the past";
		scene.sync->save();
		scene.sync->startSession(id);
		scene.settle();
		CHECK(reported.find("return to Live") != std::string::npos);
		CHECK_EQUAL("Unsaved", scene.serverTask(id).at("title").to_str());

		scene.pollAndSettle();
		CHECK_EQUAL("Edited in the past", scene.board.task(id)->title);
	}

	TEST(MovingThroughHistoryNeverEmptiesTheBoardOnTheWay) {
		if (!serverAvailable()) return;
		ServerScene scene;
		const NodeId kept = scene.createOnServer("Kept");
		const double first = scene.lastHistoryMoment();
		scene.serverEdit(kept, "title", picojson::value("Renamed"));
		const double second = scene.lastHistoryMoment();
		REQUIRE CHECK(scene.sync->load());

		size_t smallest = scene.board.graph().nodes().size();
		scene.board.graph().setOnChanged([&] { smallest = std::min(smallest, scene.board.graph().nodes().size()); });
		for (double moment : { first, second, first }) {
			REQUIRE CHECK(scene.sync->viewHistoryAt(moment));
			scene.settle();
		}
		REQUIRE CHECK(scene.sync->viewHistoryAt(std::nullopt));
		scene.settle();

		CHECK_EQUAL(1u, smallest);
		CHECK_EQUAL("Renamed", scene.board.task(kept)->title);
	}
}
