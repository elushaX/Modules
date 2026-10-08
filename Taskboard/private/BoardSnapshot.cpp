#include "BoardSnapshot.hpp"

#include "TaskServerSync.hpp"

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

const std::vector<TaskField>& tp::taskboard::persistentFields() {
	static const std::vector<TaskField> fields = {
		TaskField::Title,  TaskField::Description,    TaskField::Order,  TaskField::AgentModel, TaskField::Harness,
		TaskField::Effort, TaskField::PermissionMode, TaskField::Status, TaskField::Color,
	};
	return fields;
}

LinkKind tp::taskboard::linkKind(const Link& link) {
	return link.from.index == ualni(TaskOutput::Unblocks) ? LinkKind::Dependency : LinkKind::Parent;
}

BoardSnapshot BoardSnapshot::capture(const TaskBoard& board) {
	BoardSnapshot snapshot;
	const NodeGraph& graph = board.graph();

	for (const Node& node : graph.nodes()) {
		const Task* task = board.task(node.id);
		if (!task) continue;

		TaskRecord& record = snapshot.tasks[node.id];
		for (TaskField field : persistentFields())
			record.fields[field] = fieldValue(*task, field);
		record.layout = { node.position, node.width, node.bodyHeight, node.collapsed };
	}

	for (const Link& link : graph.links())
		snapshot.links.insert({ linkKind(link), link.from.node, link.to.node });

	for (const RGBA& color : board.colorPalette())
		snapshot.palette.push_back(colorToHex(color));
	return snapshot;
}

bool BoardChanges::empty() const { return count() == 0; }

size_t BoardChanges::count() const {
	return created.size() + deleted.size() + edits.size() + layouts.size() + linksAdded.size() + linksRemoved.size() +
				 (palette ? 1 : 0);
}

BoardChanges BoardChanges::between(const BoardSnapshot& saved, const BoardSnapshot& current) {
	BoardChanges changes;

	for (const auto& [id, record] : current.tasks) {
		auto previous = saved.tasks.find(id);
		if (previous == saved.tasks.end()) {
			changes.created.push_back(id);
			continue;
		}
		for (TaskField field : persistentFields()) {
			const picojson::value& before = previous->second.fields.at(field);
			const picojson::value& now = record.fields.at(field);
			if (before != now) changes.edits.push_back({ id, field, now, before });
		}
		if (!(previous->second.layout == record.layout)) changes.layouts.push_back(id);
	}

	for (const auto& [id, record] : saved.tasks) {
		if (!current.tasks.count(id)) changes.deleted.push_back(id);
	}

	for (const TaskLink& link : current.links) {
		if (!saved.links.count(link)) changes.linksAdded.push_back(link);
	}
	for (const TaskLink& link : saved.links) {
		const bool endpointDeleted = !current.tasks.count(link.from) || !current.tasks.count(link.to);
		if (!current.links.count(link) && !endpointDeleted) changes.linksRemoved.push_back(link);
	}

	if (saved.palette != current.palette) changes.palette = current.palette;
	return changes;
}
