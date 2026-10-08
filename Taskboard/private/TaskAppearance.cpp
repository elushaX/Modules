#include "TaskAppearance.hpp"

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

RGBA tp::taskboard::taskStatusColor(TaskStatus status) {
	switch (status) {
		case TaskStatus::Backlog: return { 0.32f, 0.34f, 0.38f, 1.0f };
		case TaskStatus::Queued: return { 0.36f, 0.42f, 0.62f, 1.0f };
		case TaskStatus::Wip: return { 0.70f, 0.55f, 0.15f, 1.0f };
		case TaskStatus::Daemon: return { 0.16f, 0.53f, 0.60f, 1.0f };
		case TaskStatus::Review: return { 0.50f, 0.32f, 0.70f, 1.0f };
		case TaskStatus::Done: return { 0.25f, 0.55f, 0.32f, 1.0f };
		case TaskStatus::Blocked: return { 0.70f, 0.26f, 0.24f, 1.0f };
	}
	return { 0.3f, 0.3f, 0.3f, 1.0f };
}

RGBA tp::taskboard::sessionStateColor(SessionState state) {
	switch (state) {
		case SessionState::None: return { 0.45f, 0.45f, 0.48f, 1 };
		case SessionState::Starting: return { 0.75f, 0.75f, 0.80f, 1 };
		case SessionState::Working: return { 0.35f, 0.85f, 0.45f, 1 };
		case SessionState::WaitingInput: return { 1.00f, 0.72f, 0.20f, 1 };
		case SessionState::Idle: return { 0.40f, 0.65f, 1.00f, 1 };
		case SessionState::Error: return { 0.95f, 0.35f, 0.32f, 1 };
		case SessionState::Ended: return { 0.60f, 0.60f, 0.62f, 1 };
	}
	return { 0.5f, 0.5f, 0.5f, 1 };
}

std::string tp::taskboard::taskLabel(NodeId id, const Task& task) {
	return "#" + std::to_string(id) + "  " + task.title;
}

RGBA tp::taskboard::taskAccentColor(const Task& task) { return task.color.value_or(taskStatusColor(task.status)); }

bool tp::taskboard::isSessionActive(SessionState state) {
	return state == SessionState::Starting || state == SessionState::Working || state == SessionState::WaitingInput;
}
