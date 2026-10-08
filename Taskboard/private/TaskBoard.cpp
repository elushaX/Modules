#include "TaskBoard.hpp"

#include <algorithm>

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

namespace {
	const RGBA kHierarchyColor = { 0.60f, 0.62f, 0.68f, 1.0f };
	const RGBA kDependencyColor = { 0.90f, 0.62f, 0.25f, 1.0f };

	NodeType taskNodeType() {
		NodeType type;
		type.name = "Task";
		type.width = 220;
		type.bodyHeight = 90;
		type.inputs = {
			{ "parent", "hierarchy", kHierarchyColor, false },
			{ "depends on", "dependency", kDependencyColor, true },
		};
		type.outputs = {
			{ "children", "hierarchy", kHierarchyColor, true },
			{ "unblocks", "dependency", kDependencyColor, true },
		};
		return type;
	}
}

std::string_view tp::taskboard::statusName(TaskStatus status) {
	switch (status) {
		case TaskStatus::Backlog: return "backlog";
		case TaskStatus::Queued: return "queued";
		case TaskStatus::Wip: return "wip";
		case TaskStatus::Daemon: return "daemon";
		case TaskStatus::Review: return "review";
		case TaskStatus::Done: return "done";
		case TaskStatus::Blocked: return "blocked";
	}
	return "unknown";
}

std::string_view tp::taskboard::statusLabel(TaskStatus status) {
	switch (status) {
		case TaskStatus::Backlog: return "Backlog";
		case TaskStatus::Queued: return "Queued";
		case TaskStatus::Wip: return "WIP";
		case TaskStatus::Daemon: return "Daemon";
		case TaskStatus::Review: return "Review";
		case TaskStatus::Done: return "Done";
		case TaskStatus::Blocked: return "Blocked";
	}
	return "Unknown";
}

std::optional<TaskStatus> tp::taskboard::statusNamed(std::string_view name) {
	for (TaskStatus status : kTaskStatuses) {
		if (statusName(status) == name) return status;
	}
	return std::nullopt;
}

std::optional<TaskStatus> tp::taskboard::statusFromName(std::string_view name) {
	if (name == "running") return TaskStatus::Wip;
	if (name == "ready") return TaskStatus::Queued;
	if (name == "failed") return TaskStatus::Blocked;
	return statusNamed(name);
}

AgentOptions tp::taskboard::offlineAgentOptions() {
	return {
		{ { "claude-opus-5-5", "Opus 5.5" },
			{ "claude-opus-4-8", "Opus 4.8" },
			{ "claude-sonnet-4-6", "Sonnet 4.6" },
			{ "claude-haiku-4-5", "Haiku 4.5" },
			{ "deepseek-chat", "DeepSeek Chat" },
			{ "deepseek-flash", "DeepSeek Flash" } },
		{ { "claude-code", "Claude Code" }, { "dsh", "DeepSeek Harness" } },
		{ "low", "medium", "high", "xhigh", "max" },
		{ "auto", "manual" },
	};
}

SessionState tp::taskboard::sessionStateFromName(std::string_view name) {
	if (name == "starting") return SessionState::Starting;
	if (name == "working") return SessionState::Working;
	if (name == "waiting_input") return SessionState::WaitingInput;
	if (name == "idle") return SessionState::Idle;
	if (name == "error") return SessionState::Error;
	if (name == "ended") return SessionState::Ended;
	return SessionState::None;
}

std::string_view tp::taskboard::sessionStateLabel(SessionState state) {
	switch (state) {
		case SessionState::None: return "no session";
		case SessionState::Starting: return "starting";
		case SessionState::Working: return "running";
		case SessionState::WaitingInput: return "needs input";
		case SessionState::Idle: return "turn finished";
		case SessionState::Error: return "error";
		case SessionState::Ended: return "ended";
	}
	return "unknown";
}

TaskBoard::TaskBoard() :
	mTaskType(mGraph.registerType(taskNodeType())) {
	mGraph.setLinkValidator([this](const NodeGraph&, const Link& link) { return keepsGraphAcyclic(link); });
}

NodeId TaskBoard::addTask(Task task, Vec2F position) { return mGraph.addNode(mTaskType, position, std::move(task)); }

bool TaskBoard::addTaskWithId(NodeId id, Task task, Vec2F position) {
	return mGraph.addNodeWithId(id, mTaskType, position, std::move(task));
}

Task* TaskBoard::task(NodeId id) {
	Node* node = mGraph.findNode(id);
	return node ? std::any_cast<Task>(&node->payload) : nullptr;
}

const Task* TaskBoard::task(NodeId id) const { return const_cast<TaskBoard*>(this)->task(id); }

PortRef TaskBoard::input(NodeId node, TaskPort port) { return { node, PortDirection::Input, ualni(port) }; }

PortRef TaskBoard::output(NodeId node, TaskOutput port) { return { node, PortDirection::Output, ualni(port) }; }

bool TaskBoard::setParent(NodeId child, NodeId parent) {
	return mGraph.connect(output(parent, TaskOutput::Children), input(child, TaskPort::Parent));
}

bool TaskBoard::addDependency(NodeId task, NodeId dependsOn) {
	return mGraph.connect(output(dependsOn, TaskOutput::Unblocks), input(task, TaskPort::DependsOn));
}

void TaskBoard::clearParent(NodeId child) { mGraph.disconnectPort(input(child, TaskPort::Parent)); }

void TaskBoard::removeDependency(NodeId task, NodeId dependsOn) {
	mGraph.disconnect({ output(dependsOn, TaskOutput::Unblocks), input(task, TaskPort::DependsOn) });
}

bool TaskBoard::canAddDependency(NodeId task, NodeId dependsOn) const {
	if (task == dependsOn || !mGraph.findNode(task) || !mGraph.findNode(dependsOn)) return false;
	return keepsGraphAcyclic({ output(dependsOn, TaskOutput::Unblocks), input(task, TaskPort::DependsOn) });
}

bool TaskBoard::canSetParent(NodeId child, NodeId parent) const {
	if (child == parent || !mGraph.findNode(child) || !mGraph.findNode(parent)) return false;
	return keepsGraphAcyclic({ output(parent, TaskOutput::Children), input(child, TaskPort::Parent) });
}

std::optional<NodeId> TaskBoard::parentOf(NodeId id) const {
	const std::vector<Link> links = mGraph.linksAt(input(id, TaskPort::Parent));
	if (links.empty()) return std::nullopt;
	return links.front().from.node;
}

std::vector<NodeId> TaskBoard::ancestorsOf(NodeId id) const {
	std::vector<NodeId> ancestors;
	for (std::optional<NodeId> parent = parentOf(id); parent; parent = parentOf(*parent))
		ancestors.insert(ancestors.begin(), *parent);
	return ancestors;
}

std::vector<NodeId> TaskBoard::childrenOf(NodeId id) const {
	std::vector<NodeId> children;
	for (const Link& link : mGraph.linksAt(output(id, TaskOutput::Children)))
		children.push_back(link.to.node);
	sortByOrder(children);
	return children;
}

std::vector<NodeId> TaskBoard::rootTasks() const {
	std::vector<NodeId> roots;
	for (const Node& node : mGraph.nodes()) {
		if (!parentOf(node.id)) roots.push_back(node.id);
	}
	sortByOrder(roots);
	return roots;
}

void TaskBoard::sortByOrder(std::vector<NodeId>& ids) const {
	std::sort(ids.begin(), ids.end(), [this](NodeId left, NodeId right) {
		return std::make_pair(task(left)->order, left) < std::make_pair(task(right)->order, right);
	});
}

std::vector<NodeId> TaskBoard::tasksInOrder() const {
	std::vector<NodeId> ids;
	for (const Node& node : mGraph.nodes())
		if (task(node.id)) ids.push_back(node.id);
	sortByOrder(ids);
	return ids;
}

int TaskBoard::orderAfterLast() const {
	const std::vector<NodeId> ordered = tasksInOrder();
	return ordered.empty() ? 0 : task(ordered.back())->order + 1;
}

void TaskBoard::placeBefore(NodeId moved, NodeId anchor) { moveInOrder(moved, anchor, 0); }

void TaskBoard::placeAfter(NodeId moved, NodeId anchor) { moveInOrder(moved, anchor, 1); }

void TaskBoard::placeLastAmongSiblings(NodeId id) {
	const std::optional<NodeId> parent = parentOf(id);
	std::vector<NodeId> siblings = parent ? childrenOf(*parent) : rootTasks();
	siblings.erase(std::remove(siblings.begin(), siblings.end(), id), siblings.end());
	if (!siblings.empty()) placeAfter(id, siblings.back());
}

void TaskBoard::moveInOrder(NodeId moved, NodeId anchor, size_t anchorOffset) {
	if (moved == anchor || !task(moved) || !task(anchor)) return;

	std::vector<NodeId> ordered = tasksInOrder();
	const size_t from = size_t(std::find(ordered.begin(), ordered.end(), moved) - ordered.begin());
	ordered.erase(ordered.begin() + std::ptrdiff_t(from));
	const size_t to = size_t(std::find(ordered.begin(), ordered.end(), anchor) - ordered.begin()) + anchorOffset;
	ordered.insert(ordered.begin() + std::ptrdiff_t(to), moved);

	const size_t first = std::min(from, to);
	const size_t last = std::max(from, to);
	std::vector<int> ranks;
	for (size_t index = first; index <= last; index++)
		ranks.push_back(task(ordered[index])->order);
	std::sort(ranks.begin(), ranks.end());

	const bool ranksAreDistinct = std::adjacent_find(ranks.begin(), ranks.end()) == ranks.end();
	if (!ranksAreDistinct) {
		for (size_t index = 0; index < ordered.size(); index++)
			task(ordered[index])->order = int(index);
		return;
	}
	for (size_t index = first; index <= last; index++)
		task(ordered[index])->order = ranks[index - first];
}

std::vector<NodeId> TaskBoard::dependentsOf(NodeId id) const {
	std::vector<NodeId> dependents;
	for (const Link& link : mGraph.linksAt(output(id, TaskOutput::Unblocks)))
		dependents.push_back(link.to.node);
	std::sort(dependents.begin(), dependents.end());
	return dependents;
}

std::vector<NodeId> TaskBoard::dependenciesOf(NodeId id) const {
	std::vector<NodeId> dependencies;
	for (const Link& link : mGraph.linksAt(input(id, TaskPort::DependsOn)))
		dependencies.push_back(link.from.node);
	return dependencies;
}

bool TaskBoard::isRunnable(NodeId id) const {
	const Task* candidate = task(id);
	if (!candidate || candidate->status != TaskStatus::Queued) return false;

	const std::vector<NodeId> dependencies = dependenciesOf(id);
	return std::all_of(dependencies.begin(), dependencies.end(), [&](NodeId dependency) {
		return task(dependency)->status == TaskStatus::Done;
	});
}

bool TaskBoard::reaches(NodeId from, NodeId to, ualni outputIndex) const {
	std::vector<NodeId> pending = { from };
	std::vector<NodeId> visited;

	while (!pending.empty()) {
		const NodeId current = pending.back();
		pending.pop_back();
		if (current == to) return true;
		if (std::find(visited.begin(), visited.end(), current) != visited.end()) continue;

		visited.push_back(current);
		for (const Link& link : mGraph.linksAt({ current, PortDirection::Output, outputIndex }))
			pending.push_back(link.to.node);
	}
	return false;
}

bool TaskBoard::keepsGraphAcyclic(const Link& link) const {
	return !reaches(link.to.node, link.from.node, link.from.index);
}
