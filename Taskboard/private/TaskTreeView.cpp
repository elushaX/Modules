#include "TaskTreeView.hpp"

#include "BoxLayout.hpp"
#include "StackLayout.hpp"
#include "TaskCard.hpp"
#include "TaskSearch.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <cstdlib>

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

namespace {
	constexpr halnf kTaskPanelWidth = 440;
	constexpr halnf kPathBarPadding = 6;
	constexpr halnf kPathBarMargin = 6;
	constexpr halnf kTaskCardWidth = 300;
	constexpr halnf kTaskCardHeight = 132;
	constexpr size_t kTreeTab = 0;
	constexpr size_t kKanbanTab = 1;
	constexpr halnf kSearchWidth = 280;
	constexpr halnf kSearchResultsWidth = 560;
	constexpr size_t kSnippetRadius = 36;
	constexpr const char* kBoardTitle = "Board";

	std::string singleLine(std::string text) {
		std::replace(text.begin(), text.end(), '\n', ' ');
		return text;
	}

	std::string snippetAround(const std::string& text, size_t at, size_t length) {
		const size_t start = at > kSnippetRadius ? at - kSnippetRadius : 0;
		const size_t end = std::min(text.size(), at + length + kSnippetRadius);
		return (start > 0 ? "\u2026" : "") + singleLine(text.substr(start, end - start)) +
					 (end < text.size() ? "\u2026" : "");
	}

}

std::string TaskTreeView::defaultSkillsDirectory() {
#ifdef TASKBOARD_SKILLS_DIR
	if (const char* override = std::getenv("TASKBOARD_SKILLS_DIR")) return override;
	return TASKBOARD_SKILLS_DIR;
#else
	const char* override = std::getenv("TASKBOARD_SKILLS_DIR");
	return override ? override : std::string();
#endif
}

TaskTreeView::TaskTreeView(
	TaskBoard& board, FileSystem& files, const AppStorage& storage, std::string skillsDirectory
) :
	Panel(Surface::Window),
	mBoard(board),
	mKanbanColumns(files, storage.root()),
	mSkills(std::move(skillsDirectory), files),
	mTaskWidget(board, files, storage) {
	setDebugName("task tree view");
	setSizing(AxisSizing::fill(), AxisSizing::fill());
	emplaceLayout<ColumnLayout>();

	mBoardShortcuts.bind(ctrl(InputID::N), BoardCommand::NewTask);
	mBoardShortcuts.bind(ctrl(InputID::O), BoardCommand::OpenSession);
	mBoardShortcuts.bind(ctrl(InputID::S), BoardCommand::Save);
	mBoardShortcuts.bind(ctrl(InputID::F), BoardCommand::Search);
	buildMenus();

	mSearch.setPlaceholder("Search tasks  (Ctrl+F)");
	mSearch.setSizing(AxisSizing::preferred(kSearchWidth), AxisSizing::fit());
	mSearch.setMinimumResultsWidth(kSearchResultsWidth);
	mSearch.setQuery([this](const std::string& query) { return searchResults(query); });
	mSearch.setOnChosen([this](size_t index) {
		if (index < mSearchChoices.size()) openTask(mSearchChoices[index]);
		mSearch.setText("");
	});

	mBreadcrumb.setOnPartChosen([this](size_t index) { pathChosen(index); });

	bindTaskWidget();
	mBoard.graph().setOnChanged([this] { boardEdited(); });

	status.setTone(Label::Tone::Muted);
	status.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mStatusArea.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mStatusArea.emplaceLayout<RowLayout>().setPadding(Insets::symmetric(10, 3));
	mStatusArea.addChild(status);
	mSaveState.setSizing(AxisSizing::fit(), AxisSizing::fit());
	mStatusArea.addChild(mSaveState);

	buildGraph();
	addChild(mDock);
	addChild(mTimeline);
	mTimeline.setVisible(false);
	addChild(mPathArea);
	addChild(mStatusArea);
	showPath({ std::nullopt });
	refreshSummary();
}

void TaskTreeView::attachMenus(MenuBar& menuBar, std::function<void()> quit) {
	mQuit = std::move(quit);
	menuBar.addMenu("File", fileMenu);
	menuBar.addMenu("Edit", editMenu);
	menuBar.addMenu("View", viewMenu);
	menuBar.setCenterWidget(&mSearch);
}

void TaskTreeView::buildGraph() {
	mGraph.style().layout.card = Vec2F(kTaskCardWidth, kTaskCardHeight);
	mGraph.setCardFactory([this](TreeItemId id) { return createCard(id); });
	mGraph.setCardUpdater([this](TreeCard& card) { updateCard(card); });
	mGraph.setOnSelected([this](TreeItemId) { graphSelectionChanged(); });
	mGraph.setOnFullViewChanged([this](std::optional<TreeItemId> item) {
		showFullView(mGraph.fullViewFrame(), item, SelectionSource::Graph);
	});
	mGraph.setOnCreateChild([this](TreeItemId parent) { createChildTask(parent); });
	mGraph.setOnReparent([this](TreeItemId child, TreeItemId parent) { reparentTask(child, parent); });
	mGraph.setOnDeleteRequested([this](const std::vector<TreeItemId>&) { askDeleteTasks(selectedTasks()); });

	mPathBar.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mPathBar.emplaceLayout<StackLayout>().setPadding(Insets::symmetric(kPathBarPadding, 0));
	mPathBar.addChild(mBreadcrumb);
	mPathArea.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mPathArea.emplaceLayout<StackLayout>().setPadding(Insets::uniform(kPathBarMargin));
	mPathArea.addChild(mPathBar);
	mGraph.setSizing(AxisSizing::fill(), AxisSizing::fill());

	mTaskPanel.setContent(&mTaskWidget);
	mTaskPanel.setDockedLength(kTaskPanelWidth);
	mTaskPanel.setOnOpenChanged([this](bool open) {
		if (open) leaveZoom();
	});

	buildKanban();
	mViews.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mViews.setTabAlignment(TabView::TabAlignment::Center);
	mViews.addTab("Tree", mGraph);
	mViews.addTab("Kanban", mKanban);
	mViews.addTab("Skills", mSkills);
	mViews.addTab("Secrets", mSecrets);
	mViews.setCurrentIndex(kTreeTab);
	mViews.setOnChanged([this](size_t index) { mainViewChanged(index); });

	mDock.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mDock.setCenter(&mViews);
	mDock.addPanel(mTaskPanel);
	mDock.setViewMode(mTaskPanel, DockPanel::ViewMode::Undocked);
	mDock.hidePanel(mTaskPanel);
}

void TaskTreeView::buildKanban() {
	mKanban.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mKanban.setColumnWidthRange(kTaskCardWidth, kUnbounded);
	mKanban.setCardHeight(kTaskCardHeight);
	mKanban.setCardFactory([](KanbanItemId id) { return std::make_unique<TaskKanbanCard>(id); });
	mKanban.setCardUpdater([this](KanbanCard& card) { updateKanbanCard(card); });
	mKanban.setOnSelected([this](KanbanItemId id) {
		if (NodeId(id) != currentTask()) focusTask(id, SelectionSource::Kanban);
	});
	mKanban.setOnActivated([this](KanbanItemId id) { focusTask(id, SelectionSource::Kanban); });
	mKanban.setOnMoved([this](const KanbanMove& move) { kanbanMoved(move); });
	mKanban.setOnDeleteRequested([this](KanbanItemId id) { askDeleteTasks({ NodeId(id) }); });
	mKanban.setOnFullViewChanged([this](std::optional<KanbanItemId> item) {
		showFullView(mKanban.fullViewFrame(), item, SelectionSource::Kanban);
	});
	mKanban.setOnColumnVisibilityChanged([this](size_t column, bool visible) {
		mKanbanColumns.setVisible(mKanbanModel.statusOf(column), visible);
	});
	mKanban.setOnColumnMoved([this](size_t from, size_t to) { mKanbanColumns.moveColumn(from, to); });
	mKanban.setColumns(mKanbanModel.columns());
}

void TaskTreeView::updateKanbanCard(KanbanCard& card) const {
	const Task* task = mBoard.task(card.id());
	auto* taskCard = dynamic_cast<TaskKanbanCard*>(&card);
	if (task && taskCard) taskCard->showTask(*task);
}

void TaskTreeView::kanbanMoved(const KanbanMove& move) {
	Task* task = mBoard.task(move.item);
	if (!task) return;
	if (mReadOnly) {
		refreshKanban();
		return;
	}

	placeMovedCard(mBoard, mKanbanModel, move);
	boardEdited();
}

void TaskTreeView::refreshKanban() {
	mKanban.modelChanged();
	mKanban.refreshCards();
}

void TaskTreeView::openPanels() { openTaskPanel(); }

void TaskTreeView::openTaskPanel() {
	if (mZoomFrame) {
		mTaskPanelOpenBeforeZoom = true;
		return;
	}
	mDock.openPanel(mTaskPanel);
}

void TaskTreeView::reportEditing(std::optional<std::pair<NodeId, TaskField>> editing) {
	if (!mStore) return;
	if (editing) mStore->beginEditing(editing->first, editing->second);
	else mStore->endEditing();
}

bool TaskTreeView::isInGraph(NodeId id) const { return mBoard.task(id) != nullptr; }

std::vector<TreeItemId> TaskTreeView::GraphModel::roots() const {
	const std::vector<NodeId> roots = mView.mBoard.rootTasks();
	return { roots.begin(), roots.end() };
}

std::vector<TreeItemId> TaskTreeView::GraphModel::children(TreeItemId id) const {
	const std::vector<NodeId> children = mView.mBoard.childrenOf(id);
	return { children.begin(), children.end() };
}

std::vector<std::pair<TreeItemId, TreeItemId>> TaskTreeView::GraphModel::crossLinks() const {
	std::vector<std::pair<TreeItemId, TreeItemId>> links;
	for (const Node& node : mView.mBoard.graph().nodes())
		for (NodeId dependency : mView.mBoard.dependenciesOf(node.id))
			links.emplace_back(dependency, node.id);
	return links;
}

bool TaskTreeView::GraphModel::canReparent(TreeItemId child, TreeItemId parent) const {
	return mView.mBoard.canSetParent(child, parent);
}

void TaskTreeView::refreshGraph() {
	mGraph.modelChanged();
	mGraph.refreshCards();
}

std::vector<NodeId> TaskTreeView::selectedTasks() const {
	const std::vector<TreeItemId> items = mGraph.selectedItems();
	return { items.begin(), items.end() };
}

void TaskTreeView::graphSelectionChanged() {
	refreshSummary();
	const std::optional<TreeItemId> selection = mGraph.selection();
	if (mSyncingSelection || !selection || NodeId(*selection) == currentTask()) return;
	focusTask(*selection, SelectionSource::Graph);
}

void TaskTreeView::focusTask(NodeId id, SelectionSource source) {
	mSelectionSource = source;
	openTask(id);
	mSelectionSource = SelectionSource::Board;
}

void TaskTreeView::syncSelection(std::optional<NodeId> id) {
	const SelectionSource source = mSelectionSource;
	mSyncingSelection = true;
	if (source != SelectionSource::Graph && id) mGraph.select(*id);
	if (source != SelectionSource::Kanban) mKanban.select(id ? std::optional<KanbanItemId>(*id) : std::nullopt);
	mSyncingSelection = false;
}

void TaskTreeView::bindTaskWidget() {
	const auto edited = [this] { boardEdited(); };
	const auto editing = [this](std::optional<std::pair<NodeId, TaskField>> field) { reportEditing(field); };
	TaskPage& page = mTaskWidget.page;

	mTaskWidget.taskPages.setOnPageShown([this](StackPage& stackPage) {
		mPath.resize(mTaskWidget.taskPages.depth());
		stackPage.setContent(mTaskWidget.page);
		currentPageChanged();
	});

	page.setOnChanged(edited);
	page.details().setOnOpened([this](NodeId id) { openTask(id); });
	page.details().setOnPreviewed([this](NodeId id) { mGraph.centerOn(id); });
	page.details().setOnTaskCreated([this](NodeId id) { focusCreatedTask(id); });
	header().setOnDeleteRequested([this] { deleteCurrentTask(); });
	header().setOnParentChanged([this] { showPath(pathTo(currentTask())); });

	mTaskWidget.description.setOnChanged(edited);
	mTaskWidget.description.setOnEditingChanged(editing);
	mTaskWidget.session.setOnChanged(edited);
}

void TaskTreeView::showFullView(FullViewFrame& frame, std::optional<NodeId> id, SelectionSource source) {
	if (!id) {
		zoomOutOf(frame);
		return;
	}
	zoomInto(frame);
	if (*id != currentTask()) focusTask(*id, source);
}

void TaskTreeView::zoomInto(FullViewFrame& frame) {
	if (mZoomFrame == &frame) return;
	if (mZoomFrame) mZoomFrame->setContent(nullptr);
	else mTaskPanelOpenBeforeZoom = mTaskPanel.isOpen();

	mZoomFrame = &frame;
	mDock.hidePanel(mTaskPanel);
	frame.setPreview(&mZoomPreview);
	frame.setContent(&mTaskWidget);
	header().setOnBack([&frame] { frame.requestBack(); });
	refreshZoomAccent();
}

void TaskTreeView::refreshZoomAccent() {
	if (!mZoomFrame) return;
	const std::optional<NodeId> current = currentTask();
	const Task* task = current ? mBoard.task(*current) : nullptr;
	mZoomFrame->setAccent(task ? std::optional<RGBA>(taskAccentColor(*task)) : std::nullopt);
	if (task) mZoomPreview.showTask(*current, *task);
}

void TaskTreeView::leaveZoom() {
	if (!mZoomFrame) return;
	mTaskPanelOpenBeforeZoom = true;
	if (mZoomFrame == &mGraph.fullViewFrame()) mGraph.leaveFullView();
	else mKanban.leaveFullView();
}

void TaskTreeView::zoomOutOf(FullViewFrame& frame) {
	if (mZoomFrame != &frame) return;

	header().setOnBack(nullptr);
	frame.setContent(nullptr);
	mZoomFrame = nullptr;
	mTaskPanel.setContent(&mTaskWidget);
	if (mTaskPanelOpenBeforeZoom) mDock.openPanel(mTaskPanel);
}

void TaskTreeView::mainViewChanged(size_t index) {
	if (index != kTreeTab && index != kKanbanTab) {
		mGraph.leaveFullView(false);
		mKanban.leaveFullView(false);
		return;
	}
	const bool showsGraph = index == kTreeTab;
	if (showsGraph) mKanban.leaveFullView(false);
	else mGraph.leaveFullView(false);
	const std::optional<TreeItemId> selection = mGraph.selection();
	if (showsGraph && selection) mGraph.centerOn(*selection, false);
}

void TaskTreeView::focusCreatedTask(NodeId id) {
	openTask(id);
	mTaskWidget.showPage(TaskWidgetPage::Task);
	if (ui()) ui()->setKeyboardFocus(&header().title);
	header().title.selectAll();
}

void TaskTreeView::createChildTask(NodeId parent) {
	if (mReadOnly) return;
	Task task;
	if (const Task* source = mBoard.task(parent)) {
		task.agentModel = source->agentModel;
		task.harness = source->harness;
		task.effort = source->effort;
		task.permissionMode = source->permissionMode;
	}
	task.order = mBoard.orderAfterLast();
	const NodeId created = mBoard.addTask(std::move(task), { 0, 0 });
	mBoard.setParent(created, parent);
	boardEdited();
	mGraph.setOpen(parent, true);
	focusCreatedTask(created);
}

void TaskTreeView::reparentTask(NodeId child, NodeId parent) {
	if (mReadOnly) return;
	if (!mBoard.canSetParent(child, parent)) return;
	if (!mBoard.setParent(child, parent)) return;
	mBoard.placeLastAmongSiblings(child);
	boardEdited();
}

std::unique_ptr<TreeCard> TaskTreeView::createCard(TreeItemId id) { return std::make_unique<TaskTreeCard>(id); }

void TaskTreeView::updateCard(TreeCard& card) const {
	const Task* task = mBoard.task(card.id());
	auto* taskCard = dynamic_cast<TaskTreeCard*>(&card);
	if (task && taskCard) taskCard->showTask(*task);
}

void TaskTreeView::buildMenus() {
	auto item = [this](Menu& menu, std::string text, BoardCommand command) {
		menu.addItem(std::move(text), [this, command] { runBoardCommand(command); }, mBoardShortcuts.textFor(command));
	};
	item(fileMenu, "New Task", BoardCommand::NewTask);
	item(fileMenu, "Open Session", BoardCommand::OpenSession);
	item(fileMenu, "Save", BoardCommand::Save);
	fileMenu.addSeparator();
	fileMenu.addItem("Quit", [this] {
		if (mQuit) mQuit();
	});

	item(editMenu, "Find Task", BoardCommand::Search);
	editMenu.addSeparator();
	item(editMenu, "Delete Task", BoardCommand::DeleteTask);

	viewMenu.addToggle("Timeline", [this] { return isTimelineShown(); }, [this] { showTimeline(!isTimelineShown()); });
}

void TaskTreeView::runBoardCommand(BoardCommand command) {
	switch (command) {
		case BoardCommand::NewTask: createTask(); break;
		case BoardCommand::Save:
			if (mOnSave) mOnSave();
			break;
		case BoardCommand::OpenSession:
			if (currentTask() && mOnOpenSession) mOnOpenSession(*currentTask());
			break;
		case BoardCommand::Search: mSearch.focus(); break;
		case BoardCommand::DeleteTask:
			if (mReadOnly) break;
			if (mGraph.selectedItems().size() > 1) askDeleteTasks(selectedTasks());
			else header().askDelete();
			break;
	}
}

void TaskTreeView::onEvent(Event& event) {
	const ShortcutMap<BoardCommand>::Binding* binding = mBoardShortcuts.find(event);
	if (!binding) return;

	runBoardCommand(binding->command);
	event.accept();
}

void TaskTreeView::createTask() {
	if (!mReadOnly) details().addChildTask();
}

void TaskTreeView::deleteCurrentTask() {
	if (const std::optional<NodeId> current = currentTask()) deleteTasks({ *current });
}

void TaskTreeView::askDeleteTasks(const std::vector<NodeId>& ids) {
	if (mReadOnly || ids.empty() || !ui()) return;
	if (ids.size() == 1) {
		if (ids.front() != currentTask()) focusTask(ids.front(), SelectionSource::Graph);
		header().askDelete();
		return;
	}
	std::string message = "Delete " + std::to_string(ids.size()) + " tasks?";
	message += " Their children move up to the nearest remaining parent.";
	mGraphDeleteConfirm.ask(*ui(), "Delete tasks", message, "Delete", [this, ids] { deleteTasks(ids); });
}

void TaskTreeView::deleteTasks(const std::vector<NodeId>& ids) {
	const std::unordered_set<NodeId> removed(ids.begin(), ids.end());
	const auto survivingParent = [&](NodeId id) {
		std::optional<NodeId> parent = mBoard.parentOf(id);
		while (parent && removed.contains(*parent))
			parent = mBoard.parentOf(*parent);
		return parent;
	};

	std::vector<std::pair<NodeId, std::optional<NodeId>>> moves;
	for (NodeId id : ids)
		for (NodeId child : mBoard.childrenOf(id))
			if (!removed.contains(child)) moves.emplace_back(child, survivingParent(child));
	std::optional<NodeId> next = currentTask();
	if (next && removed.contains(*next)) next = survivingParent(*next);

	for (const auto& [child, parent] : moves) {
		if (parent) mBoard.setParent(child, *parent);
		else mBoard.clearParent(child);
	}
	mBoard.graph().removeNodes(ids);
	openTask(next);
	focusMainView();
}

void TaskTreeView::focusMainView() {
	if (!ui()) return;
	const size_t index = size_t(mViews.currentIndex());
	if (index != kTreeTab && index != kKanbanTab) return;
	Widget& view = index == kTreeTab ? static_cast<Widget&>(mGraph) : static_cast<Widget&>(mKanban);
	ui()->setKeyboardFocus(&view);
}

std::vector<std::optional<NodeId>> TaskTreeView::pathTo(std::optional<NodeId> id) const {
	std::vector<std::optional<NodeId>> path = { std::nullopt };
	if (!id || !mBoard.task(*id)) return path;

	for (NodeId ancestor : mBoard.ancestorsOf(*id))
		path.emplace_back(ancestor);
	path.emplace_back(*id);
	return path;
}

std::string TaskTreeView::pageTitle(std::optional<NodeId> id) const {
	const Task* task = id ? mBoard.task(*id) : nullptr;
	return task ? task->title : kBoardTitle;
}

void TaskTreeView::openTask(std::optional<NodeId> id) {
	const std::vector<std::optional<NodeId>> path = pathTo(id);
	size_t shared = 0;
	while (shared < path.size() && shared < mPath.size() && path[shared] == mPath[shared])
		shared++;

	if (shared == path.size() && shared == mPath.size()) return;
	if (shared == path.size()) {
		pages().backTo(shared);
		return;
	}
	if (shared == 0) {
		showPath(path);
		return;
	}

	if (shared < pages().depth()) pages().backTo(shared);
	for (size_t index = shared; index < path.size(); index++) {
		mPath.push_back(path[index]);
		pages().push(pageTitle(path[index]), page());
	}
	currentPageChanged();
}

void TaskTreeView::showPath(const std::vector<std::optional<NodeId>>& path) {
	pages().clear();
	mPath.clear();
	for (const std::optional<NodeId>& id : path) {
		mPath.push_back(id);
		pages().push(pageTitle(id), page());
	}
	currentPageChanged();
}

void TaskTreeView::currentPageChanged() {
	mTaskWidget.showTask(mPath.back());
	refreshZoomAccent();
	mBreadcrumb.setParts(pages().titles());
	syncSelection(mPath.back());
}

void TaskTreeView::pathChosen(size_t index) { pages().backTo(index + 1); }

std::vector<SearchResult> TaskTreeView::searchResults(const std::string& query) {
	mSearchChoices.clear();
	std::vector<SearchResult> results;
	if (query.empty()) return results;

	const std::string needle = lowercaseText(query);
	const std::optional<NodeId> wantedId = taskIdQuery(query);
	std::vector<std::pair<int, NodeId>> ranked;
	for (const Node& node : mBoard.graph().nodes()) {
		const Task* task = mBoard.task(node.id);
		if (!task) continue;
		if (wantedId && node.id == *wantedId) ranked.emplace_back(0, node.id);
		else if (lowercaseText(task->title).find(needle) != std::string::npos) ranked.emplace_back(1, node.id);
		else if (lowercaseText(task->description).find(needle) != std::string::npos) ranked.emplace_back(2, node.id);
	}
	std::sort(ranked.begin(), ranked.end());

	for (const auto& [rank, id] : ranked) {
		const Task& task = *mBoard.task(id);
		std::string where;
		for (NodeId ancestor : mBoard.ancestorsOf(id))
			where += (where.empty() ? "" : " \u203A ") + mBoard.task(ancestor)->title;
		std::string detail = where.empty() ? std::string("top level") : where;
		if (rank == 2) {
			const size_t at = lowercaseText(task.description).find(needle);
			detail += "   \"" + snippetAround(task.description, at, needle.size()) + "\"";
		}
		mSearchChoices.push_back(id);
		results.push_back({ taskLabel(id, task), std::move(detail), true });
	}
	return results;
}

void TaskTreeView::setStore(TaskStore* store) {
	mStore = store;
	mTaskWidget.session.setStore(store);
}

void TaskTreeView::setOnOpenSession(std::function<void(NodeId)> callback) {
	mOnOpenSession = callback;
	mTaskWidget.session.setOnOpenSession(std::move(callback));
}

void TaskTreeView::showConflicts(const std::vector<std::string>& summaries) {
	header().showConflicts(summaries);
	if (!summaries.empty())
		status.setText(std::to_string(summaries.size()) + " conflicting edit(s): choose on the Task tab");
	else refreshSummary();
}

void TaskTreeView::showSaveState(size_t unsavedChanges, bool saving) {
	if (saving) {
		mSaveState.setTone(Label::Tone::Muted);
		mSaveState.setText("Saving...");
		return;
	}
	if (unsavedChanges == 0) {
		mSaveState.setTone(Label::Tone::Muted);
		mSaveState.setText("All changes saved");
		return;
	}
	mSaveState.setTone(Label::Tone::Warning);
	mSaveState.setText(
		"\u25CF " + std::to_string(unsavedChanges) + " unsaved change" + (unsavedChanges == 1 ? "" : "s") + "  (Ctrl+S)"
	);
}

void TaskTreeView::boardEdited() {
	if (mOnBoardEdited) mOnBoardEdited();
	boardReloaded();
}

void TaskTreeView::showTimeline(bool shown) {
	mTimeline.setVisible(shown);
	if (!shown && mTimeline.shownMoment()) mTimeline.chooseLive();
}

void TaskTreeView::setReadOnly(bool readOnly) {
	mReadOnly = readOnly;
	mTaskWidget.setReadOnly(readOnly);
}

void TaskTreeView::boardReloaded() {
	const std::optional<NodeId> current = currentTask();
	if (current && !mBoard.task(*current)) {
		openTask(std::nullopt);
	} else if (pathTo(current) != mPath) {
		showPath(pathTo(current));
	} else {
		for (size_t index = 0; index < mPath.size(); index++)
			pages().page(index).setTitle(pageTitle(mPath[index]));
		mBreadcrumb.setParts(pages().titles());
		mTaskWidget.showTask(current);
		refreshZoomAccent();
	}
	refreshGraph();
	refreshKanban();
	refreshSummary();
}

void TaskTreeView::refreshSummary() {
	size_t runnable = 0;
	for (const Node& node : mBoard.graph().nodes())
		runnable += mBoard.isRunnable(node.id) ? 1 : 0;
	status.setText(
		std::to_string(mBoard.graph().nodes().size()) + " tasks  -  " + std::to_string(runnable) + " runnable"
	);
}
