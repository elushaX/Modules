#pragma once

#include "FileBrowser.hpp"
#include "KanbanBoard.hpp"
#include "TaskBoard.hpp"

#include <string>
#include <vector>

namespace tp::taskboard {

	class KanbanColumnSettings {
	public:
		KanbanColumnSettings(ui::FileSystem& files, const std::string& storageDirectory);

		[[nodiscard]] const std::vector<TaskStatus>& order() const { return mOrder; }
		void moveColumn(size_t from, size_t to);
		[[nodiscard]] const std::vector<TaskStatus>& hidden() const { return mHidden; }
		void setVisible(TaskStatus status, bool visible);
		[[nodiscard]] const std::string& storagePath() const { return mPath; }

		[[nodiscard]] static std::vector<TaskStatus> orderByDefault();
		[[nodiscard]] static std::vector<TaskStatus> hiddenByDefault();

	private:
		void load();
		void store();

		ui::FileSystem& mFiles;
		std::string mPath;
		std::vector<TaskStatus> mOrder;
		std::vector<TaskStatus> mHidden;
	};

	class TaskKanbanModel : public ui::KanbanModel {
	public:
		TaskKanbanModel(const TaskBoard& board, const KanbanColumnSettings& settings) :
			mBoard(board),
			mSettings(settings) {}

		[[nodiscard]] std::vector<ui::KanbanColumn> columns() const;
		[[nodiscard]] TaskStatus statusOf(size_t column) const { return mSettings.order()[column]; }
		[[nodiscard]] size_t columnOf(TaskStatus status) const;

		[[nodiscard]] std::vector<ui::KanbanItemId> items(size_t column) const override;

	private:
		const TaskBoard& mBoard;
		const KanbanColumnSettings& mSettings;
	};

	void placeMovedCard(TaskBoard& board, const TaskKanbanModel& model, const ui::KanbanMove& move);
}
