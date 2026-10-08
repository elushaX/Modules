#pragma once

#include "Collapsible.hpp"
#include "Containers.hpp"
#include "Label.hpp"
#include "ReorderableList.hpp"
#include "SearchField.hpp"
#include "TaskAppearance.hpp"
#include "TaskBoard.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tp::taskboard {

	class TaskRowButton : public ui::Button {
	public:
		void setSwatch(const RGBA& color);
		[[nodiscard]] const RGBA& swatch() const { return mSwatch; }

	protected:
		[[nodiscard]] bool hasIcon() const override { return true; }
		void paintIcon(ui::Painter& painter, const RectF& box) override;

	private:
		RGBA mSwatch;
	};

	class TaskRow : public ui::Widget {
	public:
		TaskRow(TaskBoard& board, ui::NodeId id, bool removable);

		[[nodiscard]] ui::NodeId taskId() const { return mId; }
		[[nodiscard]] TaskRowButton& openButton() { return mOpen; }
		[[nodiscard]] ui::Button& removeButton() { return mRemove; }
		void refresh();

		void setOnOpened(std::function<void(ui::NodeId)> callback) { mOnOpened = std::move(callback); }
		void setOnPreviewed(std::function<void(ui::NodeId)> callback) { mOnPreviewed = std::move(callback); }
		void setOnRemoved(std::function<void(ui::NodeId)> callback) { mOnRemoved = std::move(callback); }

	private:
		TaskBoard& mBoard;
		ui::NodeId mId;
		TaskRowButton mOpen;
		ui::Button mRemove{ "\u2715" };
		std::function<void(ui::NodeId)> mOnOpened;
		std::function<void(ui::NodeId)> mOnPreviewed;
		std::function<void(ui::NodeId)> mOnRemoved;
	};

	class RelationSection : public ui::Collapsible {
	public:
		RelationSection(std::string title, std::string addText);

		[[nodiscard]] ui::Button& addButton() { return mAdd; }
		[[nodiscard]] ui::ReorderableList& list() { return mList; }
		[[nodiscard]] ui::SearchField& picker() { return mPicker; }
		void setCount(size_t count);
		void showPicker(bool shown);
		[[nodiscard]] bool isPickerShown() const { return mPicker.isVisible(); }

	private:
		ui::Button mAdd;
		ui::SearchField mPicker;
		ui::ReorderableList mList{ 3 };
	};

	class TaskDetails : public ui::Widget {
	public:
		explicit TaskDetails(TaskBoard& board);

		void showTask(std::optional<ui::NodeId> id);
		[[nodiscard]] std::optional<ui::NodeId> shownTask() const { return mShown; }

		RelationSection children{ "Children", "+ Add child" };
		RelationSection dependsOn{ "Depends on", "+ Add" };
		RelationSection unblocks{ "Unblocks", "+ Add" };

		[[nodiscard]] TaskRow* row(const RelationSection& section, ui::NodeId id) const;
		[[nodiscard]] const std::vector<std::unique_ptr<TaskRow>>& rowsOf(const RelationSection& section) const;

		void setOnChanged(std::function<void()> callback) { mOnChanged = std::move(callback); }
		void setOnOpened(std::function<void(ui::NodeId)> callback) { mOnOpened = std::move(callback); }
		void setOnPreviewed(std::function<void(ui::NodeId)> callback) { mOnPreviewed = std::move(callback); }
		void setOnTaskCreated(std::function<void(ui::NodeId)> callback) { mOnTaskCreated = std::move(callback); }

		void addChildTask();
		void refreshRelations();

	private:
		void buildLayout();
		void bindEditors();
		void fillSection(
			RelationSection& section,
			std::vector<std::unique_ptr<TaskRow>>& rows,
			const std::vector<ui::NodeId>& ids,
			bool removable
		);
		void childrenReordered(size_t from, size_t to);
		enum class Relation { Child, DependsOn, Unblocks };

		void bindPicker(RelationSection& section, Relation relation);
		[[nodiscard]] std::vector<ui::SearchResult> pickerResults(const std::string& query, Relation relation);
		[[nodiscard]] bool canLink(ui::NodeId other, Relation relation) const;
		[[nodiscard]] bool isLinked(ui::NodeId other, Relation relation) const;
		void picked(std::optional<ui::NodeId> existing, Relation relation, const std::string& query);
		void link(ui::NodeId other, Relation relation);
		void removeLink(ui::NodeId other, bool dependency);
		[[nodiscard]] ui::NodeId createTask(std::optional<ui::NodeId> parent);
		[[nodiscard]] std::vector<ui::NodeId> shownChildren() const;

		TaskBoard& mBoard;
		std::optional<ui::NodeId> mShown;
		std::vector<ui::NodeId> mPickerChoices;
		std::vector<std::unique_ptr<TaskRow>> mChildRows;
		std::vector<std::unique_ptr<TaskRow>> mDependencyRows;
		std::vector<std::unique_ptr<TaskRow>> mDependentRows;
		std::function<void()> mOnChanged;
		std::function<void(ui::NodeId)> mOnOpened;
		std::function<void(ui::NodeId)> mOnPreviewed;
		std::function<void(ui::NodeId)> mOnTaskCreated;
	};

	class TaskPage : public ui::Widget {
	public:
		explicit TaskPage(TaskBoard& board);

		void showTask(std::optional<ui::NodeId> id);
		[[nodiscard]] std::optional<ui::NodeId> shownTask() const { return mShown; }

		[[nodiscard]] TaskDetails& details() { return mDetails; }

		ui::Label rootHint{ "Top-level tasks. Add one with + Add child or Ctrl+N." };

		void setOnChanged(std::function<void()> callback) { mDetails.setOnChanged(std::move(callback)); }

	private:
		std::optional<ui::NodeId> mShown;
		TaskDetails mDetails;
	};
}
