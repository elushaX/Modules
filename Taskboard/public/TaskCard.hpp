#pragma once

#include "KanbanBoard.hpp"
#include "Label.hpp"
#include "TaskBoard.hpp"
#include "TaskStateChip.hpp"
#include "TreeView.hpp"

#include <optional>
#include <string>

namespace tp::taskboard {

	class TaskCardView : public ui::Widget {
	public:
		TaskCardView();

		void showTask(ui::NodeId id, const Task& task);
		void setStatusShown(bool shown);

		ui::Label number;
		TaskStateChip status;
		TaskStateChip session;
		ui::Label title;
		ui::Label description;

	private:
		bool mStatusShown = true;
		ui::Widget mHeader;
		ui::Spacer mHeaderSpacer;
	};

	class TaskTreeCard : public ui::TreeCard {
	public:
		explicit TaskTreeCard(ui::TreeItemId id);

		void showTask(const Task& task);
		[[nodiscard]] TaskCardView& view() { return mView; }

	private:
		TaskCardView mView;
	};

	class TaskKanbanCard : public ui::KanbanCard {
	public:
		explicit TaskKanbanCard(ui::KanbanItemId id);

		void showTask(const Task& task);
		[[nodiscard]] TaskCardView& view() { return mView; }

	private:
		TaskCardView mView;
	};
}
