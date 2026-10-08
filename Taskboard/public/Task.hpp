#pragma once

#include "Color.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tp::taskboard {

	enum class TaskStatus { Backlog, Queued, Wip, Daemon, Review, Done, Blocked };

	inline constexpr std::array kTaskStatuses = {
		TaskStatus::Backlog, TaskStatus::Queued, TaskStatus::Wip,     TaskStatus::Daemon,
		TaskStatus::Review,  TaskStatus::Done,   TaskStatus::Blocked,
	};

	struct AgentOption {
		std::string id;
		std::string label;
	};

	struct AgentOptions {
		std::vector<AgentOption> models;
		std::vector<AgentOption> harnesses;
		std::vector<std::string> efforts;
		std::vector<std::string> permissionModes;
	};

	[[nodiscard]] AgentOptions offlineAgentOptions();

	enum class SessionState { None, Starting, Working, WaitingInput, Idle, Error, Ended };

	struct LaunchSettings {
		std::string agentModel;
		std::string harness;
		std::string effort;
		std::string permissionMode;

		bool operator==(const LaunchSettings&) const = default;
	};

	struct SessionInfo {
		SessionState state = SessionState::None;
		std::string detail;
		std::optional<LaunchSettings> launchedWith;

		[[nodiscard]] bool isRunning() const { return state != SessionState::None && state != SessionState::Ended; }
	};

	struct SubrouterAccount {
		std::string email;
		bool coolingDown = false;
		std::string cooldownUntil; // ISO timestamp, non-empty when coolingDown
	};

	struct SubrouterInfo {
		bool available = false;
		std::string loginHint;
		std::vector<SubrouterAccount> accounts;
	};

	[[nodiscard]] SessionState sessionStateFromName(std::string_view name);
	[[nodiscard]] std::string_view sessionStateLabel(SessionState state);

	inline const std::array<RGBA, 8> kTaskColors = {
		RGBA{ 0.70f, 0.26f, 0.24f, 1.0f }, RGBA{ 0.82f, 0.49f, 0.19f, 1.0f }, RGBA{ 0.75f, 0.64f, 0.20f, 1.0f },
		RGBA{ 0.25f, 0.55f, 0.32f, 1.0f }, RGBA{ 0.20f, 0.52f, 0.55f, 1.0f }, RGBA{ 0.21f, 0.42f, 0.79f, 1.0f },
		RGBA{ 0.55f, 0.36f, 0.81f, 1.0f }, RGBA{ 0.80f, 0.32f, 0.55f, 1.0f },
	};

	struct Task {
		std::string title = "New task";
		std::string description;
		TaskStatus status = TaskStatus::Backlog;
		int order = 0;
		std::string agentModel = "claude-opus-5-5";
		std::string harness = "claude-code";
		std::string effort = "high";
		std::string permissionMode = "auto";
		std::optional<RGBA> color;
		SessionInfo session;
		std::string workspace;

		[[nodiscard]] LaunchSettings launchSettings() const {
			return {
				agentModel, harness.empty() ? "claude-code" : harness, effort, permissionMode.empty() ? "auto" : permissionMode
			};
		}
		[[nodiscard]] bool sessionSettingsChanged() const {
			return session.launchedWith && *session.launchedWith != launchSettings();
		}
	};

	[[nodiscard]] std::string_view statusName(TaskStatus status);
	[[nodiscard]] std::string_view statusLabel(TaskStatus status);
	[[nodiscard]] std::optional<TaskStatus> statusNamed(std::string_view name);
	[[nodiscard]] std::optional<TaskStatus> statusFromName(std::string_view name);
}
