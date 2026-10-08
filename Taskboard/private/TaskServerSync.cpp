#include "TaskServerSync.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <set>

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

namespace {
	picojson::value number(double value) { return picojson::value(value); }

	double numberOr(const picojson::object& object, const char* key, double fallback) {
		auto found = object.find(key);
		return found != object.end() && found->second.is<double>() ? found->second.get<double>() : fallback;
	}

	std::string stringOr(const picojson::object& object, const char* key, const std::string& fallback) {
		auto found = object.find(key);
		return found != object.end() && found->second.is<std::string>() ? found->second.get<std::string>() : fallback;
	}

	std::optional<NodeId> idOf(const picojson::object& object, const char* key) {
		auto found = object.find(key);
		if (found == object.end() || !found->second.is<double>()) return std::nullopt;
		return NodeId(found->second.get<double>());
	}

	const picojson::array& arrayOf(const picojson::object& object, const char* key) {
		static const picojson::array empty;
		auto found = object.find(key);
		return found != object.end() && found->second.is<picojson::array>() ? found->second.get<picojson::array>() : empty;
	}

	int channel(halnf value) { return int(std::lround(std::clamp(value, 0.0f, 1.0f) * 255)); }

	std::vector<std::string> stringsOf(const picojson::object& object, const char* key) {
		std::vector<std::string> values;
		for (const picojson::value& entry : arrayOf(object, key))
			if (entry.is<std::string>()) values.push_back(entry.get<std::string>());
		return values;
	}

	std::optional<LaunchSettings> launchSettingsFrom(const std::string& json) {
		picojson::value parsed;
		if (json.empty() || !picojson::parse(parsed, json).empty() || !parsed.is<picojson::object>()) return std::nullopt;

		const picojson::object& object = parsed.get<picojson::object>();
		return LaunchSettings{ stringOr(object, "agent_model", ""),
													 stringOr(object, "harness", "claude-code"),
													 stringOr(object, "effort", ""),
													 stringOr(object, "permission_mode", "auto") };
	}

	Task taskFromRow(const picojson::object& row) {
		Task task;
		task.title = stringOr(row, "title", "");
		task.description = stringOr(row, "description", "");
		task.status = statusFromName(stringOr(row, "status", "backlog")).value_or(TaskStatus::Backlog);
		task.order = int(numberOr(row, "order_index", 0));
		task.agentModel = stringOr(row, "agent_model", task.agentModel);
		task.harness = stringOr(row, "harness", task.harness);
		task.color = colorFromHex(stringOr(row, "color", ""));
		task.effort = stringOr(row, "effort", "");
		task.permissionMode = stringOr(row, "permission_mode", "");
		task.session.state = sessionStateFromName(stringOr(row, "session_state", ""));
		task.session.detail = stringOr(row, "session_detail", "");
		task.session.launchedWith = launchSettingsFrom(stringOr(row, "session_launch", ""));
		task.workspace = stringOr(row, "workspace", "");
		return task;
	}

	std::optional<Vec2F> rowPosition(const picojson::object& row) {
		auto x = row.find("x");
		auto y = row.find("y");
		if (x == row.end() || y == row.end() || !x->second.is<double>() || !y->second.is<double>()) return std::nullopt;
		return Vec2F(halnf(x->second.get<double>()), halnf(y->second.get<double>()));
	}

}

std::string tp::taskboard::colorToHex(const RGBA& color) {
	char text[8];
	std::snprintf(text, sizeof(text), "#%02x%02x%02x", channel(color.r), channel(color.g), channel(color.b));
	return text;
}

std::optional<RGBA> tp::taskboard::colorFromHex(const std::string& text) {
	unsigned red = 0;
	unsigned green = 0;
	unsigned blue = 0;
	if (text.size() != 7 || text[0] != '#' || std::sscanf(text.c_str() + 1, "%2x%2x%2x", &red, &green, &blue) != 3)
		return std::nullopt;
	return RGBA(halnf(red) / 255, halnf(green) / 255, halnf(blue) / 255, 1);
}

const char* tp::taskboard::serverFieldName(TaskField field) {
	switch (field) {
		case TaskField::Title: return "title";
		case TaskField::Description: return "description";
		case TaskField::Order: return "order_index";
		case TaskField::AgentModel: return "agent_model";
		case TaskField::Harness: return "harness";
		case TaskField::Effort: return "effort";
		case TaskField::PermissionMode: return "permission_mode";
		case TaskField::Status: return "status";
		case TaskField::Color: return "color";
	}
	return "";
}

picojson::value tp::taskboard::fieldValue(const Task& task, TaskField field) {
	switch (field) {
		case TaskField::Title: return picojson::value(task.title);
		case TaskField::Description: return picojson::value(task.description);
		case TaskField::Order: return picojson::value(double(task.order));
		case TaskField::AgentModel: return picojson::value(task.agentModel);
		case TaskField::Harness: return picojson::value(task.harness);
		case TaskField::Effort: return task.effort.empty() ? picojson::value() : picojson::value(task.effort);
		case TaskField::PermissionMode:
			return task.permissionMode.empty() ? picojson::value() : picojson::value(task.permissionMode);
		case TaskField::Status: return picojson::value(std::string(statusName(task.status)));
		case TaskField::Color: return task.color ? picojson::value(colorToHex(*task.color)) : picojson::value();
	}
	return {};
}

void tp::taskboard::setFieldValue(Task& task, TaskField field, const picojson::value& value) {
	switch (field) {
		case TaskField::Title: task.title = value.to_str(); break;
		case TaskField::Description: task.description = value.to_str(); break;
		case TaskField::Order:
			if (value.is<double>()) task.order = int(value.get<double>());
			break;
		case TaskField::AgentModel: task.agentModel = value.to_str(); break;
		case TaskField::Harness: task.harness = value.to_str(); break;
		case TaskField::Effort: task.effort = value.is<std::string>() ? value.get<std::string>() : std::string(); break;
		case TaskField::PermissionMode:
			task.permissionMode = value.is<std::string>() ? value.get<std::string>() : std::string();
			break;
		case TaskField::Status: task.status = statusFromName(value.to_str()).value_or(task.status); break;
		case TaskField::Color: task.color = value.is<std::string>() ? colorFromHex(value.to_str()) : std::nullopt; break;
	}
}

std::string tp::taskboard::fieldLabel(TaskField field) {
	switch (field) {
		case TaskField::Title: return "Title";
		case TaskField::Description: return "Description";
		case TaskField::Order: return "Order";
		case TaskField::AgentModel: return "Model";
		case TaskField::Harness: return "Harness";
		case TaskField::Effort: return "Effort";
		case TaskField::PermissionMode: return "Mode";
		case TaskField::Status: return "Status";
		case TaskField::Color: return "Color";
	}
	return "";
}

namespace {
	NodeLayout layoutFromRow(const picojson::object& row, const NodeLayout& fallback) {
		NodeLayout layout = fallback;
		if (const std::optional<Vec2F> position = rowPosition(row)) layout.position = *position;
		layout.width = halnf(numberOr(row, "width", fallback.width));
		layout.bodyHeight = halnf(numberOr(row, "body_height", fallback.bodyHeight));
		layout.collapsed = numberOr(row, "collapsed", 0) != 0;
		return layout;
	}

	void applyLayout(Node& node, const NodeLayout& layout) {
		node.position = layout.position;
		node.width = layout.width;
		node.bodyHeight = layout.bodyHeight;
		node.collapsed = layout.collapsed;
	}

	picojson::value layoutJson(NodeId id, const NodeLayout& layout) {
		picojson::object object;
		object["task_id"] = picojson::value(double(id));
		object["x"] = picojson::value(double(layout.position.x));
		object["y"] = picojson::value(double(layout.position.y));
		object["width"] = picojson::value(double(layout.width));
		object["body_height"] = picojson::value(double(layout.bodyHeight));
		object["collapsed"] = picojson::value(layout.collapsed);
		return picojson::value(object);
	}

	picojson::value linkJson(const TaskLink& link, const std::set<NodeId>& created) {
		auto endpoint = [&](NodeId id) {
			return created.count(id) ? picojson::value(-double(id)) : picojson::value(double(id));
		};
		picojson::object object;
		object["kind"] = picojson::value(link.kind == LinkKind::Parent ? "parent" : "dependency");
		object["from"] = endpoint(link.from);
		object["to"] = endpoint(link.to);
		return picojson::value(object);
	}

	PortRef outputPort(const TaskLink& link) {
		return TaskBoard::output(link.from, link.kind == LinkKind::Parent ? TaskOutput::Children : TaskOutput::Unblocks);
	}

	PortRef inputPort(const TaskLink& link) {
		return TaskBoard::input(link.to, link.kind == LinkKind::Parent ? TaskPort::Parent : TaskPort::DependsOn);
	}
}

TaskServerSync::TaskServerSync(TaskBoard& board, McpClient& client) :
	mBoard(board),
	mClient(client) {}

bool TaskServerSync::load() {
	const McpToolResult result = mClient.callToolBlocking("get_board", {});
	if (!result.ok || !result.value.is<picojson::object>()) {
		reportError(result.error.empty() ? "get_board returned no board" : result.error);
		return false;
	}

	applyBoard(result.value.get<picojson::object>());
	return true;
}

void TaskServerSync::processResponses() { mClient.dispatchCompletions(); }

void TaskServerSync::pollForChanges() {
	if (mPollInFlight || mSaving || isViewingHistory()) return;

	mPollInFlight = true;
	mClient.callTool("get_board_revision", {}, [this](const McpToolResult& result) {
		mPollInFlight = false;
		if (!result.ok) {
			reportError(result.error);
			return;
		}
		if (result.value.to_str() != mRevision) requestReload();
	});

	if (++mSubrouterPollTick >= 30) {
		mSubrouterPollTick = 0;
		loadSubrouterInfo();
	}
}

void TaskServerSync::loadSubrouterInfo() {
	if (mSubrouterInFlight || !mOnSubrouterInfo) return;
	mSubrouterInFlight = true;
	mClient.callTool("get_subrouter_status", {}, [this](const McpToolResult& result) {
		mSubrouterInFlight = false;
		if (!result.ok || !result.value.is<picojson::object>()) return;
		const picojson::object& object = result.value.get<picojson::object>();

		SubrouterInfo info;
		const auto availIt = object.find("available");
		info.available = availIt != object.end() && availIt->second.is<bool>() && availIt->second.get<bool>();
		info.loginHint = stringOr(object, "login_hint", "");

		const auto accountsIt = object.find("accounts");
		if (accountsIt != object.end() && accountsIt->second.is<picojson::object>()) {
			for (const auto& [provider, pool] : accountsIt->second.get<picojson::object>()) {
				if (!pool.is<picojson::object>()) continue;
				for (const picojson::value& entry : arrayOf(pool.get<picojson::object>(), "accounts")) {
					if (!entry.is<picojson::object>()) continue;
					const picojson::object& acc = entry.get<picojson::object>();
					SubrouterAccount account;
					account.email = stringOr(acc, "email", "");
					const auto coolIt = acc.find("coolingDownUntil");
					if (coolIt != acc.end() && coolIt->second.is<std::string>()) {
						account.coolingDown = true;
						account.cooldownUntil = coolIt->second.get<std::string>();
					}
					info.accounts.push_back(std::move(account));
				}
			}
		}

		mOnSubrouterInfo(info);
	});
}

void TaskServerSync::requestReload() {
	if (mSaving || isViewingHistory()) return;

	mPollInFlight = true;
	const unsigned generation = mBoardGeneration;
	mClient.callTool("get_board", {}, [this, generation](const McpToolResult& result) {
		mPollInFlight = false;
		if (generation != mBoardGeneration) return;
		if (!result.ok || !result.value.is<picojson::object>()) {
			reportError(result.error);
			return;
		}
		applyBoard(result.value.get<picojson::object>());
	});
}

bool TaskServerSync::viewHistoryAt(std::optional<double> moment) {
	if (mSaving) {
		reportError("wait for the save to finish before browsing history");
		return false;
	}
	if (moment && !isViewingHistory() && hasUnsavedChanges()) {
		reportError("save or discard your changes before browsing history");
		return false;
	}

	mViewedMoment = moment;
	if (mOnHistoryChanged) mOnHistoryChanged();

	if (mHistoryInFlight) {
		mQueuedMoment = moment;
		return true;
	}
	requestBoard(moment);
	return true;
}

void TaskServerSync::requestBoard(std::optional<double> moment) {
	picojson::object arguments;
	if (moment) arguments["at"] = number(*moment);

	mHistoryInFlight = true;
	const unsigned generation = ++mBoardGeneration;
	mClient.callTool("get_board", arguments, [this, generation](const McpToolResult& result) {
		boardArrived(result, generation);
	});
}

void TaskServerSync::boardArrived(const McpToolResult& result, unsigned generation) {
	mHistoryInFlight = false;
	if (mQueuedMoment) {
		const std::optional<double> next = *mQueuedMoment;
		mQueuedMoment.reset();
		requestBoard(next);
		return;
	}
	if (generation != mBoardGeneration) return;
	if (!result.ok || !result.value.is<picojson::object>()) {
		reportError(result.error.empty() ? "get_board returned no board" : result.error);
		return;
	}

	applyBoard(result.value.get<picojson::object>());
}

void TaskServerSync::applyHistorySpan(const picojson::object& board) {
	auto found = board.find("history");
	if (found == board.end() || !found->second.is<picojson::object>()) return;

	const picojson::object& history = found->second.get<picojson::object>();
	auto moment = [&](const char* key) -> std::optional<double> {
		auto entry = history.find(key);
		if (entry == history.end() || !entry->second.is<double>()) return std::nullopt;
		return entry->second.get<double>();
	};
	const HistorySpan span{ moment("first"), moment("last") };
	const bool grew = span.last != mHistorySpan.last;
	mHistorySpan = span;
	if (mOnHistoryChanged) mOnHistoryChanged();
	if (grew) loadHistoryChanges();
}

void TaskServerSync::loadHistoryChanges() {
	mClient.callTool("get_history_moments", {}, [this](const McpToolResult& result) {
		if (!result.ok || !result.value.is<picojson::array>()) return;

		std::vector<double> moments;
		for (const picojson::value& entry : result.value.get<picojson::array>())
			if (entry.is<double>()) moments.push_back(entry.get<double>());
		mHistoryChanges = std::move(moments);
		if (mOnHistoryChanged) mOnHistoryChanged();
	});
}

BoardChanges TaskServerSync::unsavedChanges() const {
	return BoardChanges::between(mSaved, BoardSnapshot::capture(mBoard));
}

// An empty server palette means the board default is in effect, so the default is what counts as saved.
// Merging a server board: every piece the user has not changed locally (live board equals mSaved) takes the
// server's value; pieces with unsaved local changes keep the local value, and a field both sides changed
// differently becomes a conflict. mSaved then becomes the server board, so the unsaved difference stays exact.
void TaskServerSync::applyBoard(const picojson::object& board) {
	const BoardSnapshot local = BoardSnapshot::capture(mBoard);

	applyPalette(board, local);
	const std::vector<TaskLink> parentLinks = applyTasks(board, local);
	applyLinks(board, parentLinks, local);
	mRevision = stringOr(board, "revision", "");
	applyHistorySpan(board);

	if (mOnBoardReloaded) mOnBoardReloaded();
}

void TaskServerSync::applyPalette(const picojson::object& board, const BoardSnapshot& local) {
	std::vector<std::string> server;
	for (const picojson::value& entry : arrayOf(board, "palette"))
		if (entry.is<std::string>()) server.push_back(entry.get<std::string>());

	if (server.empty()) {
		for (const RGBA& color : kTaskColors)
			server.push_back(colorToHex(color));
	}

	const bool unchangedLocally = local.palette == mSaved.palette;
	mSaved.palette = server;
	if (!unchangedLocally) return;

	std::vector<RGBA> colors;
	for (const std::string& hex : server)
		if (const auto color = colorFromHex(hex)) colors.push_back(*color);
	mBoard.setColorPalette(colors);
}

std::vector<TaskLink> TaskServerSync::applyTasks(const picojson::object& board, const BoardSnapshot& local) {
	NodeGraph& graph = mBoard.graph();
	std::set<NodeId> present;
	std::vector<TaskLink> parentLinks;
	halnf nextColumn = 40;

	for (const picojson::value& entry : arrayOf(board, "tasks")) {
		if (!entry.is<picojson::object>()) continue;
		const picojson::object& row = entry.get<picojson::object>();
		const std::optional<NodeId> id = idOf(row, "id");
		if (!id) continue;
		present.insert(*id);

		const Task incoming = taskFromRow(row);
		const bool known = mSaved.tasks.count(*id) > 0;
		const bool deletedLocally = known && !local.tasks.count(*id);
		if (const std::optional<NodeId> parent = idOf(row, "parent_id"))
			parentLinks.push_back({ LinkKind::Parent, *parent, *id });

		NodeLayout fallback{ Vec2F(nextColumn, 40.0f),
												 mBoard.graph().type(mBoard.taskType()).width,
												 mBoard.graph().type(mBoard.taskType()).bodyHeight,
												 false };
		if (!rowPosition(row)) nextColumn += 260;
		const NodeLayout layout = layoutFromRow(row, fallback);

		if (deletedLocally) {
			TaskRecord& saved = mSaved.tasks[*id];
			for (TaskField field : persistentFields())
				saved.fields[field] = fieldValue(incoming, field);
			saved.layout = layout;
			continue;
		}

		if (!graph.findNode(*id)) {
			mBoard.addTaskWithId(*id, incoming, layout.position);
			applyLayout(*graph.findNode(*id), layout);
		} else {
			mergeTask(*id, incoming, layout, local);
		}

		TaskRecord& saved = mSaved.tasks[*id];
		for (TaskField field : persistentFields())
			saved.fields[field] = fieldValue(incoming, field);
		saved.layout = layout;
	}

	std::vector<NodeId> removed;
	for (auto it = mSaved.tasks.begin(); it != mSaved.tasks.end();) {
		if (present.count(it->first)) {
			++it;
			continue;
		}
		removed.push_back(it->first);
		it = mSaved.tasks.erase(it);
	}
	graph.removeNodes(removed);
	return parentLinks;
}

void TaskServerSync::mergeTask(NodeId id, const Task& incoming, const NodeLayout& layout, const BoardSnapshot& local) {
	Task& task = *mBoard.task(id);
	task.session = incoming.session;
	task.workspace = incoming.workspace;
	const TaskRecord& saved = mSaved.tasks.at(id);
	const TaskRecord& mine = local.tasks.at(id);

	for (TaskField field : persistentFields()) {
		const picojson::value theirs = fieldValue(incoming, field);
		const picojson::value& base = saved.fields.at(field);
		const picojson::value& current = mine.fields.at(field);
		if (theirs == base) continue;

		const bool changedLocally = current != base || isEditing(id, field);
		if (!changedLocally) {
			setFieldValue(task, field, theirs);
			continue;
		}
		if (current != theirs) addConflict({ id, field, current, theirs });
	}

	if (mine.layout == saved.layout) applyLayout(*mBoard.graph().findNode(id), layout);
}

void TaskServerSync::applyLinks(
	const picojson::object& board, const std::vector<TaskLink>& parentLinks, const BoardSnapshot& local
) {
	std::set<TaskLink> server(parentLinks.begin(), parentLinks.end());
	for (const picojson::value& entry : arrayOf(board, "dependencies")) {
		if (!entry.is<picojson::object>()) continue;
		const picojson::object& row = entry.get<picojson::object>();
		const std::optional<NodeId> task = idOf(row, "task_id");
		const std::optional<NodeId> dependsOn = idOf(row, "depends_on_id");
		if (task && dependsOn) server.insert({ LinkKind::Dependency, *dependsOn, *task });
	}

	for (const TaskLink& link : mSaved.links) {
		const bool goneOnServer = !server.count(link);
		const bool keptLocally = local.links.count(link) > 0;
		if (goneOnServer && keptLocally) disconnect(link);
	}
	for (const TaskLink& link : server) {
		const bool newOnServer = !mSaved.links.count(link);
		if (newOnServer) connect(link);
	}
	mSaved.links = server;
}

void TaskServerSync::connect(const TaskLink& link) {
	if (!mBoard.graph().findNode(link.from) || !mBoard.graph().findNode(link.to)) return;
	mBoard.graph().connect(outputPort(link), inputPort(link));
}

void TaskServerSync::disconnect(const TaskLink& link) {
	mBoard.graph().disconnect({ outputPort(link), inputPort(link) });
}

void TaskServerSync::save() {
	if (mSaving) return;
	if (isViewingHistory()) {
		reportError("a past board is read-only; return to Live to save");
		return;
	}

	const BoardChanges changes = unsavedChanges();
	if (changes.empty()) return;

	mSaving = true;
	mClient.callTool("save_board", saveRequest(changes), [this, changes](const McpToolResult& result) {
		finishSave(result, changes);
	});
}

picojson::object TaskServerSync::saveRequest(const BoardChanges& changes) const {
	const BoardSnapshot current = BoardSnapshot::capture(mBoard);
	const std::set<NodeId> created(changes.created.begin(), changes.created.end());
	picojson::object request;

	picojson::array createdTasks;
	for (NodeId id : changes.created) {
		const TaskRecord& record = current.tasks.at(id);
		picojson::object entry = layoutJson(id, record.layout).get<picojson::object>();
		entry.erase("task_id");
		entry["ref"] = picojson::value(-double(id));
		for (TaskField field : persistentFields())
			entry[serverFieldName(field)] = record.fields.at(field);
		createdTasks.emplace_back(entry);
	}
	request["created"] = picojson::value(createdTasks);

	std::map<NodeId, std::pair<picojson::object, picojson::object>> editsByTask;
	for (const FieldEdit& edit : changes.edits) {
		editsByTask[edit.task].first[serverFieldName(edit.field)] = edit.value;
		editsByTask[edit.task].second[serverFieldName(edit.field)] = edit.expected;
	}
	picojson::array edits;
	for (const auto& [id, edit] : editsByTask) {
		picojson::object entry;
		entry["task_id"] = picojson::value(double(id));
		entry["changes"] = picojson::value(edit.first);
		entry["expected"] = picojson::value(edit.second);
		edits.emplace_back(entry);
	}
	request["edits"] = picojson::value(edits);

	picojson::array layouts;
	for (NodeId id : changes.layouts)
		layouts.push_back(layoutJson(id, current.tasks.at(id).layout));
	request["layouts"] = picojson::value(layouts);

	picojson::array deleted;
	for (NodeId id : changes.deleted)
		deleted.emplace_back(double(id));
	request["deleted"] = picojson::value(deleted);

	picojson::array added;
	for (const TaskLink& link : changes.linksAdded)
		added.push_back(linkJson(link, created));
	request["links_added"] = picojson::value(added);

	picojson::array removed;
	for (const TaskLink& link : changes.linksRemoved)
		removed.push_back(linkJson(link, created));
	request["links_removed"] = picojson::value(removed);

	if (changes.palette) {
		picojson::array palette;
		for (const std::string& color : *changes.palette)
			palette.emplace_back(color);
		request["palette"] = picojson::value(palette);
	}
	return request;
}

void TaskServerSync::finishSave(const McpToolResult& result, const BoardChanges& changes) {
	mSaving = false;
	if (!result.ok || !result.value.is<picojson::object>()) {
		reportError("save failed, nothing was written: " + result.error);
		return;
	}

	const picojson::object& response = result.value.get<picojson::object>();
	if (response.count("ids") && response.at("ids").is<picojson::object>())
		adoptCreatedIds(response.at("ids").get<picojson::object>());

	for (const picojson::value& entry : arrayOf(response, "conflicts")) {
		if (!entry.is<picojson::object>()) continue;
		const picojson::object& conflict = entry.get<picojson::object>();
		const std::optional<NodeId> id = idOf(conflict, "task_id");
		const std::string field = stringOr(conflict, "field", "");
		for (TaskField candidate : persistentFields()) {
			if (!id || serverFieldName(candidate) != field) continue;
			const picojson::object& current = conflict.at("current").get<picojson::object>();
			addConflict({ *id, candidate, conflict.at("mine"), current.at(field) });
		}
	}

	(void) changes;
	requestReload();
	if (mOnSaved) mOnSaved();
}

// Created tasks get their server id: their node is re-keyed in place so selection, links and layout survive.
void TaskServerSync::adoptCreatedIds(const picojson::object& ids) {
	for (const auto& [reference, value] : ids) {
		if (!value.is<double>()) continue;
		const NodeId local = NodeId(-std::stod(reference));
		const NodeId server = NodeId(value.get<double>());
		mBoard.graph().renameNode(local, server);

		const BoardSnapshot current = BoardSnapshot::capture(mBoard);
		mSaved.tasks[server] = current.tasks.at(server);
	}
}

void TaskServerSync::startSession(NodeId id) { sessionCall("start_session", id, {}); }

void TaskServerSync::restartSession(NodeId id, bool fresh) {
	picojson::object arguments;
	arguments["fresh"] = picojson::value(fresh);
	sessionCall("restart_session", id, arguments);
}

void TaskServerSync::stopSession(NodeId id) { sessionCall("stop_session", id, {}); }

void TaskServerSync::listSecrets(std::function<void(std::vector<SecretEntry>)> onResult) {
	mClient.callTool("list_secrets", {}, [this, onResult = std::move(onResult)](const McpToolResult& result) {
		if (!result.ok || !result.value.is<picojson::array>()) {
			if (!result.ok) reportError("list_secrets: " + result.error);
			onResult({});
			return;
		}
		std::vector<SecretEntry> entries;
		for (const picojson::value& entry : result.value.get<picojson::array>()) {
			if (!entry.is<picojson::object>()) continue;
			const picojson::object& object = entry.get<picojson::object>();
			entries.push_back({ stringOr(object, "key", ""), stringOr(object, "description", "") });
		}
		onResult(std::move(entries));
	});
}

void TaskServerSync::setSecret(
	const std::string& key, const std::string& value, const std::string& description,
	std::function<void(bool, std::string)> onDone
) {
	picojson::object arguments;
	arguments["key"] = picojson::value(key);
	arguments["value"] = picojson::value(value);
	arguments["description"] = picojson::value(description);
	mClient.callTool("set_secret", arguments, [onDone = std::move(onDone)](const McpToolResult& result) {
		onDone(result.ok, result.ok ? std::string() : result.error);
	});
}

void TaskServerSync::deleteSecret(const std::string& key, std::function<void(bool, std::string)> onDone) {
	picojson::object arguments;
	arguments["key"] = picojson::value(key);
	mClient.callTool("delete_secret", arguments, [onDone = std::move(onDone)](const McpToolResult& result) {
		onDone(result.ok, result.ok ? std::string() : result.error);
	});
}

void TaskServerSync::sessionCall(const std::string& tool, NodeId id, picojson::object arguments) {
	if (isViewingHistory()) {
		reportError("sessions are not available on a past board; return to Live");
		return;
	}
	if (!mSaved.tasks.count(id)) {
		reportError("save the task before starting its session");
		return;
	}
	arguments["task_id"] = number(double(id));
	mClient.callTool(tool, arguments, [this, tool](const McpToolResult& result) {
		if (!result.ok) {
			reportError(tool + ": " + result.error);
			return;
		}
		requestReload();
	});
}

bool TaskServerSync::loadOptions() {
	const McpToolResult result = mClient.callToolBlocking("get_options", {});
	if (!result.ok || !result.value.is<picojson::object>()) return false;

	const picojson::object& object = result.value.get<picojson::object>();
	AgentOptions options;
	for (const picojson::value& entry : arrayOf(object, "models")) {
		if (!entry.is<picojson::object>()) continue;
		const picojson::object& model = entry.get<picojson::object>();
		options.models.push_back({ stringOr(model, "id", ""), stringOr(model, "label", "") });
	}
	for (const picojson::value& entry : arrayOf(object, "harnesses")) {
		if (!entry.is<picojson::object>()) continue;
		const picojson::object& harness = entry.get<picojson::object>();
		options.harnesses.push_back({ stringOr(harness, "id", ""), stringOr(harness, "label", "") });
	}
	if (options.harnesses.empty()) options.harnesses = offlineAgentOptions().harnesses;
	options.efforts = stringsOf(object, "efforts");
	options.permissionModes = stringsOf(object, "permission_modes");
	if (options.models.empty() || options.efforts.empty() || options.permissionModes.empty()) return false;

	mOptions = std::move(options);
	return true;
}

void TaskServerSync::beginEditing(NodeId id, TaskField field) { mEditing = std::make_pair(id, field); }

void TaskServerSync::endEditing() { mEditing.reset(); }

bool TaskServerSync::isEditing(NodeId id, TaskField field) const { return mEditing == std::make_pair(id, field); }

void TaskServerSync::addConflict(FieldConflict conflict) {
	dropConflict(conflict.task, conflict.field);
	mConflicts.push_back(std::move(conflict));
	if (mOnConflictsChanged) mOnConflictsChanged();
}

bool TaskServerSync::dropConflict(NodeId id, TaskField field) {
	const size_t removed = std::erase_if(mConflicts, [&](const FieldConflict& existing) {
		return existing.task == id && existing.field == field;
	});
	return removed > 0;
}

// Keep mine: the local value stays and remains unsaved (it now differs from the server's), so Save writes it.
// Use theirs: the local value becomes the server's, so the field is no longer unsaved.
void TaskServerSync::resolveConflict(FieldConflict conflict, bool keepMine) {
	dropConflict(conflict.task, conflict.field);
	if (Task* task = mBoard.task(conflict.task))
		setFieldValue(*task, conflict.field, keepMine ? conflict.mine : conflict.theirs);

	if (mOnConflictsChanged) mOnConflictsChanged();
	if (mOnBoardReloaded) mOnBoardReloaded();
}

void TaskServerSync::reportError(const std::string& error) {
	mLastError = error;
	if (mOnError) mOnError(error);
}
