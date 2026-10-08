#include "TaskKanban.hpp"

#include "picojson.h"

#include <algorithm>

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

namespace {
	constexpr const char* kSettingsFile = "kanban-columns.json";

	bool contains(const std::vector<TaskStatus>& statuses, TaskStatus status) {
		return std::find(statuses.begin(), statuses.end(), status) != statuses.end();
	}

	std::vector<TaskStatus> statusesNamedIn(const picojson::value& list) {
		std::vector<TaskStatus> statuses;
		if (!list.is<picojson::array>()) return statuses;
		for (const picojson::value& entry : list.get<picojson::array>()) {
			if (!entry.is<std::string>()) continue;
			const std::optional<TaskStatus> status = statusNamed(entry.get<std::string>());
			if (status && !contains(statuses, *status)) statuses.push_back(*status);
		}
		return statuses;
	}

	std::vector<TaskStatus> withMissingStatusesAppended(std::vector<TaskStatus> order) {
		for (const TaskStatus status : kTaskStatuses)
			if (!contains(order, status)) order.push_back(status);
		return order;
	}

	picojson::value namesOf(const std::vector<TaskStatus>& statuses) {
		picojson::array names;
		for (const TaskStatus status : statuses)
			names.emplace_back(std::string(statusName(status)));
		return picojson::value(names);
	}
}

std::vector<KanbanColumn> TaskKanbanModel::columns() const {
	std::vector<KanbanColumn> columns;
	for (const TaskStatus status : mSettings.order()) {
		KanbanColumn column;
		column.title = std::string(statusLabel(status));
		column.ordering = KanbanOrdering::Manual;
		column.visible = !contains(mSettings.hidden(), status);
		columns.push_back(column);
	}
	return columns;
}

size_t TaskKanbanModel::columnOf(TaskStatus status) const {
	const std::vector<TaskStatus>& order = mSettings.order();
	return size_t(std::find(order.begin(), order.end(), status) - order.begin());
}

std::vector<KanbanItemId> TaskKanbanModel::items(size_t column) const {
	const TaskStatus status = statusOf(column);
	std::vector<KanbanItemId> ids;
	for (const NodeId id : mBoard.tasksInOrder())
		if (mBoard.task(id)->status == status) ids.push_back(id);
	return ids;
}

void tp::taskboard::placeMovedCard(TaskBoard& board, const TaskKanbanModel& model, const KanbanMove& move) {
	Task* moved = board.task(move.item);
	if (!moved) return;

	std::vector<KanbanItemId> destination = model.items(move.toColumn);
	destination.erase(std::remove(destination.begin(), destination.end(), move.item), destination.end());
	moved->status = model.statusOf(move.toColumn);

	if (move.toIndex < destination.size()) board.placeBefore(move.item, destination[move.toIndex]);
	else if (!destination.empty()) board.placeAfter(move.item, destination.back());
}

KanbanColumnSettings::KanbanColumnSettings(FileSystem& files, const std::string& storageDirectory) :
	mFiles(files),
	mPath(joinPath(storageDirectory, kSettingsFile)),
	mOrder(orderByDefault()),
	mHidden(hiddenByDefault()) {
	load();
}

std::vector<TaskStatus> KanbanColumnSettings::orderByDefault() {
	return { kTaskStatuses.begin(), kTaskStatuses.end() };
}

std::vector<TaskStatus> KanbanColumnSettings::hiddenByDefault() {
	return { TaskStatus::Backlog, TaskStatus::Daemon, TaskStatus::Blocked };
}

void KanbanColumnSettings::moveColumn(size_t from, size_t to) {
	if (from >= mOrder.size() || to >= mOrder.size() || from == to) return;
	const TaskStatus moved = mOrder[from];
	mOrder.erase(mOrder.begin() + std::ptrdiff_t(from));
	mOrder.insert(mOrder.begin() + std::ptrdiff_t(to), moved);
	store();
}

void KanbanColumnSettings::setVisible(TaskStatus status, bool visible) {
	if (contains(mHidden, status) != visible) return;
	if (visible) mHidden.erase(std::remove(mHidden.begin(), mHidden.end(), status), mHidden.end());
	else mHidden.push_back(status);
	store();
}

void KanbanColumnSettings::load() {
	const std::optional<std::string> stored = mFiles.readText(mPath);
	if (!stored) return;

	picojson::value document;
	if (!picojson::parse(document, *stored).empty() || !document.is<picojson::object>()) return;
	const picojson::object& root = document.get<picojson::object>();

	if (const auto order = root.find("order"); order != root.end())
		mOrder = withMissingStatusesAppended(statusesNamedIn(order->second));
	if (const auto hidden = root.find("hidden"); hidden != root.end() && hidden->second.is<picojson::array>())
		mHidden = statusesNamedIn(hidden->second);
}

void KanbanColumnSettings::store() {
	picojson::object root;
	root["order"] = namesOf(mOrder);
	root["hidden"] = namesOf(mHidden);
	if (createDirectories(mFiles, parentPath(mPath))) mFiles.writeText(mPath, picojson::value(root).serialize(true));
}
