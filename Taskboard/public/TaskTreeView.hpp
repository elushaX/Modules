#pragma once

#include "Breadcrumb.hpp"
#include "Button.hpp"
#include "Collapsible.hpp"
#include "ColorPicker.hpp"
#include "Containers.hpp"
#include "Dialog.hpp"
#include "KanbanBoard.hpp"
#include "Docking.hpp"
#include "Control.hpp"
#include "Label.hpp"
#include "Menus.hpp"
#include "TreeView.hpp"
#include "PageStack.hpp"
#include "ReorderableList.hpp"
#include "SearchField.hpp"
#include "SecretsPanel.hpp"
#include "SessionPanel.hpp"
#include "Shortcut.hpp"
#include "SkillsPanel.hpp"
#include "TaskBoard.hpp"
#include "TaskCard.hpp"
#include "TaskKanban.hpp"
#include "TaskPage.hpp"
#include "TaskPanels.hpp"
#include "TaskStore.hpp"
#include "TaskTimeline.hpp"
#include "TaskWidget.hpp"
#include "TextField.hpp"
#include "Views.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tp::taskboard {

	class TaskTreeView : public ui::Panel {
	public:
		explicit TaskTreeView(
			TaskBoard& board, ui::FileSystem& files = ui::FileSystem::local(), const AppStorage& storage = taskboardStorage(),
			std::string skillsDirectory = defaultSkillsDirectory()
		);

		[[nodiscard]] static std::string defaultSkillsDirectory();

		[[nodiscard]] TaskWidget& taskWidget() { return mTaskWidget; }
		[[nodiscard]] TaskPage& page() { return mTaskWidget.page; }
		[[nodiscard]] TaskHeader& header() { return mTaskWidget.description.header; }
		[[nodiscard]] TaskDetails& details() { return mTaskWidget.page.details(); }
		[[nodiscard]] SessionTab& session() { return mTaskWidget.session; }
		[[nodiscard]] ui::SearchField& search() { return mSearch; }
		[[nodiscard]] ui::PageStack& pages() { return mTaskWidget.taskPages; }
		[[nodiscard]] ui::Breadcrumb& breadcrumb() { return mBreadcrumb; }
		[[nodiscard]] ui::TreeView& graph() { return mGraph; }
		[[nodiscard]] ui::KanbanBoard& kanban() { return mKanban; }
		[[nodiscard]] size_t kanbanColumn(TaskStatus status) const { return mKanbanModel.columnOf(status); }
		[[nodiscard]] SkillsPanel& skills() { return mSkills; }
		[[nodiscard]] SecretsPanel& secrets() { return mSecrets; }
		[[nodiscard]] ui::TabView& views() { return mViews; }
		[[nodiscard]] ui::DockArea& dock() { return mDock; }
		[[nodiscard]] ui::DockPanel& taskPanel() { return mTaskPanel; }
		[[nodiscard]] ui::ConfirmDialog& graphDeleteConfirm() { return mGraphDeleteConfirm; }
		[[nodiscard]] TaskDescriptionPanel& description() { return mTaskWidget.description; }
		[[nodiscard]] TaskWorkspacePanel& workspace() { return mTaskWidget.workspace; }
		[[nodiscard]] ui::ScrollView& sessionScroll() { return mTaskWidget.sessionScroll; }
		[[nodiscard]] ui::FullViewFrame* zoomFrame() const { return mZoomFrame; }
		[[nodiscard]] TaskTimeline& timeline() { return mTimeline; }
		void setReadOnly(bool readOnly);
		void showTimeline(bool shown);
		[[nodiscard]] bool isTimelineShown() const { return mTimeline.isVisible(); }
		[[nodiscard]] bool isReadOnly() const { return mReadOnly; }
		[[nodiscard]] bool isInGraph(ui::NodeId id) const;
		[[nodiscard]] std::optional<ui::NodeId> currentTask() const { return mTaskWidget.page.shownTask(); }

		void openTask(std::optional<ui::NodeId> id);
		void createTask();
		void setStore(TaskStore* store);
		void setSecretsStore(SecretsStore* store) { mSecrets.setStore(store); }
		void setAgentOptions(AgentOptions options) { session().setAgentOptions(std::move(options)); }
		void showConflicts(const std::vector<std::string>& summaries);
		void showSaveState(size_t unsavedChanges, bool saving);
		void boardReloaded();
		void attachMenus(ui::MenuBar& menuBar, std::function<void()> quit);
		void openPanels();
		void openTaskPanel();

		void setOnBoardEdited(std::function<void()> callback) { mOnBoardEdited = std::move(callback); }
		void setOnSave(std::function<void()> callback) { mOnSave = std::move(callback); }
		void setOnOpenSession(std::function<void(ui::NodeId)> callback);

		ui::Menu fileMenu;
		ui::Menu editMenu;
		ui::Menu viewMenu;
		ui::Label status;

		[[nodiscard]] std::vector<ui::SearchResult> searchResults(const std::string& query);

	protected:
		void onEvent(ui::Event& event) override;

	private:
		enum class BoardCommand { NewTask, OpenSession, Save, Search, DeleteTask };

		void buildMenus();
		void buildGraph();
		void buildKanban();
		void bindTaskWidget();
		void updateKanbanCard(ui::KanbanCard& card) const;
		void kanbanMoved(const ui::KanbanMove& move);
		void refreshKanban();
		void runBoardCommand(BoardCommand command);
		void graphSelectionChanged();
		void currentPageChanged();
		void reportEditing(std::optional<std::pair<ui::NodeId, TaskField>> editing);
		enum class SelectionSource { Board, Graph, Kanban };
		void showFullView(ui::FullViewFrame& frame, std::optional<ui::NodeId> id, SelectionSource source);
		void zoomInto(ui::FullViewFrame& frame);
		void zoomOutOf(ui::FullViewFrame& frame);
		void leaveZoom();
		void refreshZoomAccent();

		void focusTask(ui::NodeId id, SelectionSource source);
		void syncSelection(std::optional<ui::NodeId> id);
		void mainViewChanged(size_t index);
		void focusCreatedTask(ui::NodeId id);
		void createChildTask(ui::NodeId parent);
		void reparentTask(ui::NodeId child, ui::NodeId parent);
		[[nodiscard]] std::unique_ptr<ui::TreeCard> createCard(ui::TreeItemId id);
		void updateCard(ui::TreeCard& card) const;
		void refreshGraph();
		void showPath(const std::vector<std::optional<ui::NodeId>>& path);
		void pathChosen(size_t index);
		void deleteCurrentTask();
		void askDeleteTasks(const std::vector<ui::NodeId>& ids);
		void deleteTasks(const std::vector<ui::NodeId>& ids);
		void focusMainView();
		[[nodiscard]] std::vector<ui::NodeId> selectedTasks() const;
		void boardEdited();
		void refreshSummary();
		[[nodiscard]] std::vector<std::optional<ui::NodeId>> pathTo(std::optional<ui::NodeId> id) const;
		[[nodiscard]] std::string pageTitle(std::optional<ui::NodeId> id) const;

		TaskBoard& mBoard;
		TaskStore* mStore = nullptr;
		ui::SearchField mSearch;
		std::vector<ui::NodeId> mSearchChoices;
		class GraphModel : public ui::TreeModel {
		public:
			explicit GraphModel(const TaskTreeView& view) :
				mView(view) {}

			[[nodiscard]] std::vector<ui::TreeItemId> roots() const override;
			[[nodiscard]] std::vector<ui::TreeItemId> children(ui::TreeItemId id) const override;
			[[nodiscard]] std::vector<std::pair<ui::TreeItemId, ui::TreeItemId>> crossLinks() const override;
			[[nodiscard]] bool canReparent(ui::TreeItemId child, ui::TreeItemId parent) const override;

		private:
			const TaskTreeView& mView;
		};

		SelectionSource mSelectionSource = SelectionSource::Board;
		bool mSyncingSelection = false;
		GraphModel mGraphModel{ *this };
		ui::TreeView mGraph{ mGraphModel };
		KanbanColumnSettings mKanbanColumns;
		TaskKanbanModel mKanbanModel{ mBoard, mKanbanColumns };
		ui::KanbanBoard mKanban{ mKanbanModel };
		SkillsPanel mSkills;
		SecretsPanel mSecrets;
		ui::TabView mViews;
		ui::DockArea mDock;
		ui::DockPanel mTaskPanel{ "Task", ui::DockSide::Right };
		TaskWidget mTaskWidget;
		ui::FullViewFrame* mZoomFrame = nullptr;
		TaskCardView mZoomPreview;
		bool mTaskPanelOpenBeforeZoom = false;
		ui::Widget mPathArea;
		ui::Widget mPathBar;
		ui::Breadcrumb mBreadcrumb;
		std::vector<std::optional<ui::NodeId>> mPath;
		ui::ShortcutMap<BoardCommand> mBoardShortcuts;
		ui::ConfirmDialog mGraphDeleteConfirm;
		TaskTimeline mTimeline;
		bool mReadOnly = false;
		ui::Panel mStatusArea{ ui::Panel::Surface::None };
		ui::Label mSaveState;
		std::function<void()> mQuit;
		std::function<void()> mOnBoardEdited;
		std::function<void()> mOnSave;
		std::function<void(ui::NodeId)> mOnOpenSession;
	};
}
