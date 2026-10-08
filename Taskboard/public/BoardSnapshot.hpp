#pragma once

#include "TaskBoard.hpp"
#include "TaskStore.hpp"

#include "picojson.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace tp::taskboard {

	enum class LinkKind { Parent, Dependency };

	struct TaskLink {
		LinkKind kind = LinkKind::Parent;
		ui::NodeId from = 0;
		ui::NodeId to = 0;

		auto operator<=>(const TaskLink&) const = default;
	};

	struct NodeLayout {
		Vec2F position;
		halnf width = 0;
		halnf bodyHeight = 0;
		bool collapsed = false;

		bool operator==(const NodeLayout&) const = default;
	};

	struct TaskRecord {
		std::map<TaskField, picojson::value> fields;
		NodeLayout layout;
	};

	// A complete description of the board's persistent content. The server's last known board and the
	// board the user is editing are both snapshots; what is unsaved is exactly their difference.
	struct BoardSnapshot {
		std::map<ui::NodeId, TaskRecord> tasks;
		std::set<TaskLink> links;
		std::vector<std::string> palette;

		[[nodiscard]] static BoardSnapshot capture(const TaskBoard& board);
	};

	struct FieldEdit {
		ui::NodeId task = 0;
		TaskField field = TaskField::Title;
		picojson::value value;
		picojson::value expected;
	};

	struct BoardChanges {
		std::vector<ui::NodeId> created;
		std::vector<ui::NodeId> deleted;
		std::vector<FieldEdit> edits;
		std::vector<ui::NodeId> layouts;
		std::vector<TaskLink> linksAdded;
		std::vector<TaskLink> linksRemoved;
		std::optional<std::vector<std::string>> palette;

		[[nodiscard]] bool empty() const;
		[[nodiscard]] size_t count() const;
		[[nodiscard]] static BoardChanges between(const BoardSnapshot& saved, const BoardSnapshot& current);
	};

	[[nodiscard]] LinkKind linkKind(const ui::Link& link);
	[[nodiscard]] const std::vector<TaskField>& persistentFields();
}
