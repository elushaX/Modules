#pragma once

#include "NodeGraph.hpp"
#include "NodeGraph.hpp"
#include "Task.hpp"

#include <optional>
#include <vector>

namespace tp::taskboard {

	enum class TaskPort : ualni { Parent = 0, DependsOn = 1 };
	enum class TaskOutput : ualni { Children = 0, Unblocks = 1 };

	class TaskBoard {
	public:
		TaskBoard();

		[[nodiscard]] ui::NodeGraph& graph() { return mGraph; }
		[[nodiscard]] ui::NodeTypeId taskType() const { return mTaskType; }
		[[nodiscard]] const ui::NodeGraph& graph() const { return mGraph; }

		ui::NodeId addTask(Task task, Vec2F position);
		bool addTaskWithId(ui::NodeId id, Task task, Vec2F position);
		[[nodiscard]] Task* task(ui::NodeId id);
		[[nodiscard]] const Task* task(ui::NodeId id) const;

		bool setParent(ui::NodeId child, ui::NodeId parent);
		void clearParent(ui::NodeId child);
		bool addDependency(ui::NodeId task, ui::NodeId dependsOn);
		void removeDependency(ui::NodeId task, ui::NodeId dependsOn);
		[[nodiscard]] bool canAddDependency(ui::NodeId task, ui::NodeId dependsOn) const;
		[[nodiscard]] bool canSetParent(ui::NodeId child, ui::NodeId parent) const;

		[[nodiscard]] std::optional<ui::NodeId> parentOf(ui::NodeId id) const;
		[[nodiscard]] std::vector<ui::NodeId> ancestorsOf(ui::NodeId id) const;
		[[nodiscard]] std::vector<ui::NodeId> childrenOf(ui::NodeId id) const;
		[[nodiscard]] std::vector<ui::NodeId> rootTasks() const;

		[[nodiscard]] std::vector<ui::NodeId> tasksInOrder() const;
		[[nodiscard]] int orderAfterLast() const;
		void placeBefore(ui::NodeId moved, ui::NodeId anchor);
		void placeAfter(ui::NodeId moved, ui::NodeId anchor);
		void placeLastAmongSiblings(ui::NodeId id);
		[[nodiscard]] std::vector<ui::NodeId> dependenciesOf(ui::NodeId id) const;
		[[nodiscard]] std::vector<ui::NodeId> dependentsOf(ui::NodeId id) const;
		[[nodiscard]] bool isRunnable(ui::NodeId id) const;

		[[nodiscard]] const std::vector<RGBA>& colorPalette() const { return mColorPalette; }
		void setColorPalette(std::vector<RGBA> colors) { mColorPalette = std::move(colors); }

		[[nodiscard]] static ui::PortRef input(ui::NodeId node, TaskPort port);
		[[nodiscard]] static ui::PortRef output(ui::NodeId node, TaskOutput port);

	private:
		[[nodiscard]] bool reaches(ui::NodeId from, ui::NodeId to, ualni outputIndex) const;
		[[nodiscard]] bool keepsGraphAcyclic(const ui::Link& link) const;
		void sortByOrder(std::vector<ui::NodeId>& ids) const;
		void moveInOrder(ui::NodeId moved, ui::NodeId anchor, size_t anchorOffset);

		ui::NodeGraph mGraph;
		ui::NodeTypeId mTaskType;
		std::vector<RGBA> mColorPalette{ kTaskColors.begin(), kTaskColors.end() };
	};
}
