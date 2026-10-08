#pragma once

#include "NodeGraph.hpp"

#include <functional>
#include <string>
#include <vector>

namespace tp::taskboard {

	enum class TaskField { Title, Description, Order, AgentModel, Harness, Effort, PermissionMode, Status, Color };

	struct SecretEntry {
		std::string key;
		std::string description;
	};

	// The board manages which secrets exist, but never their values: agents read a value at run time through
	// the server's get_secret tool. So a store lists, sets and deletes keys; it never returns a value.
	class SecretsStore {
	public:
		virtual ~SecretsStore() = default;

		virtual void listSecrets(std::function<void(std::vector<SecretEntry>)> onResult) = 0;
		virtual void setSecret(
			const std::string& key, const std::string& value, const std::string& description,
			std::function<void(bool ok, std::string error)> onDone
		) = 0;
		virtual void deleteSecret(const std::string& key, std::function<void(bool ok, std::string error)> onDone) = 0;
	};

	// Board edits stay local until saved; a store only handles actions that act on the server immediately
	// (sessions) and tells the view which field the user is typing in, so incoming changes leave it alone.
	class TaskStore {
	public:
		virtual ~TaskStore() = default;

		virtual void startSession(ui::NodeId id) = 0;
		virtual void restartSession(ui::NodeId id, bool fresh) = 0;
		virtual void stopSession(ui::NodeId id) = 0;
		virtual void beginEditing(ui::NodeId id, TaskField field) = 0;
		virtual void endEditing() = 0;
	};
}
