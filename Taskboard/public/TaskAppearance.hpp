#pragma once

#include "TaskBoard.hpp"

#include <string>

namespace tp::taskboard {

	[[nodiscard]] RGBA taskStatusColor(TaskStatus status);
	[[nodiscard]] RGBA sessionStateColor(SessionState state);
	[[nodiscard]] RGBA taskAccentColor(const Task& task);
	[[nodiscard]] bool isSessionActive(SessionState state);
	[[nodiscard]] std::string taskLabel(ui::NodeId id, const Task& task);
}
