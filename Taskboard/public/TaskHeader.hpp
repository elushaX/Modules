#pragma once

#include "ColorPicker.hpp"
#include "Containers.hpp"
#include "Dialog.hpp"
#include "Label.hpp"
#include "Menus.hpp"
#include "SearchField.hpp"
#include "TaskBoard.hpp"
#include "TaskStateChip.hpp"
#include "TaskStore.hpp"
#include "TextField.hpp"

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace tp::taskboard {

	using EditingCallback = std::function<void(std::optional<std::pair<ui::NodeId, TaskField>>)>;

	class TaskTitleField : public ui::TextField {
	public:
		TaskTitleField();

		void showTitle(const std::string& title);
	};

	class TaskHeader : public ui::Widget {
	public:
		explicit TaskHeader(TaskBoard& board);

		void showTask(std::optional<ui::NodeId> id);
		[[nodiscard]] std::optional<ui::NodeId> shownTask() const { return mShown; }
		void showConflicts(const std::vector<std::string>& summaries);
		void showParentPicker();
		void askDelete();

		[[nodiscard]] ui::Widget& badgeRow() { return mBadgeRow; }
		void setOnBack(std::function<void()> callback);

		ui::Button back{ "\u2190 Back" };
		ui::Label idLabel;
		TaskTitleField title;
		ui::Dropdown status;
		ui::ColorPickerButton color;
		TaskStateChip badge;
		ui::MenuIconButton actionsButton;
		ui::Menu actionsMenu;
		ui::SearchField parentPicker;
		ui::ConfirmDialog deleteConfirm;
		ui::Label conflictText;
		ui::Button keepMine{ "Keep mine" };
		ui::Button useTheirs{ "Use theirs" };

		void setOnChanged(std::function<void()> callback) { mOnChanged = std::move(callback); }
		void setOnEditingChanged(EditingCallback callback) { mOnEditingChanged = std::move(callback); }
		void setOnConflictChoice(std::function<void(bool keepMine)> callback) { mOnConflictChoice = std::move(callback); }
		void setOnDeleteRequested(std::function<void()> callback) { mOnDeleteRequested = std::move(callback); }
		void setOnParentChanged(std::function<void()> callback) { mOnParentChanged = std::move(callback); }

	private:
		void buildBadgeRow();
		void buildConflictBanner();
		void bindControls();
		void bindActions();
		[[nodiscard]] std::vector<ui::SearchResult> parentResults(const std::string& query);
		void parentPicked(std::optional<ui::NodeId> parent);
		void change(const std::function<void(Task&)>& edit);
		void refresh(const Task& task);

		TaskBoard& mBoard;
		std::optional<ui::NodeId> mShown;
		bool mLoading = false;
		std::vector<std::optional<ui::NodeId>> mParentChoices;
		ui::Widget mBadgeRow;
		ui::Spacer mBadgeSpacer;
		ui::Panel mConflictBanner{ ui::Panel::Surface::Raised };
		ui::Widget mConflictActions;
		std::function<void()> mOnChanged;
		EditingCallback mOnEditingChanged;
		std::function<void()> mOnDeleteRequested;
		std::function<void()> mOnParentChanged;
		std::function<void(bool)> mOnConflictChoice;
		std::function<void()> mOnBack;
	};
}
