#include "BoxLayout.hpp"
#include "DemoBoard.hpp"
#include "TaskBoard.hpp"
#include "TaskTreeView.hpp"
#include "ApplicationFrame.hpp"
#include "Menus.hpp"
#include "NodeEditor.hpp"
#include "MemoryFileSystem.hpp"
#include "SessionPanel.hpp"
#include "TaskCard.hpp"
#include "TaskStore.hpp"
#include "UiDriver.hpp"

#include <algorithm>

#include "UnitTest++/UnitTest++.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;
using namespace tp::ui::testing;

SUITE(TaskBoardModel) {
	struct OrderedBoard {
		TaskBoard board;
		std::vector<NodeId> ids;

		explicit OrderedBoard(std::vector<int> orders) {
			for (int order : orders)
				ids.push_back(board.addTask({ .order = order }, { 0, 0 }));
		}

		std::vector<NodeId> inOrder(std::initializer_list<size_t> indices) const {
			std::vector<NodeId> result;
			for (size_t index : indices)
				result.push_back(ids[index]);
			return result;
		}
	};

	TEST(PlacingATaskMovesItAndOnlyRenumbersTasksInBetween) {
		OrderedBoard scene({ 10, 20, 30, 40, 50 });

		scene.board.placeBefore(scene.ids[3], scene.ids[1]);
		CHECK((scene.board.tasksInOrder() == scene.inOrder({ 0, 3, 1, 2, 4 })));
		CHECK_EQUAL(10, scene.board.task(scene.ids[0])->order);
		CHECK_EQUAL(50, scene.board.task(scene.ids[4])->order);

		scene.board.placeAfter(scene.ids[0], scene.ids[2]);
		CHECK((scene.board.tasksInOrder() == scene.inOrder({ 3, 1, 2, 0, 4 })));
		CHECK_EQUAL(50, scene.board.task(scene.ids[4])->order);
	}

	TEST(PlacingRenumbersEverythingWhenRanksCollide) {
		OrderedBoard scene({ 0, 0, 0 });

		scene.board.placeBefore(scene.ids[2], scene.ids[0]);

		CHECK((scene.board.tasksInOrder() == scene.inOrder({ 2, 0, 1 })));
		CHECK_EQUAL(0, scene.board.task(scene.ids[2])->order);
		CHECK_EQUAL(2, scene.board.task(scene.ids[1])->order);
	}

	TEST(ReparentedTaskGoesLastAmongItsNewSiblings) {
		OrderedBoard scene({ 0, 1, 2, 3 });
		scene.board.setParent(scene.ids[1], scene.ids[0]);
		scene.board.setParent(scene.ids[3], scene.ids[0]);

		scene.board.setParent(scene.ids[2], scene.ids[0]);
		scene.board.placeLastAmongSiblings(scene.ids[2]);

		CHECK((scene.board.childrenOf(scene.ids[0]) == scene.inOrder({ 1, 3, 2 })));
		CHECK_EQUAL(scene.board.orderAfterLast() - 1, scene.board.task(scene.ids[2])->order);
	}

	TEST(DependenciesGateRunnableTasks) {
		TaskBoard board;
		const NodeId first = board.addTask({ .status = TaskStatus::Done }, { 0, 0 });
		const NodeId second = board.addTask({ .status = TaskStatus::Wip }, { 300, 0 });
		const NodeId last = board.addTask({ .status = TaskStatus::Queued }, { 600, 0 });

		CHECK(board.addDependency(last, first));
		CHECK(board.addDependency(last, second));
		CHECK(!board.isRunnable(last));

		board.task(second)->status = TaskStatus::Done;
		CHECK(board.isRunnable(last));
	}

	TEST(DependencyCyclesAreRejected) {
		TaskBoard board;
		const NodeId first = board.addTask({}, { 0, 0 });
		const NodeId second = board.addTask({}, { 300, 0 });
		const NodeId third = board.addTask({}, { 600, 0 });

		CHECK(board.addDependency(second, first));
		CHECK(board.addDependency(third, second));
		CHECK(!board.addDependency(first, third));
	}

	TEST(TaskHasSingleParentAndHierarchyCyclesAreRejected) {
		TaskBoard board;
		const NodeId root = board.addTask({}, { 0, 0 });
		const NodeId other = board.addTask({}, { 0, 300 });
		const NodeId child = board.addTask({}, { 300, 0 });

		CHECK(board.setParent(child, root));
		CHECK(board.setParent(child, other));
		CHECK_EQUAL(other, *board.parentOf(child));
		CHECK(!board.setParent(other, child));
	}

	TEST(RelationQueriesFollowLinks) {
		TaskBoard board;
		const NodeId root = board.addTask({}, { 0, 0 });
		const NodeId child = board.addTask({}, { 0, 0 });
		const NodeId grandchild = board.addTask({}, { 0, 0 });
		const NodeId other = board.addTask({}, { 0, 0 });
		board.setParent(child, root);
		board.setParent(grandchild, child);
		board.addDependency(other, child);

		CHECK((board.rootTasks() == std::vector<NodeId>{ root, other }));
		CHECK(board.childrenOf(root) == std::vector<NodeId>{ child });
		CHECK((board.ancestorsOf(grandchild) == (std::vector<NodeId>{ root, child })));
		CHECK(board.dependentsOf(child) == std::vector<NodeId>{ other });
		CHECK(board.dependenciesOf(other) == std::vector<NodeId>{ child });

		board.removeDependency(other, child);
		CHECK(board.dependentsOf(child).empty());
		board.clearParent(child);
		CHECK((board.rootTasks() == (std::vector<NodeId>{ root, child, other })));
	}

	TEST(LinkChecksRejectSelfMissingAndCycles) {
		TaskBoard board;
		const NodeId first = board.addTask({}, { 0, 0 });
		const NodeId second = board.addTask({}, { 0, 0 });
		board.addDependency(second, first);
		board.setParent(second, first);

		CHECK(!board.canAddDependency(first, first));
		CHECK(!board.canAddDependency(first, 999));
		CHECK(!board.canAddDependency(first, second));
		CHECK(board.canAddDependency(second, first));
		CHECK(!board.canSetParent(first, second));
		CHECK(board.canSetParent(second, first));
	}

	TEST(HierarchyAndDependencyPortsDoNotMix) {
		TaskBoard board;
		const NodeId first = board.addTask({}, { 0, 0 });
		const NodeId second = board.addTask({}, { 300, 0 });

		CHECK(!board.graph().connect(
			TaskBoard::output(first, TaskOutput::Children), TaskBoard::input(second, TaskPort::DependsOn)
		));
	}
}

namespace {
	bool shownOnScreen(const Widget& widget) {
		for (const Widget* current = &widget; current; current = current->parent())
			if (!current->isVisible()) return false;
		return widget.ui() != nullptr;
	}

	Button* findButton(Widget& root, const std::string& text) {
		if (auto* button = dynamic_cast<Button*>(&root); button && button->text() == text) return button;
		for (Widget* child : root.children())
			if (Button* found = findButton(*child, text)) return found;
		return nullptr;
	}

	struct RecordingStore : TaskStore {
		void startSession(NodeId id) override { started.push_back(id); }
		void restartSession(NodeId id, bool fresh) override { restarts.emplace_back(id, fresh); }
		void stopSession(NodeId id) override { stopped.push_back(id); }
		void beginEditing(NodeId id, TaskField field) override { editing = std::make_pair(id, field); }
		void endEditing() override { editing.reset(); }

		std::optional<std::pair<NodeId, TaskField>> editing;
		std::vector<NodeId> started;
		std::vector<std::pair<NodeId, bool>> restarts;
		std::vector<NodeId> stopped;
	};

	struct TreeScene {
		TaskBoard board;
		NodeId widgets = board.addTask({ .title = "Widgets" }, { 0, 0 });
		NodeId textEdit = board.addTask({ .title = "Text edit glitches", .description = "space and emoji" }, { 0, 0 });
		NodeId emoji =
			board.addTask({ .title = "Emoji fallback", .description = "copy NotoEmoji into the assets" }, { 0, 0 });
		NodeId release = board.addTask({ .title = "Release notes", .workspace = "/agents/task-4" }, { 0, 0 });
		MemoryFileSystem files;
		ApplicationFrame window;
		AppStorage storage{ "/home/user/taskboard" };
		TaskTreeView view{ board, files, storage };
		RecordingStore store;
		int edits = 0;

		explicit TreeScene(bool panelsOpen = true) {
			files.addTextFile("/agents/task-4/reports/summary.md", "# Summary\n\nAll done.");
			files.addTextFile("/agents/task-4/reports/data.csv", "a,b");
			files.addDirectory("/agents/task-4/notes");
			window.setContent(&view);
			view.attachMenus(window.menuBar, [] {});
			view.dock().setViewMode(view.taskPanel(), DockPanel::ViewMode::Docked);
			view.dock().hidePanel(view.taskPanel());
			if (panelsOpen) view.openPanels();
			view.taskWidget().showPage(TaskWidgetPage::Links);
			board.setParent(textEdit, widgets);
			board.setParent(emoji, textEdit);
			view.boardReloaded();
			view.setStore(&store);
			view.setOnBoardEdited([this] { edits++; });
		}

		TaskDetails& details() { return view.details(); }

		void showHeader(UiDriver& driver) {
			view.taskWidget().showPage(TaskWidgetPage::Task);
			driver.settle();
		}

		void chooseAction(UiDriver& driver, const std::string& text) {
			showHeader(driver);
			driver.click(view.header().actionsButton);
			driver.settle();
			for (const auto& item : view.header().actionsMenu.items()) {
				if (item->text() != text) continue;
				driver.click(*item);
				driver.settle();
				return;
			}
			CHECK(false);
		}
	};
}

SUITE(TaskTreeNavigation) {
	TEST(StartsAtBoardListingTopLevelTasks) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });

		CHECK(!scene.view.currentTask());
		CHECK_EQUAL(1u, scene.view.pages().depth());
		CHECK_EQUAL(1u, scene.view.breadcrumb().partCount());
		CHECK(scene.details().row(scene.details().children, scene.widgets) != nullptr);
		CHECK(scene.details().row(scene.details().children, scene.release) != nullptr);
		CHECK(scene.details().row(scene.details().children, scene.textEdit) == nullptr);
		CHECK(scene.view.page().rootHint.isVisible());
		CHECK(!shownOnScreen(scene.view.header().title));
	}

	TEST(SingleClickOnChildRowCentresTheGraphOnIt) {
		TreeScene scene;
		scene.board.task(scene.release)->order = 9;
		UiDriver driver(scene.window, { 1200, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();

		driver.click(scene.details().row(scene.details().children, scene.release)->openButton());
		driver.settle();
		CHECK(!scene.view.currentTask());
		const RectF card = *graph.cardRect(scene.release);
		const Vec2F centre = card.pos + card.size / 2.0f;
		CHECK_CLOSE(graph.size().x / 2, centre.x, 1.0f);
		CHECK_CLOSE(graph.size().y / 2, centre.y, 1.0f);
	}

	TEST(OpeningChildRowPushesPageAndBackReturns) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		driver.settle();

		Button& row = scene.details().row(scene.details().children, scene.widgets)->openButton();
		const Vec2F at = row.windowRect().center();
		driver.click(at);
		driver.click(at);
		driver.settle();
		CHECK(scene.view.currentTask() == scene.widgets);
		CHECK_EQUAL(2u, scene.view.pages().depth());
		CHECK_EQUAL("Widgets", scene.view.header().title.text());
		CHECK(scene.details().row(scene.details().children, scene.textEdit) != nullptr);

		CHECK(!shownOnScreen(scene.view.pages().currentPage().backButton()));
		driver.click(scene.view.breadcrumb().part(0));
		driver.settle();
		CHECK(!scene.view.currentTask());
		CHECK_EQUAL(1u, scene.view.pages().depth());
	}

	TEST(OpeningDeepTaskBuildsItsParentPath) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });

		scene.view.openTask(scene.emoji);
		driver.settle();
		CHECK_EQUAL(4u, scene.view.pages().depth());
		REQUIRE CHECK_EQUAL(4u, scene.view.breadcrumb().partCount());
		CHECK_EQUAL("Board", scene.view.breadcrumb().part(0).text());
		CHECK_EQUAL("Text edit glitches", scene.view.breadcrumb().part(2).text());

		driver.click(scene.view.breadcrumb().part(1));
		driver.settle();
		CHECK(scene.view.currentTask() == scene.widgets);
		CHECK_EQUAL(2u, scene.view.pages().depth());
	}

	TEST(SwitchingToUnrelatedTaskReplacesPath) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.openTask(scene.emoji);
		scene.view.openTask(scene.release);
		driver.settle();

		CHECK(scene.view.currentTask() == scene.release);
		CHECK_EQUAL(2u, scene.view.pages().depth());
	}

	TEST(SectionsStartExpandedAndKeepUserChoice) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.openTask(scene.widgets);
		driver.settle();
		for (RelationSection* section :
				 { &scene.details().children, &scene.details().dependsOn, &scene.details().unblocks })
			CHECK(section->isExpanded());
		CHECK_EQUAL("1", scene.details().children.header().summary());
		CHECK(scene.details().row(scene.details().children, scene.textEdit) != nullptr);

		driver.click(scene.details().dependsOn.header());
		driver.settle();
		CHECK(!scene.details().dependsOn.isExpanded());
		scene.view.openTask(scene.release);
		CHECK(!scene.details().dependsOn.isExpanded());
	}

	TEST(DraggingChildrenReordersThemAndRecordsOrder) {
		TreeScene scene;
		const NodeId second = scene.board.addTask({ .title = "Second", .order = 1 }, { 0, 0 });
		const NodeId third = scene.board.addTask({ .title = "Third", .order = 2 }, { 0, 0 });
		scene.board.setParent(second, scene.widgets);
		scene.board.setParent(third, scene.widgets);
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.openTask(scene.widgets);
		driver.settle();

		ReorderableList& list = scene.details().children.list();
		REQUIRE CHECK_EQUAL(3u, list.itemCount());
		const Vec2F grip = list.windowPosition() + list.gripRect(2).center();
		const Vec2F top = list.windowPosition() + list.gripRect(0).center() - Vec2F(0, 8);
		scene.edits = 0;
		driver.drag(grip, top, 6);
		driver.settle();

		CHECK(scene.board.childrenOf(scene.widgets) == (std::vector<NodeId>{ third, scene.textEdit, second }));
		CHECK(scene.board.task(third)->order < scene.board.task(scene.textEdit)->order);
		CHECK(scene.board.task(scene.textEdit)->order < scene.board.task(second)->order);
		CHECK(scene.edits > 0);
	}

	TEST(ChildRowShowsColorAsSwatchNotFill) {
		TreeScene scene;
		scene.board.task(scene.widgets)->color = RGBA(1, 0, 0, 1);
		scene.view.boardReloaded();
		UiDriver driver(scene.window, { 1200, 900 });
		driver.settle();

		TaskRowButton& button = scene.details().row(scene.details().children, scene.widgets)->openButton();
		CHECK(button.swatch() == RGBA(1, 0, 0, 1));

		const RectF row = button.windowRect();
		const RectF panel = scene.view.pages().windowRect();
		size_t redFills = 0;
		bool rowFilledRed = false;
		for (const PaintCommand& command : driver.paint().commands()) {
			if (command.kind != PaintCommand::Kind::FillRect || !(command.color == RGBA(1, 0, 0, 1))) continue;
			const bool inPanel = command.rect.pos.x >= panel.pos.x - 1 &&
													 command.rect.pos.x + command.rect.size.x <= panel.pos.x + panel.size.x + 1;
			if (!inPanel) continue;
			redFills++;
			rowFilledRed |= command.rect.size.x >= row.size.x - 1;
		}
		CHECK_EQUAL(1u, redFills);
		CHECK(!rowFilledRed);
	}

	TEST(NewChildGoesLastInTheGlobalOrder) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.board.task(scene.textEdit)->order = 4;
		scene.board.task(scene.release)->order = 7;
		scene.view.openTask(scene.widgets);
		scene.view.createTask();

		REQUIRE CHECK(scene.view.currentTask());
		CHECK_EQUAL(8, scene.board.task(*scene.view.currentTask())->order);
		CHECK(scene.board.childrenOf(scene.widgets).back() == *scene.view.currentTask());
	}

	TEST(DeleteAsksThenMovesChildrenUpKeepingTheirPlaceAndReturnsToParent) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.board.task(scene.release)->order = 9;
		scene.view.openTask(scene.widgets);
		driver.settle();

		scene.chooseAction(driver, "Delete task...");
		driver.settle();
		REQUIRE CHECK(scene.view.header().deleteConfirm.isOpen());
		CHECK(scene.board.task(scene.widgets) != nullptr);
		scene.view.header().deleteConfirm.accept();
		driver.settle();

		CHECK(scene.board.task(scene.widgets) == nullptr);
		CHECK(!scene.view.currentTask());
		CHECK(!scene.board.parentOf(scene.textEdit));
		CHECK(scene.board.parentOf(scene.emoji) == scene.textEdit);
		CHECK(scene.board.rootTasks() == (std::vector<NodeId>{ scene.textEdit, scene.release }));
		CHECK(scene.edits > 0);
	}

	TEST(DeleteFromMenuCanBeCancelled) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.openTask(scene.emoji);
		driver.settle();

		for (const auto& item : scene.view.editMenu.items())
			if (item->text() == "Delete Task") item->activate();
		driver.settle();
		REQUIRE CHECK(scene.view.header().deleteConfirm.isOpen());
		scene.view.header().deleteConfirm.cancel();
		driver.settle();
		CHECK(scene.board.task(scene.emoji) != nullptr);
		CHECK(scene.view.currentTask() == scene.emoji);
	}

	TEST(TaskRemovedByReloadFallsBackToBoard) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.openTask(scene.release);
		scene.board.graph().removeNodes({ scene.release });
		scene.view.boardReloaded();
		driver.settle();

		CHECK(!scene.view.currentTask());
		CHECK_EQUAL(1u, scene.view.pages().depth());
	}
}

SUITE(TaskTreeSearch) {
	TEST(SearchMatchesIdTitleAndDescriptionInThatOrder) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });

		const std::vector<SearchResult> byId = scene.view.searchResults("#3");
		REQUIRE CHECK_EQUAL(1u, byId.size());
		CHECK_EQUAL("#3  Emoji fallback", byId[0].title);
		CHECK(byId[0].detail.find("Widgets \u203A Text edit glitches") != std::string::npos);

		const std::vector<SearchResult> byText = scene.view.searchResults("emoji");
		REQUIRE CHECK_EQUAL(2u, byText.size());
		CHECK_EQUAL("#3  Emoji fallback", byText[0].title);
		CHECK_EQUAL("#2  Text edit glitches", byText[1].title);
		CHECK(byText[1].detail.find("\"space and emoji\"") != std::string::npos);

		CHECK(scene.view.searchResults("NOTOEMOJI").size() == 1);
		CHECK(scene.view.searchResults("").empty());
		CHECK(scene.view.searchResults("nothing like this").empty());
	}

	TEST(ChoosingSearchResultOpensTask) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });

		driver.key(InputID::F, { .control = true });
		CHECK(scene.view.search().input().isEditing());
		driver.type("emoji");
		driver.settle();
		REQUIRE CHECK(scene.view.search().isShowingResults());
		driver.key(InputID::ENTER);
		driver.settle();

		CHECK(scene.view.currentTask() == scene.emoji);
		CHECK_EQUAL(4u, scene.view.pages().depth());
		CHECK(scene.view.search().input().text().empty());
	}
}

SUITE(TaskTreeEditing) {
	TEST(TitleRowShowsIdAndHidesBadgeWithoutSession) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.openTask(scene.emoji);
		driver.settle();
		scene.showHeader(driver);

		TaskHeader& page = scene.view.header();
		CHECK_EQUAL("#3", page.idLabel.text());
		CHECK(page.idLabel.windowRect().pos.y + page.idLabel.windowRect().size.y <= page.title.windowRect().pos.y + 1);
		CHECK(page.idLabel.parent() == &page.badgeRow());
		CHECK(!page.badge.isVisible());
		CHECK(!driver.paint().hasText("\u25CF no session"));

		scene.board.task(scene.emoji)->session.state = SessionState::Working;
		scene.view.boardReloaded();
		driver.settle();
		CHECK(page.badge.isVisible());
		CHECK(page.badge.isPulsing());
		CHECK(page.badge.isDotShown());
		CHECK(page.actionsButton.windowRect().pos.x > page.badge.windowRect().pos.x);
		const RectF id = page.idLabel.windowRect();
		CHECK(page.badge.windowRect().pos.x > id.pos.x + id.size.x + 40);
		CHECK(page.badge.windowRect().pos.x > page.status.windowRect().pos.x);
	}

	TEST(DockedPanelShowsTheBadgeRowAboveTheTitleAndLinksHaveNoPageHeader) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.openTask(scene.textEdit);
		driver.settle();

		CHECK(!shownOnScreen(scene.view.pages().currentPage().header()));
		CHECK(!driver.paint().hasText("#2  Text edit glitches"));

		scene.showHeader(driver);
		TaskHeader& header = scene.view.header();
		CHECK(header.badgeRow().parent() != &header);
		CHECK(shownOnScreen(header.idLabel));
		CHECK(header.badgeRow().windowRect().pos.y + header.badgeRow().windowRect().size.y <= header.title.windowRect().pos.y + 1);
	}

	TEST(TitleRowHoldsStatusAndColorAndSectionHeadersHoldAdd) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.openTask(scene.release);
		driver.settle();
		scene.showHeader(driver);

		TaskHeader& header = scene.view.header();
		const halnf badgeY = header.idLabel.windowRect().center().y;
		CHECK_CLOSE(badgeY, header.status.windowRect().center().y, 2.0f);
		CHECK_CLOSE(badgeY, header.color.windowRect().center().y, 2.0f);
		CHECK(header.title.windowRect().pos.y > header.status.windowRect().pos.y);
		CHECK(header.title.windowRect().size.y > header.status.windowRect().size.y);
		scene.view.taskWidget().showPage(TaskWidgetPage::Links);
		driver.settle();
		CHECK(shownOnScreen(header.status));
		CHECK(!shownOnScreen(header.title));
		for (RelationSection* section :
				 { &scene.details().children, &scene.details().dependsOn, &scene.details().unblocks }) {
			CHECK(section->header().action() == &section->addButton());
			CHECK(!section->isPickerShown());
			CHECK_CLOSE(section->header().windowRect().center().y, section->addButton().windowRect().center().y, 2.0f);
		}
	}

	TEST(EditingFieldsChangesTaskAndReportsEdits) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.openTask(scene.release);
		driver.settle();
		scene.showHeader(driver);

		driver.click(scene.view.header().title);
		driver.key(InputID::END);
		driver.type("!");
		scene.view.header().status.setSelectedIndex(
			long(std::find(kTaskStatuses.begin(), kTaskStatuses.end(), TaskStatus::Queued) - kTaskStatuses.begin())
		);

		const Task& task = *scene.board.task(scene.release);
		CHECK_EQUAL("Release notes!", task.title);
		CHECK(task.status == TaskStatus::Queued);
		CHECK(scene.edits >= 2);
		CHECK_EQUAL("Release notes!", scene.view.pages().currentPage().title());
	}

	TEST(AddChildOpensSearchWithCreateNewFirst) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.board.task(scene.widgets)->agentModel = "claude-opus-4-8";
		scene.view.openTask(scene.widgets);
		driver.settle();

		driver.click(scene.details().children.addButton());
		driver.settle();
		SearchField& picker = scene.details().children.picker();
		REQUIRE CHECK(picker.isShowingResults());
		CHECK(picker.input().isEditing());
		REQUIRE CHECK(picker.rows().size() >= 2);
		CHECK_EQUAL("+ Create new: New task", picker.rows()[0]->result().title);
		CHECK_EQUAL(0, picker.highlighted());

		driver.type("Docs");
		driver.settle();
		CHECK_EQUAL("+ Create new: Docs", picker.rows()[0]->result().title);
		driver.key(InputID::ENTER);
		driver.settle();

		const std::optional<NodeId> created = scene.view.currentTask();
		REQUIRE CHECK(created && *created != scene.widgets);
		CHECK_EQUAL("Docs", scene.board.task(*created)->title);
		CHECK(scene.board.parentOf(*created) == scene.widgets);
		CHECK_EQUAL("claude-opus-4-8", scene.board.task(*created)->agentModel);
		CHECK(scene.view.header().title.isEditing());
	}

	TEST(AddChildCanAdoptExistingTask) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.openTask(scene.widgets);
		driver.settle();

		driver.click(scene.details().children.addButton());
		driver.type("Release");
		driver.settle();
		SearchField& picker = scene.details().children.picker();
		REQUIRE CHECK_EQUAL(2u, picker.rows().size());
		driver.key(InputID::DOWN);
		driver.key(InputID::ENTER);
		driver.settle();

		CHECK(scene.view.currentTask() == scene.widgets);
		CHECK(scene.board.parentOf(scene.release) == scene.widgets);
		CHECK(scene.board.childrenOf(scene.widgets).back() == scene.release);

		driver.click(scene.details().children.addButton());
		driver.type("Widgets");
		driver.settle();
		CHECK_EQUAL(1u, picker.rows().size());
	}

	TEST(CtrlNOnBoardCreatesTopLevelTask) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		driver.key(InputID::N, { .control = true });
		driver.settle();

		REQUIRE CHECK(scene.view.currentTask());
		CHECK(!scene.board.parentOf(*scene.view.currentTask()));
		CHECK_EQUAL(5u, scene.board.graph().nodes().size());
	}

	TEST(DependencyPickerLinksAndBlocksCycles) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.board.addDependency(scene.widgets, scene.release);
		scene.view.openTask(scene.release);
		driver.settle();

		driver.click(scene.details().dependsOn.addButton());
		driver.settle();
		SearchField& picker = scene.details().dependsOn.picker();
		REQUIRE CHECK(scene.details().dependsOn.isPickerShown());
		CHECK(picker.input().isEditing());
		driver.type("Widgets");
		driver.settle();

		REQUIRE CHECK_EQUAL(2u, picker.rows().size());
		CHECK(!picker.rows()[1]->result().enabled);
		CHECK_EQUAL("would create a cycle", picker.rows()[1]->result().detail);

		picker.setText("Emoji");
		driver.settle();
		driver.key(InputID::DOWN);
		driver.key(InputID::ENTER);
		driver.settle();

		CHECK(scene.board.dependenciesOf(scene.release) == std::vector<NodeId>{ scene.emoji });
		CHECK(!scene.details().dependsOn.isPickerShown());
		CHECK(scene.details().row(scene.details().dependsOn, scene.emoji) != nullptr);
		CHECK(scene.details().row(scene.details().unblocks, scene.widgets) != nullptr);
	}

	TEST(DependencyCreateNewMakesSiblingAndLinksIt) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.openTask(scene.emoji);
		driver.settle();

		driver.click(scene.details().dependsOn.addButton());
		driver.type("Fonts");
		driver.settle();
		driver.key(InputID::ENTER);
		driver.settle();

		const std::optional<NodeId> created = scene.view.currentTask();
		REQUIRE CHECK(created && *created != scene.emoji);
		CHECK_EQUAL("Fonts", scene.board.task(*created)->title);
		CHECK(scene.board.parentOf(*created) == scene.textEdit);
		CHECK(scene.board.dependenciesOf(scene.emoji) == std::vector<NodeId>{ *created });
	}

	TEST(UnblocksPickerAddsDependentAndRemoveUnlinks) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.openTask(scene.emoji);
		driver.settle();

		driver.click(scene.details().unblocks.addButton());
		driver.type("#4");
		driver.settle();
		driver.key(InputID::DOWN);
		driver.key(InputID::ENTER);
		driver.settle();
		CHECK(scene.board.dependenciesOf(scene.release) == std::vector<NodeId>{ scene.emoji });

		driver.click(scene.details().row(scene.details().unblocks, scene.release)->removeButton());
		driver.settle();
		CHECK(scene.board.dependenciesOf(scene.release).empty());
		CHECK(scene.view.currentTask() == scene.emoji);
	}

	TEST(SetParentMovesTaskAndUpdatesPath) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.openTask(scene.emoji);
		driver.settle();

		scene.chooseAction(driver, "Set parent...");
		driver.settle();
		SearchField& picker = scene.view.header().parentPicker;
		REQUIRE CHECK(picker.isShowingResults());
		CHECK_EQUAL("Top level", picker.rows()[0]->result().title);

		driver.type("Release");
		driver.settle();
		REQUIRE CHECK_EQUAL(2u, picker.rows().size());
		driver.key(InputID::DOWN);
		driver.key(InputID::ENTER);
		driver.settle();
		CHECK(scene.board.parentOf(scene.emoji) == scene.release);
		CHECK(scene.view.currentTask() == scene.emoji);
		CHECK_EQUAL(3u, scene.view.pages().depth());

		scene.chooseAction(driver, "Set parent...");
		driver.settle();
		REQUIRE CHECK(picker.isShowingResults());
		driver.key(InputID::ENTER);
		driver.settle();
		CHECK(!scene.board.parentOf(scene.emoji));
		CHECK_EQUAL(2u, scene.view.pages().depth());

		scene.chooseAction(driver, "Set parent...");
		driver.type("Text edit");
		driver.settle();
		scene.view.openTask(scene.widgets);
		scene.chooseAction(driver, "Set parent...");
		driver.type("Emoji");
		driver.settle();
		REQUIRE CHECK_EQUAL(2u, picker.rows().size());
		CHECK(picker.rows()[1]->result().enabled);
	}

	TEST(TypingReportsEditingScopeAndReloadKeepsIt) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.taskWidget().showPage(TaskWidgetPage::Task);
		scene.view.openTask(scene.release);
		driver.settle();
		scene.showHeader(driver);

		MarkdownEditor& editor = scene.view.description().editor;
		editor.setMode(MarkdownEditor::Mode::Editor);
		driver.settle();
		driver.click(editor.sourceArea());
		driver.type("draft");
		REQUIRE CHECK(scene.store.editing.has_value());
		CHECK(scene.store.editing->second == TaskField::Description);

		scene.view.boardReloaded();
		CHECK_EQUAL("draft", editor.markdown());
		CHECK_EQUAL("draft", scene.board.task(scene.release)->description);
		driver.click(scene.view.header().title);
		REQUIRE CHECK(scene.store.editing.has_value());
		CHECK(scene.store.editing->second == TaskField::Title);
	}

	TEST(ConflictBannerShowsAndReportsChoice) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		std::vector<bool> choices;
		scene.view.header().setOnConflictChoice([&](bool keepMine) { choices.push_back(keepMine); });
		scene.view.openTask(scene.release);

		scene.view.showConflicts({ "#4 Title: theirs \"A\", yours \"B\"" });
		driver.settle();
		scene.showHeader(driver);
		CHECK(shownOnScreen(scene.view.header().keepMine));
		driver.click(scene.view.header().keepMine);
		driver.click(scene.view.header().useTheirs);
		CHECK((choices == (std::vector<bool>{ true, false })));

		scene.view.showConflicts({});
		CHECK(!shownOnScreen(scene.view.header().keepMine));
	}

	TEST(SaveFromCtrlSAndMenuAndStatusShowsUnsavedCount) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		int saves = 0;
		scene.view.setOnSave([&] { saves++; });

		driver.key(InputID::S, { .control = true });
		for (const auto& item : scene.view.fileMenu.items())
			if (item->text() == "Save") item->activate();
		CHECK_EQUAL(2, saves);

		scene.view.showSaveState(3, false);
		driver.settle();
		CHECK(driver.paint().findText("\u25CF 3 unsaved changes  (Ctrl+S)") != nullptr);
		scene.view.showSaveState(0, false);
		driver.settle();
		CHECK(driver.paint().findText("All changes saved") != nullptr);
	}
}

SUITE(TaskTreePanels) {
	TEST(TaskPanelStartsDockedUnpinnedAndClosed) {
		TaskBoard board;
		MemoryFileSystem files;
		TaskTreeView view{ board, files, AppStorage{ "/home/user/taskboard" } };
		CHECK(view.taskPanel().viewMode() == DockPanel::ViewMode::Undocked);
		CHECK(view.taskPanel().side() == DockSide::Right);
		CHECK(!view.taskPanel().isOpen());
	}

	TEST(PanelsStartClosedLeavingTheGraphTheWholeWindow) {
		TreeScene scene(false);
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		CHECK(!scene.view.taskPanel().isOpen());
		CHECK(scene.view.graph().size().x > 1300);

		scene.view.openPanels();
		driver.settle();
		CHECK(scene.view.taskPanel().isOpen());
		CHECK(scene.view.graph().size().x < 1000);
	}
}

SUITE(TaskTreeSession) {
	TEST(SessionPanelStaysAtTheBottomAndControlsFollowState) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		scene.view.taskWidget().showPage(TaskWidgetPage::Session);
		driver.settle();
		CHECK(!scene.view.session().isVisible());
		CHECK(driver.paint().hasText("Open a task to start or follow its agent session."));

		scene.view.openTask(scene.release);
		driver.settle();
		CHECK(scene.view.session().isVisible());
		SessionTab& session = scene.view.session();
		CHECK(session.startSession.isEffectivelyEnabled());

		driver.click(session.startSession);
		CHECK(scene.store.started == std::vector<NodeId>{ scene.release });

		scene.board.task(scene.release)->session = { SessionState::Working, "tool: Edit", std::nullopt };
		scene.view.boardReloaded();
		driver.settle();
		CHECK(!session.startSession.isEffectivelyEnabled());
		CHECK(session.stopSession.isEffectivelyEnabled());
		CHECK(session.state.text().find("tool: Edit") != std::string::npos);
		CHECK(scene.view.header().badge.text().find("running") != std::string::npos);

		driver.click(session.stopSession);
		CHECK(scene.store.stopped == std::vector<NodeId>{ scene.release });
	}

	TEST(DshSessionOpensRegardlessOfSessionStateWhileTerminalSessionsNeedAProcess) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.view.taskWidget().showPage(TaskWidgetPage::Session);
		std::vector<NodeId> opened;
		scene.view.setOnOpenSession([&opened](NodeId id) { opened.push_back(id); });
		scene.view.openTask(scene.release);
		driver.settle();
		SessionTab& session = scene.view.session();
		Task& task = *scene.board.task(scene.release);

		task.harness = "claude-code";
		task.session = { SessionState::Error, "rate_limit", std::nullopt };
		scene.view.boardReloaded();
		driver.settle();
		CHECK(session.openSession.isEffectivelyEnabled());

		task.session = { SessionState::Ended, "stopped from the board", std::nullopt };
		scene.view.boardReloaded();
		driver.settle();
		CHECK(!session.openSession.isEffectivelyEnabled());

		task.harness = "dsh";
		for (SessionState state : { SessionState::None, SessionState::Ended, SessionState::Error }) {
			task.session = { state, "", std::nullopt };
			scene.view.boardReloaded();
			driver.settle();
			CHECK(session.openSession.isEffectivelyEnabled());
		}

		driver.click(session.openSession);
		CHECK(opened == std::vector<NodeId>{ scene.release });
	}

	TEST(PresetsSitAboveCollapsedParametersAboveTheButtonsAndThereIsNoMessageField) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.view.taskWidget().showPage(TaskWidgetPage::Session);
		scene.view.openTask(scene.release);
		driver.settle();
		SessionTab& session = scene.view.session();

		CHECK(!session.parameters.isExpanded());
		CHECK(!session.model.isVisible() || !session.parameters.isBodyShown());
		CHECK(session.presets.windowRect().pos.y < session.parameters.windowRect().pos.y);
		CHECK(session.parameters.windowRect().pos.y < session.startSession.windowRect().pos.y);
		CHECK(!driver.paint().hasText("Message the agent..."));
		const Task& task = *scene.board.task(scene.release);
		CHECK(driver.paint().hasText(
			task.agentModel + "  \u00b7  " + task.launchSettings().harness + "  \u00b7  " + task.effort + "  \u00b7  " +
			task.permissionMode
		));

		driver.click(session.parameters.header());
		driver.settle();
		CHECK(session.parameters.isExpanded());
		CHECK(session.model.isVisible());
		CHECK(session.model.windowRect().pos.y < session.startSession.windowRect().pos.y);
	}

	TEST(ParametersSaveAsANamedPresetAndApplyToAnotherTask) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.view.taskWidget().showPage(TaskWidgetPage::Session);
		scene.view.openTask(scene.release);
		driver.settle();
		SessionTab& session = scene.view.session();
		session.parameters.setExpanded(true, false);
		driver.settle();
		const std::vector<std::string>& efforts = session.effort.options();
		session.effort.setSelectedIndex(
			long(std::find(efforts.begin(), efforts.end(), std::string("low")) - efforts.begin())
		);
		CHECK_EQUAL("low", scene.board.task(scene.release)->effort);
		CHECK_EQUAL(SessionPresetPicker::kCustomOption, session.presets.preset.selectedText());

		driver.click(session.presets.saveAs);
		driver.settle();
		REQUIRE CHECK(session.presets.isNaming());
		driver.click(session.presets.name);
		driver.type("dumb agent");
		driver.key(InputID::ENTER);
		driver.settle();
		CHECK(!session.presets.isNaming());
		CHECK_EQUAL("dumb agent", session.presets.preset.selectedText());

		const std::string path = SessionPresets::pathIn(scene.storage.root());
		CHECK_EQUAL("/home/user/taskboard/session-presets.json", path);
		const std::optional<std::string> stored = scene.files.readText(path);
		REQUIRE CHECK(stored.has_value());
		CHECK(stored->find("\"dumb agent\"") != std::string::npos);
		CHECK(stored->find("\"low\"") != std::string::npos);

		scene.view.openTask(scene.widgets);
		driver.settle();
		CHECK_EQUAL(SessionPresetPicker::kCustomOption, session.presets.preset.selectedText());
		CHECK(scene.board.task(scene.widgets)->effort != "low");
		session.presets.preset.setSelectedIndex(1);
		CHECK_EQUAL("low", scene.board.task(scene.widgets)->effort);
		CHECK_EQUAL("low", session.effort.selectedText());
		CHECK(scene.view.header().title.text().size() > 0);

		SessionPresets reloaded(scene.files, scene.storage.root());
		REQUIRE CHECK_EQUAL(1u, reloaded.all().size());
		CHECK_EQUAL("dumb agent", reloaded.all()[0].name);
		CHECK_EQUAL("low", reloaded.all()[0].settings.effort);

		driver.click(session.presets.remove);
		driver.settle();
		CHECK_EQUAL(SessionPresetPicker::kCustomOption, session.presets.preset.selectedText());
		reloaded.reload();
		CHECK(reloaded.all().empty());
	}

	TEST(ZoomingIntoATaskCardFillsTheGraphWithTheSameCardView) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.view.taskWidget().showPage(TaskWidgetPage::Task);
		driver.settle();
		TreeView& graph = scene.view.graph();
		graph.select(scene.release);
		graph.setView(graph.pan(), graph.fullViewScale(), false);
		graph.centerOn(scene.release, false);
		driver.settle();
		driver.moveTo(graph.windowPosition() + graph.size() / 2.0f);

		for (int notch = 0; !graph.fullViewItem() && notch < 40; notch++)
			driver.wheel(1);
		driver.settle();
		REQUIRE CHECK(graph.fullViewItem() == std::optional<NodeId>(scene.release));
		TaskDescriptionPanel& description = scene.view.description();
		CHECK(shownOnScreen(description.editor));
		CHECK(description.windowRect().size.x > graph.size().x * 0.9f);
		CHECK_EQUAL(scene.board.task(scene.release)->title, description.header.title.text());
		CHECK(!shownOnScreen(*graph.card(scene.release)));
		CHECK(scene.view.currentTask() == std::optional<NodeId>(scene.release));

		driver.click(scene.view.header().back);
		driver.settle();
		CHECK(!graph.fullViewItem());
		CHECK(description.windowRect().size.x < graph.size().x * 0.5f);
		CHECK(shownOnScreen(*graph.card(scene.release)));
	}

	TEST(ZoomingMovesTheOneTaskWidgetIntoTheZoomAndCollapsesItsDockPanelUntilZoomingOut) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		TaskWidget& widget = scene.view.taskWidget();
		REQUIRE CHECK(scene.view.taskPanel().isOpen());
		CHECK(widget.isSelfOrAncestorOf(*scene.view.taskPanel().content()) || scene.view.taskPanel().content() == &widget);

		graph.select(scene.release);
		graph.enterFullView(scene.release);
		driver.settle();
		CHECK(!scene.view.taskPanel().isOpen());
		CHECK(scene.view.zoomFrame() == &graph.fullViewFrame());
		CHECK(graph.fullViewFrame().isSelfOrAncestorOf(widget));
		CHECK(shownOnScreen(widget.carousel));
		CHECK(widget.windowRect().size.x > graph.size().x * 0.9f);

		graph.leaveFullView(false);
		driver.settle();
		CHECK(scene.view.taskPanel().isOpen());
		CHECK(scene.view.zoomFrame() == nullptr);
		CHECK(scene.view.taskPanel().isSelfOrAncestorOf(widget));
		CHECK(shownOnScreen(widget.carousel));
	}

	TEST(ZoomingOutKeepsTheDockPanelClosedWhenItWasClosedBeforeZooming) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.view.dock().hidePanel(scene.view.taskPanel());
		driver.settle();
		TreeView& graph = scene.view.graph();

		graph.select(scene.release);
		graph.enterFullView(scene.release);
		driver.settle();
		CHECK(scene.view.taskWidget().carousel.isVisible());
		CHECK(shownOnScreen(scene.view.taskWidget().carousel));

		graph.leaveFullView(false);
		driver.settle();
		CHECK(!scene.view.taskPanel().isOpen());
	}

	TEST(OpeningTheTaskPanelFromItsStripeWhileZoomedLeavesTheZoom) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		graph.select(scene.release);
		graph.enterFullView(scene.release);
		driver.settle();
		StripeButton* stripe = scene.view.dock().stripeButton(scene.view.taskPanel());
		REQUIRE CHECK(stripe != nullptr);
		REQUIRE CHECK(shownOnScreen(*stripe));

		driver.click(*stripe);
		driver.settle();
		CHECK(!graph.fullViewItem());
		CHECK(scene.view.zoomFrame() == nullptr);
		CHECK(scene.view.taskPanel().isOpen());
		CHECK(scene.view.taskPanel().isSelfOrAncestorOf(scene.view.taskWidget()));
		CHECK(shownOnScreen(scene.view.taskWidget().carousel));
	}

	TEST(SwitchingMainViewWhileZoomedReturnsTheTaskWidgetToTheDock) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		graph.select(scene.release);
		graph.enterFullView(scene.release);
		driver.settle();
		REQUIRE CHECK(!scene.view.taskPanel().isOpen());

		driver.click(scene.view.views().tabButton(1));
		driver.settle();
		CHECK(scene.view.zoomFrame() == nullptr);
		CHECK(scene.view.taskPanel().isOpen());
		CHECK(scene.view.taskPanel().isSelfOrAncestorOf(scene.view.taskWidget()));
	}

	TEST(EscapeZoomsOutOfTheFullViewUnlessItsTitleOrEditorHasFocus) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.view.taskWidget().showPage(TaskWidgetPage::Task);
		driver.settle();
		TreeView& graph = scene.view.graph();
		graph.select(scene.release);
		graph.enterFullView(scene.release);
		driver.settle();
		TaskDescriptionPanel& description = scene.view.description();
		description.editor.setMode(MarkdownEditor::Mode::Editor);
		driver.settle();

		driver.click(description.header.title);
		driver.key(InputID::ESCAPE);
		driver.settle();
		CHECK(graph.fullViewItem() == std::optional<NodeId>(scene.release));

		driver.click(description.editor.sourceArea());
		driver.key(InputID::ESCAPE);
		driver.settle();
		CHECK(graph.fullViewItem() == std::optional<NodeId>(scene.release));

		driver.ui().setKeyboardFocus(&graph.fullViewFrame());
		REQUIRE CHECK(driver.ui().keyboardFocus() == &graph.fullViewFrame());
		driver.key(InputID::ESCAPE);
		driver.settle();
		CHECK(!graph.fullViewItem());
	}

	TEST(SavingUnderAnExistingNameReplacesThatPreset) {
		ui::testing::MemoryFileSystem files("/home/user");
		SessionPresets presets(files, "/home/user/taskboard");
		presets.save("smart", { "claude-opus-5-5", "claude-code", "high", "auto" });
		presets.save("dumb", { "claude-haiku", "claude-code", "low", "auto" });
		presets.save("smart", { "claude-opus-5-5", "dsh", "max", "auto" });
		REQUIRE CHECK_EQUAL(2u, presets.all().size());
		CHECK_EQUAL("max", presets.all()[0].settings.effort);
		CHECK(presets.matching({ "claude-haiku", "claude-code", "low", "auto" }) == std::optional<size_t>(1));
		CHECK(!presets.lastSaveFailed());
	}

	TEST(ChangedSettingsOfferRestart) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		scene.view.taskWidget().showPage(TaskWidgetPage::Session);
		Task& task = *scene.board.task(scene.release);
		task.session = { SessionState::Idle, "", task.launchSettings() };
		scene.view.openTask(scene.release);
		driver.settle();
		SessionTab& session = scene.view.session();
		CHECK(!session.isOfferingRestart());

		session.model.setSelectedIndex(1);
		driver.settle();
		CHECK_EQUAL("claude-opus-4-8", task.agentModel);
		CHECK(session.isOfferingRestart());
		CHECK(session.restartHint.text().find("claude-opus-5-5") != std::string::npos);

		scene.view.sessionScroll().jumpTo(scene.view.sessionScroll().maximumOffset());
		driver.settle();
		driver.click(session.restartKeep);
		driver.click(session.restartFresh);
		CHECK(
			scene.store.restarts ==
			(std::vector<std::pair<NodeId, bool>>{ { scene.release, false }, { scene.release, true } })
		);
	}

	TEST(OpenSessionGoesToCallbackForCurrentTask) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1200, 900 });
		std::vector<NodeId> opened;
		scene.view.setOnOpenSession([&](NodeId id) { opened.push_back(id); });

		driver.key(InputID::O, { .control = true });
		CHECK(opened.empty());
		scene.view.openTask(scene.emoji);
		CHECK_EQUAL(0u, driver.ui().openPopupCount());
		driver.key(InputID::O, { .control = true });
		CHECK(opened == std::vector<NodeId>{ scene.emoji });
	}
}

namespace {
	halnf length(Vec2F vector) { return std::sqrt(vector.x * vector.x + vector.y * vector.y); }
}

SUITE(TaskTreeGraph) {
	Vec2F cardPoint(TreeScene & scene, NodeId id) {
		TreeView& graph = scene.view.graph();
		const RectF card = *graph.cardRect(id);
		return graph.windowPosition() + card.pos + card.size / 2.0f;
	}

	Vec2F graphMiddle(TreeScene & scene) {
		TreeView& graph = scene.view.graph();
		return graph.windowPosition() + graph.size() / 2.0f;
	}

	bool isShown(TreeScene & scene, NodeId id) { return scene.view.graph().stateOf(id) == TreeItemState::Shown; }

	void showEverything(UiDriver & driver, TreeView & graph, NodeId around) {
		graph.openTo(std::nullopt, graph.structure().height());
		graph.select(around);
		driver.settle();
	}

	TEST(GraphHoldsTheWholeBoardFramedWhenTheWindowOpens) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		CHECK_EQUAL(4u, graph.structure().items().size());
		size_t shown = 0;
		for (const TreeStructure::Item& item : graph.structure().items()) {
			if (!isShown(scene, NodeId(item.id))) continue;
			shown++;
			const RectF card = *graph.cardRect(item.id);
			CHECK(contains(graph.localRect(), card.pos));
			CHECK(contains(graph.localRect(), card.pos + card.size));
		}
		CHECK(shown >= 2);
	}

	TEST(BoardLoadedAfterTheWindowOpensIsShown) {
		TaskBoard board;
		ApplicationFrame window;
		TaskTreeView view{ board };
		window.setContent(&view);
		view.attachMenus(window.menuBar, [] {});
		UiDriver driver(window, { 1400, 900 });
		driver.settle();

		const NodeId root = board.addTask({ .title = "Root" }, { 0, 0 });
		for (int index = 0; index < 4; index++) {
			const NodeId child = board.addTask({ .title = "Child" }, { 0, 0 });
			board.setParent(child, root);
		}
		view.boardReloaded();
		driver.settle();
		TreeView& graph = view.graph();
		CHECK_EQUAL(5u, graph.structure().items().size());
		const RectF card = *graph.cardRect(root);
		CHECK(contains(graph.localRect(), card.pos + card.size / 2.0f));
	}

	TEST(OpeningATaskSelectsAndCentresItWithoutReRooting) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.view.openTask(scene.emoji);
		driver.settle();
		TreeView& graph = scene.view.graph();
		CHECK_EQUAL(4u, graph.structure().items().size());
		CHECK(graph.selection() == scene.emoji);
		CHECK(isShown(scene, scene.emoji));
		CHECK(length(cardPoint(scene, scene.emoji) - graphMiddle(scene)) < 1);
	}

	TEST(TapSelectsAndShowsPanelsSecondTapOpensAndClosesNoDoubleTapFocus) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		showEverything(driver, graph, scene.widgets);
		graph.select(std::nullopt);
		driver.settle();

		driver.click(cardPoint(scene, scene.textEdit));
		driver.settle();
		CHECK(scene.view.currentTask() == scene.textEdit);
		CHECK(graph.selection() == scene.textEdit);
		CHECK(length(cardPoint(scene, scene.textEdit) - graphMiddle(scene)) < 1);
		CHECK(graph.isOpen(scene.textEdit));

		driver.click(cardPoint(scene, scene.textEdit));
		driver.settle();
		CHECK(!graph.isOpen(scene.textEdit));
		CHECK(!isShown(scene, scene.emoji));
		CHECK(scene.view.currentTask() == scene.textEdit);
		CHECK_EQUAL(4u, graph.structure().items().size());

		driver.click(cardPoint(scene, scene.textEdit));
		driver.settle();
		CHECK(graph.isOpen(scene.textEdit));
		CHECK(isShown(scene, scene.emoji));
	}

	TEST(HorizontalWheelOpensOnlyTheSelectedTasksChildrenVerticalOnlyZooms) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		graph.openTo(std::nullopt, 0);
		graph.select(std::nullopt);
		graph.frame(false);
		driver.settle();
		driver.click(cardPoint(scene, scene.widgets));
		driver.settle();
		driver.moveTo(graphMiddle(scene));
		const halnf scale = graph.targetScale();
		for (int notch = 0; notch < 40 && !graph.isOpen(scene.widgets); notch++)
			driver.horizontalWheel(1);
		driver.settle();
		CHECK(graph.isOpen(scene.widgets));
		CHECK(isShown(scene, scene.textEdit));
		CHECK(!isShown(scene, scene.emoji));
		CHECK(scene.view.currentTask() == scene.widgets);
		CHECK_CLOSE(scale, graph.targetScale(), 0.0001f);

		for (int notch = 0; notch < 8; notch++)
			driver.wheel(1);
		driver.settle();
		CHECK(graph.targetScale() > scale);
		CHECK(!graph.isOpen(scene.textEdit));
	}

	TEST(ArrowKeysMoveTheSelectionAfterClickingANode) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		showEverything(driver, graph, scene.widgets);
		driver.click(cardPoint(scene, scene.widgets));
		driver.settle();
		driver.key(InputID::RIGHT);
		driver.settle();
		CHECK(graph.selection() == scene.textEdit);
		CHECK(scene.view.currentTask() == scene.textEdit);
		driver.key(InputID::LEFT);
		driver.settle();
		CHECK(graph.selection() == scene.widgets);
	}

	TEST(GraphPlacesTasksAsATreeAndDragDoesNotMoveThem) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		showEverything(driver, graph, scene.textEdit);
		const RectF widgets = *graph.cardRect(scene.widgets);
		const RectF textEdit = *graph.cardRect(scene.textEdit);
		const RectF emoji = *graph.cardRect(scene.emoji);
		CHECK(textEdit.pos.x > widgets.pos.x + widgets.size.x);
		CHECK(emoji.pos.x > textEdit.pos.x + textEdit.size.x);

		const Vec2F point = cardPoint(scene, scene.textEdit);
		driver.drag(point, point + Vec2F(60.0f, 40.0f), 4);
		driver.settle();
		CHECK_CLOSE(textEdit.pos.x, graph.cardRect(scene.textEdit)->pos.x, 0.01f);
		CHECK_CLOSE(textEdit.pos.y, graph.cardRect(scene.textEdit)->pos.y, 0.01f);
	}

	TEST(DraggingFromChildrenPortCreatesAChildAndFocusesItForEditing) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		showEverything(driver, graph, scene.release);
		const size_t before = scene.board.graph().nodes().size();
		const Vec2F port = graph.windowPosition() + *graph.portPosition(scene.release, TreeView::Port::Children);
		driver.drag(port, port + Vec2F(160.0f, 200.0f), 6);
		driver.settle();

		REQUIRE CHECK_EQUAL(before + 1, scene.board.graph().nodes().size());
		const NodeId created = scene.board.graph().nodes().back().id;
		CHECK(scene.board.parentOf(created) == scene.release);
		CHECK(isShown(scene, created));
		CHECK(graph.selection() == created);
		CHECK(length(cardPoint(scene, created) - graphMiddle(scene)) < 1);

		CHECK(scene.view.currentTask() == created);
		CHECK(scene.view.session().isVisible());
		CHECK(scene.view.header().title.isEditing());
	}

	TEST(DeletingAChildFocusesItsParentWithoutReframing) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		showEverything(driver, graph, scene.textEdit);
		scene.view.openTask(scene.textEdit);
		driver.settle();
		const halnf scaleBefore = graph.targetScale();

		scene.chooseAction(driver, "Delete task...");
		driver.settle();
		REQUIRE CHECK(scene.view.header().deleteConfirm.isOpen());
		scene.view.header().deleteConfirm.accept();
		driver.settle();

		CHECK(scene.board.task(scene.textEdit) == nullptr);
		CHECK(scene.view.currentTask() == scene.widgets);
		CHECK(graph.selection() == scene.widgets);
		CHECK_CLOSE(scaleBefore, graph.targetScale(), 0.0001f);
	}

	TEST(DeletingAnUnsavedGraphTaskRemovesItsCardImmediately) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		showEverything(driver, graph, scene.release);
		const Vec2F port = graph.windowPosition() + *graph.portPosition(scene.release, TreeView::Port::Children);
		driver.drag(port, port + Vec2F(160.0f, 200.0f), 6);
		driver.settle();
		const std::optional<NodeId> created = scene.view.currentTask();
		REQUIRE CHECK(created.has_value());
		CHECK(graph.structure().indexOf(*created).has_value());

		scene.chooseAction(driver, "Delete task...");
		driver.settle();
		REQUIRE CHECK(scene.view.header().deleteConfirm.isOpen());
		scene.view.header().deleteConfirm.accept();
		driver.settle();

		CHECK(scene.board.task(*created) == nullptr);
		CHECK(!graph.structure().indexOf(*created).has_value());
		CHECK(scene.view.currentTask() == scene.release);
	}

	TEST(DraggingATaskOntoItsOwnSubtreeIsRefused) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		showEverything(driver, graph, scene.textEdit);

		const Vec2F port = graph.windowPosition() + *graph.portPosition(scene.widgets, TreeView::Port::Parent);
		driver.drag(port, cardPoint(scene, scene.emoji), 6);
		driver.settle();
		CHECK(!scene.board.parentOf(scene.widgets));
		CHECK(scene.board.parentOf(scene.emoji) == scene.textEdit);
	}

	TEST(ReparentingByDraggingRearrangesTheTree) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		showEverything(driver, graph, scene.textEdit);
		const Vec2F port = graph.windowPosition() + *graph.portPosition(scene.emoji, TreeView::Port::Parent);
		driver.drag(port, cardPoint(scene, scene.release), 6);
		driver.settle();
		CHECK(scene.board.parentOf(scene.emoji) == scene.release);
		const RectF release = *graph.cardRect(scene.release);
		const RectF emoji = *graph.cardRect(scene.emoji);
		CHECK(emoji.pos.x > release.pos.x + release.size.x);
		CHECK_CLOSE(release.center().y, emoji.center().y, 0.5f);
	}

	TEST(DeletingKeepsTheTreeFocusedSoArrowsKeepWorking) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		showEverything(driver, graph, scene.emoji);
		scene.view.openTask(scene.emoji);
		driver.settle();
		driver.ui().setKeyboardFocus(&graph);
		REQUIRE CHECK(graph.selection() == scene.emoji);

		driver.key(InputID::DELETE_KEY);
		driver.settle();
		REQUIRE CHECK(scene.view.header().deleteConfirm.isOpen());
		scene.view.header().deleteConfirm.accept();
		driver.settle();
		REQUIRE CHECK(scene.board.task(scene.emoji) == nullptr);
		CHECK(driver.ui().keyboardFocus() == &graph);

		REQUIRE CHECK(graph.selection() == scene.textEdit);
		driver.key(InputID::LEFT);
		driver.settle();
		CHECK(graph.selection() == scene.widgets);
	}

	TEST(DeleteKeyRemovesEverySelectedTaskAfterConfirming) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		const NodeId spare = scene.board.addTask({ .title = "Spare" }, { 0, 0 });
		scene.board.setParent(spare, scene.emoji);
		scene.view.boardReloaded();
		showEverything(driver, graph, scene.widgets);

		driver.click(cardPoint(scene, scene.textEdit));
		driver.settle();
		driver.click(cardPoint(scene, scene.release), { .control = true });
		driver.settle();
		REQUIRE CHECK_EQUAL(2u, graph.selectedItems().size());

		driver.key(InputID::DELETE_KEY);
		driver.settle();
		REQUIRE CHECK(scene.view.graphDeleteConfirm().isOpen());
		scene.view.graphDeleteConfirm().accept();
		driver.settle();

		CHECK(scene.board.task(scene.textEdit) == nullptr);
		CHECK(scene.board.task(scene.release) == nullptr);
		CHECK(scene.board.parentOf(scene.emoji) == scene.widgets);
		CHECK(scene.board.parentOf(spare) == scene.emoji);
		CHECK_EQUAL(3u, scene.board.graph().nodes().size());
		CHECK(!graph.selection());
		CHECK(!scene.view.currentTask());
	}

	TEST(DeleteKeyOnOneSelectedTaskUsesThePageConfirmation) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		driver.click(cardPoint(scene, scene.release));
		driver.settle();
		driver.key(InputID::DELETE_KEY);
		driver.settle();
		CHECK(scene.view.header().deleteConfirm.isOpen());
		CHECK(scene.view.currentTask() == scene.release);
	}

	TEST(DetailedTreeCardsShowTheSharedTaskCardWithoutAHeader) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.board.task(scene.release)->session.state = SessionState::Working;
		scene.board.task(scene.release)->description = "Collect the changes";
		scene.view.boardReloaded();
		driver.settle();
		TreeView& graph = scene.view.graph();
		graph.setView(graph.pan(), 1, false);
		graph.select(scene.release);
		driver.settle();

		auto* card = dynamic_cast<TaskTreeCard*>(graph.card(scene.release));
		REQUIRE CHECK(card != nullptr);
		CHECK(!card->isHeaderShown());
		CHECK(card->content().isVisible());
		TaskCardView& view = card->view();
		CHECK_EQUAL("#4", view.number.text());
		CHECK_EQUAL("Release notes", view.title.text());
		CHECK_EQUAL("Collect the changes", view.description.text());
		CHECK_EQUAL(std::string(statusLabel(scene.board.task(scene.release)->status)), view.status.text());
		CHECK(view.session.isVisible());
		CHECK(view.session.isPulsing());
		CHECK(view.session.isDotShown());
		CHECK(!view.status.isDotShown());
		CHECK(view.title.windowRect().pos.y > view.number.windowRect().pos.y);
		CHECK(view.description.windowRect().pos.y > view.title.windowRect().pos.y);
		CHECK(view.description.wraps());
	}

	TEST(TreeAndKanbanCardsShareOneTaskCardAndSize) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.board.task(scene.release)->description = std::string(600, 'x') + " tail";
		scene.board.task(scene.release)->status = TaskStatus::Queued;
		scene.view.boardReloaded();
		driver.settle();
		TreeView& graph = scene.view.graph();
		graph.setView(graph.pan(), 1, false);
		driver.settle();
		const RectF treeCard = *graph.cardRect(scene.release);

		driver.click(scene.view.views().tabButton(1));
		driver.settle();
		auto* kanbanCard = dynamic_cast<TaskKanbanCard*>(scene.view.kanban().card(scene.release));
		REQUIRE CHECK(kanbanCard != nullptr);
		CHECK_CLOSE(treeCard.size.y, kanbanCard->windowRect().size.y, 1.0f);
		TaskCardView& view = kanbanCard->view();
		CHECK_EQUAL("Release notes", view.title.text());
		CHECK(!view.status.isVisible());
		const RectF description = view.description.windowRect();
		const RectF cardRect = kanbanCard->windowRect();
		CHECK(description.pos.y + description.size.y <= cardRect.pos.y + cardRect.size.y + 0.5f);
	}

	TEST(OneTaskPanelOnTheRightHoldsEveryTaskPageAndThePathRunsAcrossTop) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.view.openTask(scene.release);
		driver.settle();

		const RectF graph = scene.view.graph().windowRect();
		const RectF widget = scene.view.taskWidget().windowRect();
		const RectF path = scene.view.breadcrumb().windowRect();
		const RectF view = scene.view.windowRect();
		CHECK(scene.view.taskPanel().side() == DockSide::Right);
		CHECK_EQUAL(1u, scene.view.dock().panels().size());
		CHECK(widget.pos.x >= graph.pos.x + graph.size.x - 1);
		CHECK(path.pos.y >= graph.pos.y + graph.size.y - 1);
		CHECK(path.pos.y + path.size.y <= scene.view.status.windowRect().pos.y + 1);
		CHECK(path.size.x > view.size.x - 40);
		CHECK(scene.window.menuBar.centerWidget() == &scene.view.search());

		TabView& carousel = scene.view.taskWidget().carousel;
		REQUIRE CHECK_EQUAL(4u, carousel.tabCount());
		CHECK_EQUAL("Task", carousel.tabButton(0).text());
		CHECK_EQUAL("Session", carousel.tabButton(1).text());
		CHECK_EQUAL("Workdir", carousel.tabButton(2).text());
		CHECK_EQUAL("Links", carousel.tabButton(3).text());

		scene.view.taskWidget().showPage(TaskWidgetPage::Task);
		scene.view.openTask(std::nullopt);
		driver.settle();
		CHECK(!shownOnScreen(scene.view.description().editor));
		CHECK(driver.paint().hasText("Open a task to see and edit its description."));
	}
}

SUITE(TaskDescriptionPanel) {
	TEST(DescriptionIsAMarkdownPreviewFillingThePanelUnderAnIdAndTitleRow) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.view.taskWidget().showPage(TaskWidgetPage::Task);
		scene.board.task(scene.release)->description = "# Plan\n\nShip **it**";
		scene.view.openTask(scene.release);
		driver.settle();

		TaskDescriptionPanel& panel = scene.view.description();
		CHECK(panel.editor.mode() == MarkdownEditor::Mode::Preview);
		CHECK(shownOnScreen(panel.editor.preview()));
		CHECK(!panel.editor.showsSource());
		CHECK_EQUAL("#4", panel.header.idLabel.text());
		CHECK_EQUAL("Release notes", panel.header.title.text());
		CHECK(driver.paint().hasText("Plan"));

		const RectF panelBody = scene.view.taskWidget().body().windowRect();
		const RectF editor = panel.editor.windowRect();
		CHECK(editor.pos.y > panel.header.title.windowRect().pos.y);
		CHECK(editor.pos.y + editor.size.y > panelBody.pos.y + panelBody.size.y - 30);
		CHECK(editor.size.x > panelBody.size.x - 40);
	}

	TEST(RenamingInTheDescriptionPanelEditsTheTaskAndReportsTheTitleScope) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.view.taskWidget().showPage(TaskWidgetPage::Task);
		scene.view.openTask(scene.release);
		driver.settle();

		TaskTitleField& title = scene.view.header().title;
		driver.click(title);
		title.selectAll();
		driver.type("Changelog");
		REQUIRE CHECK(scene.store.editing.has_value());
		CHECK(scene.store.editing->second == TaskField::Title);
		CHECK_EQUAL("Changelog", scene.board.task(scene.release)->title);
		CHECK_EQUAL("Changelog", scene.view.header().title.text());
		CHECK(scene.edits > 0);
	}
}

SUITE(TaskWorkspacePanel) {
	struct WorkspaceScene : TreeScene {
		UiDriver driver{ window, { 1400, 900 } };

		WorkspaceScene() {
			view.taskWidget().showPage(TaskWidgetPage::Workdir);
			view.openTask(release);
			driver.settle();
		}

		void open(const std::string& name) {
			FileRow* row = view.workspace().browser.row(name);
			REQUIRE CHECK(row != nullptr);
			driver.click(*row);
			driver.click(*row);
			driver.settle();
		}
	};

	std::vector<std::string> listedNames(const FileBrowser& browser) {
		std::vector<std::string> names;
		for (const auto& row : browser.rows())
			names.push_back(row->entry().name);
		return names;
	}

	TEST(ListsTheTaskWorkspaceStartingAtItsRoot) {
		WorkspaceScene scene;
		TaskWorkspacePanel& panel = scene.view.workspace();
		CHECK_EQUAL("/agents/task-4", panel.browser.directory());
		CHECK((listedNames(panel.browser) == std::vector<std::string>{ "notes", "reports" }));
		CHECK(!panel.browser.up.isEnabled());
		CHECK(shownOnScreen(panel.browser));
	}

	TEST(WorkdirHeaderAppearsOnlyForAnOpenedFile) {
		WorkspaceScene scene;
		TaskWorkspacePanel& panel = scene.view.workspace();
		CHECK(!shownOnScreen(panel.pages.currentPage().header()));
		CHECK(shownOnScreen(panel.browser.path));

		scene.open("reports");
		scene.open("summary.md");
		REQUIRE CHECK_EQUAL(2u, panel.pages.depth());
		CHECK(shownOnScreen(panel.pages.currentPage().header()));
	}

	TEST(FoldersOpenInPlaceAndOnlyMarkdownFilesAreListed) {
		WorkspaceScene scene;
		TaskWorkspacePanel& panel = scene.view.workspace();
		scene.open("reports");
		CHECK_EQUAL("/agents/task-4/reports", panel.browser.directory());
		CHECK_EQUAL(1u, panel.pages.depth());
		CHECK((listedNames(panel.browser) == std::vector<std::string>{ "summary.md" }));

		scene.driver.click(panel.browser.up);
		CHECK_EQUAL("/agents/task-4", panel.browser.directory());
		CHECK(!panel.browser.goUp());
	}

	TEST(MarkdownOpensInAPreviewPageAndBackReturnsToTheFolder) {
		WorkspaceScene scene;
		TaskWorkspacePanel& panel = scene.view.workspace();
		scene.open("reports");
		scene.open("summary.md");

		CHECK_EQUAL(2u, panel.pages.depth());
		CHECK_EQUAL("reports/summary.md", panel.pages.currentPage().title());
		CHECK_EQUAL("/agents/task-4/reports/summary.md", panel.openedDocument());
		CHECK(panel.document.mode() == MarkdownEditor::Mode::Preview);
		CHECK(shownOnScreen(panel.document.preview()));
		CHECK(scene.driver.paint().hasText("All done."));

		scene.driver.click(panel.pages.currentPage().backButton());
		scene.driver.settle();
		CHECK_EQUAL(1u, panel.pages.depth());
		CHECK(panel.openedDocument().empty());
		CHECK_EQUAL("/agents/task-4/reports", panel.browser.directory());
		CHECK(shownOnScreen(panel.browser));
	}

	TEST(EditingAnOpenedDocumentWritesItBack) {
		WorkspaceScene scene;
		TaskWorkspacePanel& panel = scene.view.workspace();
		panel.openDocument("/agents/task-4/reports/summary.md");
		panel.document.setMode(MarkdownEditor::Mode::Editor);
		scene.driver.settle();
		scene.driver.click(panel.document.sourceArea());
		scene.driver.type("!");
		CHECK_EQUAL(panel.document.markdown(), *scene.files.readText("/agents/task-4/reports/summary.md"));
		CHECK(scene.files.readText("/agents/task-4/reports/summary.md")->find('!') != std::string::npos);
	}

	TEST(OtherFileTypesAreNotOpened) {
		WorkspaceScene scene;
		TaskWorkspacePanel& panel = scene.view.workspace();
		panel.openDocument("/agents/task-4/reports/data.csv");
		CHECK_EQUAL(1u, panel.pages.depth());
		CHECK(panel.openedDocument().empty());
	}

	TEST(SwitchingTasksLeavesTheDocumentAndExplainsMissingWorkspaces) {
		WorkspaceScene scene;
		TaskWorkspacePanel& panel = scene.view.workspace();
		panel.openDocument("/agents/task-4/reports/summary.md");
		scene.view.openTask(scene.widgets);
		scene.driver.settle();

		CHECK_EQUAL(1u, panel.pages.depth());
		CHECK(!shownOnScreen(panel.browser));
		CHECK(shownOnScreen(panel.hint));
		CHECK_EQUAL("This task has no workdir yet. It appears once its agent session starts.", panel.hint.text());

		scene.view.openTask(std::nullopt);
		scene.driver.settle();
		CHECK(shownOnScreen(panel.hint));
		CHECK_EQUAL("Open a task to browse the files its agent produced.", panel.hint.text());
	}
}

SUITE(TaskKanban) {
	size_t statusColumn(TreeScene & scene, TaskStatus status) { return scene.view.kanbanColumn(status); }

	void showKanbanTab(UiDriver & driver, TreeScene & scene) {
		driver.click(scene.view.views().tabButton(1));
		driver.settle();
	}

	void showKanban(UiDriver & driver, TreeScene & scene) {
		scene.view.kanban().setColumnVisible(statusColumn(scene, TaskStatus::Backlog), true);
		showKanbanTab(driver, scene);
	}

	std::vector<KanbanItemId> cardsIn(TreeScene & scene, TaskStatus status) {
		std::vector<KanbanItemId> ids;
		for (const KanbanCard* card : scene.view.kanban().columnView(statusColumn(scene, status)).stack().cards())
			ids.push_back(card->id());
		return ids;
	}

	std::vector<std::string> shownColumnTitles(KanbanBoard & kanban) {
		std::vector<std::string> titles;
		for (size_t column = 0; column < kanban.columnCount(); column++)
			if (shownOnScreen(kanban.columnView(column))) titles.push_back(kanban.columnView(column).title().text());
		return titles;
	}

	TEST(DoubleClickingAKanbanCardOpensItsDescriptionFullViewAndBackReturns) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.view.taskWidget().showPage(TaskWidgetPage::Task);
		driver.settle();
		showKanban(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();
		KanbanCard& card = *kanban.card(scene.release);
		scene.view.dock().hidePanel(scene.view.taskPanel());
		driver.settle();
		REQUIRE CHECK(!scene.view.taskPanel().isOpen());

		driver.click(card);
		driver.click(card);
		driver.settle();
		REQUIRE CHECK(kanban.fullViewItem() == std::optional<KanbanItemId>(scene.release));
		CHECK(!scene.view.taskPanel().isOpen());
		TaskDescriptionPanel& description = scene.view.description();
		CHECK(shownOnScreen(description.editor));
		CHECK_EQUAL(scene.board.task(scene.release)->title, description.header.title.text());
		CHECK(scene.view.currentTask() == std::optional<NodeId>(scene.release));
		CHECK(!shownOnScreen(kanban.columnView(0)));

		driver.click(scene.view.header().back);
		driver.settle();
		CHECK(!kanban.fullViewItem());
		CHECK(!shownOnScreen(description));
		CHECK(shownOnScreen(card));
	}

	TEST(DockedPanelShowsTheSameBadgeRowAboveTheFramedTaskBody) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.view.openTask(scene.release);
		driver.settle();

		TaskWidget& widget = scene.view.taskWidget();
		REQUIRE CHECK(scene.view.taskPanel().isSelfOrAncestorOf(widget));
		const RectF badges = scene.view.header().badgeRow().windowRect();
		const RectF body = widget.body().windowRect();
		CHECK(shownOnScreen(scene.view.header().idLabel));
		CHECK(!shownOnScreen(scene.view.header().back));
		CHECK(badges.pos.y + badges.size.y <= body.pos.y - 4);
		CHECK(widget.carousel.tabButton(0).windowRect().pos.y >= body.pos.y + 4);

		const RGBA surface = driver.ui().theme().palette.surface;
		bool bodyPainted = false;
		for (const PaintCommand& command : driver.paint().commands())
			bodyPainted |= command.kind == PaintCommand::Kind::FillRect && command.color == surface &&
										 std::abs(command.rect.size.x - body.size.x) < 1 && std::abs(command.rect.size.y - body.size.y) < 1;
		CHECK(bodyPainted);
	}

	TEST(BadgeRowSitsAboveTheFramedTaskBodyInsideTheFullView) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		showKanban(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();
		kanban.enterFullView(scene.release, false);
		driver.settle();

		TaskWidget& widget = scene.view.taskWidget();
		const RectF frame = kanban.fullViewFrame().windowRect();
		const RectF content = widget.windowRect();
		CHECK(content.pos.x - frame.pos.x >= FullViewFrame::kAccentInset + FullViewFrame::kAccentWidth + 4);
		CHECK(content.pos.y - frame.pos.y >= 4);
		CHECK(frame.pos.x + frame.size.x - (content.pos.x + content.size.x) >= 4);

		const RectF badges = scene.view.header().badgeRow().windowRect();
		const RectF body = widget.body().windowRect();
		CHECK(scene.view.header().badgeRow().parent() == &widget);
		CHECK(widget.carousel.parent() == &widget.body());
		CHECK(badges.pos.y + badges.size.y <= body.pos.y - 4);
		CHECK(widget.carousel.tabButton(0).windowRect().pos.y >= body.pos.y + 4);

		const RectF previous = widget.carousel.previousButton().windowRect();
		CHECK(previous.pos.x - content.pos.x >= 4);
	}

	TEST(FullViewIsTheGrownCardWithTheSameBadgeRowAndBack) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		const RGBA color = kTaskColors[2];
		scene.board.task(scene.release)->color = color;
		scene.view.boardReloaded();
		TaskHeader& header = scene.view.header();
		Widget* const badgeRowParent = header.badgeRow().parent();
		CHECK(!header.back.isVisible());

		showKanban(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();
		kanban.enterFullView(scene.release, false);
		driver.settle();

		FullViewFrame& frame = kanban.fullViewFrame();
		REQUIRE CHECK(frame.accent().has_value());
		CHECK(*frame.accent() == color);
		CHECK(frame.content() == &scene.view.taskWidget());
		auto* preview = dynamic_cast<TaskCardView*>(frame.preview());
		REQUIRE CHECK(preview != nullptr);
		CHECK_EQUAL(scene.board.task(scene.release)->title, preview->title.text());
		CHECK(header.badgeRow().parent() == badgeRowParent);
		CHECK(shownOnScreen(header.back));
		CHECK_EQUAL("\u2190 Back", header.back.text());
		CHECK(shownOnScreen(header.idLabel));
		CHECK_CLOSE(header.back.windowRect().center().y, header.idLabel.windowRect().center().y, 2.0f);
		CHECK(header.idLabel.windowRect().pos.x > header.back.windowRect().pos.x);

		scene.view.taskWidget().showPage(TaskWidgetPage::Links);
		driver.settle();
		CHECK(shownOnScreen(header.idLabel));
		CHECK(shownOnScreen(header.back));

		driver.click(header.back);
		driver.settle();
		CHECK(!kanban.fullViewItem());
		CHECK(!header.back.isVisible());
		CHECK(header.badgeRow().parent() == badgeRowParent);
	}

	TEST(KanbanDeleteKeyAsksLikeTheTreeAndArrowsMoveTheSelection) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		showKanban(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();
		const size_t column = statusColumn(scene, scene.board.task(scene.release)->status);
		const std::vector<KanbanCard*>& cards = kanban.columnView(column).stack().cards();
		REQUIRE CHECK(cards.size() >= 2);
		const NodeId first = cards[0]->id();
		const NodeId second = cards[1]->id();

		driver.click(*kanban.card(first));
		driver.settle();
		CHECK(driver.ui().keyboardFocus() == &kanban);
		driver.key(InputID::DOWN);
		driver.settle();
		CHECK(kanban.selection() == std::optional<KanbanItemId>(second));
		CHECK(scene.view.currentTask() == std::optional<NodeId>(second));

		driver.key(InputID::DELETE_KEY);
		driver.settle();
		REQUIRE CHECK(scene.view.header().deleteConfirm.isOpen());
		scene.view.header().deleteConfirm.accept();
		driver.settle();
		CHECK(scene.board.task(second) == nullptr);
		CHECK(driver.ui().keyboardFocus() == &kanban);
	}

	TEST(TreeAndKanbanAreTabsOfTheMainView) {
		TreeScene scene(false);
		UiDriver driver(scene.window, { 1400, 900 });

		REQUIRE CHECK_EQUAL(4u, scene.view.views().tabCount());
		CHECK_EQUAL("Tree", scene.view.views().tabButton(0).text());
		CHECK_EQUAL("Kanban", scene.view.views().tabButton(1).text());
		CHECK_EQUAL("Skills", scene.view.views().tabButton(2).text());
		CHECK_EQUAL("Secrets", scene.view.views().tabButton(3).text());
		CHECK(shownOnScreen(scene.view.graph()));
		CHECK(!shownOnScreen(scene.view.kanban()));

		showKanban(driver, scene);
		CHECK(shownOnScreen(scene.view.kanban()));
		CHECK(!shownOnScreen(scene.view.graph()));
	}

	TEST(ColumnsAreTaskStatusesInTheGlobalTaskOrder) {
		TreeScene scene(false);
		scene.board.task(scene.emoji)->status = TaskStatus::Wip;
		scene.board.task(scene.widgets)->order = 3;
		scene.board.task(scene.textEdit)->order = 1;
		scene.board.task(scene.release)->order = 2;
		scene.view.boardReloaded();
		UiDriver driver(scene.window, { 1400, 900 });
		showKanban(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();

		REQUIRE CHECK_EQUAL(kTaskStatuses.size(), kanban.columnCount());
		CHECK_EQUAL("Backlog", kanban.columnView(statusColumn(scene, TaskStatus::Backlog)).title().text());
		CHECK_EQUAL("Queued", kanban.columnView(statusColumn(scene, TaskStatus::Queued)).title().text());
		CHECK_EQUAL("WIP", kanban.columnView(statusColumn(scene, TaskStatus::Wip)).title().text());
		CHECK_EQUAL(statusColumn(scene, TaskStatus::Wip), *kanban.columnOf(scene.emoji));

		CHECK(
			(cardsIn(scene, TaskStatus::Backlog) == std::vector<KanbanItemId>{ scene.textEdit, scene.release, scene.widgets })
		);
		for (size_t column = 0; column < kanban.columnCount(); column++)
			CHECK(!kanban.columns()[column].accent.has_value());
		CHECK_EQUAL("#2  Text edit glitches", kanban.card(scene.textEdit)->title());
		CHECK_EQUAL("space and emoji", kanban.card(scene.textEdit)->detail());
		CHECK_EQUAL("No description", kanban.card(scene.widgets)->detail());
	}

	TEST(RetiredStatusNamesInSavedColumnSettingsAreIgnored) {
		MemoryFileSystem files;
		files.addTextFile("/home/user/taskboard/kanban-columns.json", R"({"hidden": ["backlog", "ready", "failed"]})");

		KanbanColumnSettings settings(files, "/home/user/taskboard");

		CHECK((settings.hidden() == std::vector<TaskStatus>{ TaskStatus::Backlog }));
	}

	TEST(SavedColumnOrderIsCompletedWithStatusesItDoesNotName) {
		MemoryFileSystem files;
		files.addTextFile(
			"/home/user/taskboard/kanban-columns.json", R"({"order": ["done", "running", "wip", "done", 7], "hidden": []})"
		);

		KanbanColumnSettings settings(files, "/home/user/taskboard");

		std::vector<TaskStatus> expected = { TaskStatus::Done, TaskStatus::Wip };
		for (const TaskStatus status : kTaskStatuses)
			if (status != TaskStatus::Done && status != TaskStatus::Wip) expected.push_back(status);
		CHECK((settings.order() == expected));
		CHECK(settings.hidden().empty());
	}

	TEST(ColumnSettingsWithoutAnOrderKeepTheDefaultOrder) {
		MemoryFileSystem files;
		files.addTextFile("/home/user/taskboard/kanban-columns.json", R"({"hidden": ["done"]})");

		KanbanColumnSettings settings(files, "/home/user/taskboard");

		CHECK((settings.order() == KanbanColumnSettings::orderByDefault()));
		CHECK((settings.hidden() == std::vector<TaskStatus>{ TaskStatus::Done }));
	}

	TEST(ColumnOrderSurvivesARestartOnDisk) {
		const std::filesystem::path directory =
			std::filesystem::temp_directory_path() / ("taskboard-columns-" + std::to_string(::getpid()));
		std::filesystem::remove_all(directory);

		{
			KanbanColumnSettings settings(FileSystem::local(), directory.string());
			settings.moveColumn(settings.order().size() - 1, 0);
			settings.setVisible(TaskStatus::Blocked, true);
		}
		KanbanColumnSettings restarted(FileSystem::local(), directory.string());

		CHECK(restarted.order().front() == TaskStatus::Blocked);
		CHECK_EQUAL(kTaskStatuses.size(), restarted.order().size());
		const std::vector<TaskStatus>& hidden = restarted.hidden();
		CHECK(std::find(hidden.begin(), hidden.end(), TaskStatus::Blocked) == hidden.end());
		std::filesystem::remove_all(directory);
	}

	TEST(BacklogAndRarelyUsedColumnsAreHiddenByDefault) {
		TreeScene scene(false);
		UiDriver driver(scene.window, { 1400, 900 });
		showKanbanTab(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();

		CHECK((shownColumnTitles(kanban) == std::vector<std::string>{ "Queued", "WIP", "Review", "Done" }));
		CHECK(!shownOnScreen(*kanban.card(scene.release)));
	}

	TEST(ColumnMenuShowsAColumnAndTheChoiceIsRemembered) {
		TreeScene scene(false);
		UiDriver driver(scene.window, { 1400, 900 });
		showKanbanTab(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();
		KanbanColumnView& queued = kanban.columnView(statusColumn(scene, TaskStatus::Queued));

		driver.click(queued.optionsButton());
		driver.settle();
		REQUIRE CHECK(queued.optionsMenu().isOpen());
		const auto& items = queued.optionsMenu().items();
		REQUIRE CHECK_EQUAL(kTaskStatuses.size(), items.size());
		driver.click(*items[statusColumn(scene, TaskStatus::Backlog)]);
		driver.click(queued.optionsButton());
		driver.settle();
		driver.click(*items[statusColumn(scene, TaskStatus::Done)]);
		driver.settle();

		CHECK((shownColumnTitles(kanban) == std::vector<std::string>{ "Backlog", "Queued", "WIP", "Review" }));
		CHECK(shownOnScreen(*kanban.card(scene.release)));

		TaskTreeView reopened{ scene.board, scene.files, scene.storage };
		CHECK(reopened.kanban().isColumnVisible(reopened.kanbanColumn(TaskStatus::Backlog)));
		CHECK(!reopened.kanban().isColumnVisible(reopened.kanbanColumn(TaskStatus::Done)));
		CHECK(reopened.kanban().isColumnVisible(reopened.kanbanColumn(TaskStatus::Queued)));
	}

	TEST(DraggingAColumnHeaderMovesTheColumnAndRemembersTheOrder) {
		TreeScene scene(false);
		scene.board.task(scene.widgets)->status = TaskStatus::Wip;
		scene.view.boardReloaded();
		UiDriver driver(scene.window, { 1600, 900 });
		showKanbanTab(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();

		const RectF review = kanban.columnView(statusColumn(scene, TaskStatus::Review)).windowRect();
		const Vec2F reviewRightHalf = { review.pos.x + review.size.x * 0.75f, review.center().y };
		driver.drag(driver.centerOf(kanban.columnView(statusColumn(scene, TaskStatus::Wip)).title()), reviewRightHalf, 10);
		driver.settle();

		CHECK((shownColumnTitles(kanban) == std::vector<std::string>{ "Queued", "Review", "WIP", "Done" }));
		CHECK((cardsIn(scene, TaskStatus::Wip) == std::vector<KanbanItemId>{ scene.widgets }));
		CHECK(scene.board.task(scene.widgets)->status == TaskStatus::Wip);

		TaskTreeView reopened{ scene.board, scene.files, scene.storage };
		CHECK(reopened.kanbanColumn(TaskStatus::Review) < reopened.kanbanColumn(TaskStatus::Wip));
		CHECK_EQUAL("WIP", reopened.kanban().columnView(reopened.kanbanColumn(TaskStatus::Wip)).title().text());
		CHECK(!reopened.kanban().isColumnVisible(reopened.kanbanColumn(TaskStatus::Backlog)));
	}

	TEST(ACardDroppedIntoAMovedColumnTakesThatColumnsStatus) {
		TreeScene scene(false);
		UiDriver driver(scene.window, { 1600, 900 });
		showKanban(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();
		kanban.moveColumn(statusColumn(scene, TaskStatus::Review), 0);
		driver.settle();
		REQUIRE CHECK_EQUAL(0u, statusColumn(scene, TaskStatus::Review));

		driver.drag(driver.centerOf(*kanban.card(scene.release)), kanban.columnView(0).windowRect().center(), 10);
		driver.settle();

		CHECK(scene.board.task(scene.release)->status == TaskStatus::Review);
		CHECK((cardsIn(scene, TaskStatus::Review) == std::vector<KanbanItemId>{ scene.release }));
	}

	TEST(DroppingACardBetweenTwoCardsPlacesItThereInTheGlobalOrder) {
		TreeScene scene(false);
		for (NodeId id : { scene.widgets, scene.textEdit, scene.emoji, scene.release })
			scene.board.task(id)->status = TaskStatus::Queued;
		scene.board.task(scene.widgets)->order = 0;
		scene.board.task(scene.textEdit)->order = 1;
		scene.board.task(scene.emoji)->order = 2;
		scene.board.task(scene.release)->order = 3;
		scene.view.boardReloaded();
		UiDriver driver(scene.window, { 1600, 900 });
		showKanban(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();

		const RectF target = kanban.card(scene.textEdit)->windowRect();
		driver.drag(driver.centerOf(*kanban.card(scene.release)), { target.center().x, target.pos.y + 4 }, 10);
		driver.settle();

		const std::vector<KanbanItemId> expected = { scene.widgets, scene.release, scene.textEdit, scene.emoji };
		CHECK((cardsIn(scene, TaskStatus::Queued) == expected));
		CHECK((scene.board.tasksInOrder() == std::vector<NodeId>(expected.begin(), expected.end())));
		CHECK_EQUAL(1, scene.edits);
	}

	TEST(TheKanbanOrderIsTheOrderOfChildrenInTheTree) {
		TreeScene scene(false);
		const NodeId second =
			scene.board.addTask({ .title = "Second child", .status = TaskStatus::Queued, .order = 5 }, { 0, 0 });
		scene.board.setParent(second, scene.widgets);
		scene.board.task(scene.textEdit)->status = TaskStatus::Queued;
		scene.board.task(scene.textEdit)->order = 4;
		scene.view.boardReloaded();
		UiDriver driver(scene.window, { 1600, 900 });
		showKanban(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();
		REQUIRE CHECK((scene.board.childrenOf(scene.widgets) == std::vector<NodeId>{ scene.textEdit, second }));

		const RectF target = kanban.card(scene.textEdit)->windowRect();
		driver.drag(driver.centerOf(*kanban.card(second)), { target.center().x, target.pos.y + 4 }, 10);
		driver.settle();

		CHECK((scene.board.childrenOf(scene.widgets) == std::vector<NodeId>{ second, scene.textEdit }));
	}

	TEST(CardsHaveOneFixedHeightWhateverTheDescription) {
		TreeScene scene(false);
		scene.board.task(scene.emoji)->description = std::string(2000, 'x') + " long description";
		scene.view.boardReloaded();
		UiDriver driver(scene.window, { 1400, 900 });
		showKanban(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();

		const halnf height = kanban.card(scene.widgets)->size().y;
		CHECK(height > 60);
		CHECK_CLOSE(height, kanban.card(scene.emoji)->size().y, 0.01f);
		CHECK_CLOSE(height, kanban.card(scene.textEdit)->size().y, 0.01f);
	}

	TEST(DraggingACardToAnotherColumnChangesTheTaskStatus) {
		TreeScene scene(false);
		UiDriver driver(scene.window, { 1600, 900 });
		showKanban(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();
		const size_t review = statusColumn(scene, TaskStatus::Review);

		driver.drag(driver.centerOf(*kanban.card(scene.release)), kanban.columnView(review).windowRect().center(), 10);
		driver.settle();

		CHECK(scene.board.task(scene.release)->status == TaskStatus::Review);
		CHECK_EQUAL(review, *kanban.columnOf(scene.release));
		CHECK_EQUAL(1, scene.edits);
	}

	TEST(ReadOnlyBoardPutsADraggedCardBack) {
		TreeScene scene(false);
		UiDriver driver(scene.window, { 1600, 900 });
		showKanban(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();
		const size_t original = *kanban.columnOf(scene.release);
		scene.view.setReadOnly(true);

		driver.drag(
			driver.centerOf(*kanban.card(scene.release)),
			kanban.columnView(statusColumn(scene, TaskStatus::Review)).windowRect().center(),
			10
		);
		driver.settle();

		CHECK(scene.board.task(scene.release)->status == TaskStatus::Backlog);
		CHECK_EQUAL(original, *kanban.columnOf(scene.release));
		CHECK_EQUAL(0, scene.edits);
	}

	TEST(StatusChangedElsewhereMovesTheCard) {
		TreeScene scene(false);
		UiDriver driver(scene.window, { 1400, 900 });
		showKanban(driver, scene);

		scene.board.task(scene.widgets)->status = TaskStatus::Done;
		scene.view.boardReloaded();
		driver.settle();

		CHECK_EQUAL(statusColumn(scene, TaskStatus::Done), *scene.view.kanban().columnOf(scene.widgets));
	}

	TEST(ClickingACardOpensTheTaskAndOpeningATaskSelectsItsCard) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1600, 900 });
		showKanban(driver, scene);
		KanbanBoard& kanban = scene.view.kanban();

		driver.click(*kanban.card(scene.emoji));
		driver.settle();
		CHECK(scene.view.currentTask() == scene.emoji);
		CHECK(kanban.selection() == scene.emoji);

		scene.view.openTask(scene.release);
		driver.settle();
		CHECK(kanban.selection() == scene.release);
		CHECK(kanban.card(scene.release)->isSelected());
		CHECK(!kanban.card(scene.emoji)->isSelected());
	}

	TEST(TaskSelectedInKanbanIsShownCentredWhenSwitchingToTree) {
		TreeScene scene(false);
		UiDriver driver(scene.window, { 1600, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		graph.openTo(std::nullopt, 0);
		driver.settle();
		REQUIRE CHECK(graph.stateOf(scene.emoji) != TreeItemState::Shown);

		showKanban(driver, scene);
		driver.click(*scene.view.kanban().card(scene.emoji));
		driver.settle();
		driver.click(scene.view.views().tabButton(0));
		driver.settle();

		CHECK(graph.selection() == scene.emoji);
		CHECK(graph.stateOf(scene.emoji) == TreeItemState::Shown);
		const RectF card = *graph.cardRect(scene.emoji);
		const Vec2F centre = card.pos + card.size / 2.0f;
		CHECK_CLOSE(graph.size().x / 2, centre.x, 1.0f);
		CHECK_CLOSE(graph.size().y / 2, centre.y, 1.0f);
	}

	TEST(SwitchingBackToTreeRecentresTheSelectedTaskAfterPanningAway) {
		TreeScene scene(false);
		UiDriver driver(scene.window, { 1600, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		scene.view.openTask(scene.emoji);
		driver.settle();
		graph.setView(graph.pan() + Vec2F(400, 250), graph.scale(), false);
		driver.settle();

		showKanban(driver, scene);
		driver.click(scene.view.views().tabButton(0));
		driver.settle();

		const RectF card = *graph.cardRect(scene.emoji);
		const Vec2F centre = card.pos + card.size / 2.0f;
		CHECK_CLOSE(graph.size().x / 2, centre.x, 1.0f);
		CHECK_CLOSE(graph.size().y / 2, centre.y, 1.0f);
	}

	TEST(TaskSelectedInTreeIsSelectedInKanban) {
		TreeScene scene(false);
		UiDriver driver(scene.window, { 1600, 900 });
		driver.settle();
		TreeView& graph = scene.view.graph();
		graph.openTo(std::nullopt, graph.structure().height());
		driver.settle();

		const RectF card = *graph.cardRect(scene.textEdit);
		driver.click(graph.windowPosition() + card.pos + card.size / 2.0f);
		driver.settle();
		showKanban(driver, scene);

		CHECK(scene.view.kanban().selection() == scene.textEdit);
		CHECK(scene.view.kanban().card(scene.textEdit)->isSelected());
	}

	TEST(DeletedTaskLeavesTheKanban) {
		TreeScene scene(false);
		UiDriver driver(scene.window, { 1400, 900 });
		showKanban(driver, scene);

		scene.board.graph().removeNode(scene.release);
		scene.view.boardReloaded();
		driver.settle();

		CHECK(scene.view.kanban().card(scene.release) == nullptr);
	}
}

SUITE(TaskTimeline) {
	constexpr double kHour = 60 * 60;
	constexpr double kDay = 24 * kHour;

	TEST(SliderCoversOneWindowOfThePrecisionAndItsEndIsLive) {
		TaskTimeline timeline;
		std::vector<std::optional<double>> chosen;
		timeline.setOnMomentChosen([&](std::optional<double> moment) { chosen.push_back(moment); });
		CHECK(!timeline.slider.isEnabled());

		const double last = 10 * kDay;
		timeline.setSpan(last - 3 * kDay, last);
		CHECK(timeline.slider.isEnabled());
		CHECK_EQUAL("1 hour", std::string(timeline.precision().label));
		CHECK_EQUAL(60u, timeline.stepCount());
		CHECK_CLOSE(last - kHour, timeline.windowStart(), 0.001);

		timeline.setPrecision(2);
		CHECK_EQUAL(96u, timeline.stepCount());
		CHECK_CLOSE(last - kDay, timeline.momentAtStep(0).value_or(0), 0.001);
		CHECK_CLOSE(last - kDay / 2, timeline.momentAtStep(48).value_or(0), 0.001);
		CHECK(!timeline.momentAtStep(96).has_value());

		timeline.chooseStep(48);
		timeline.chooseStep(96);
		REQUIRE CHECK_EQUAL(2u, chosen.size());
		CHECK_CLOSE(last - kDay / 2, chosen[0].value_or(0), 0.001);
		CHECK(!chosen[1].has_value());
	}

	TEST(BackAndForwardShiftTheWindowByOnePrecision) {
		TaskTimeline timeline;
		std::optional<double> shown;
		timeline.setOnMomentChosen([&](std::optional<double> moment) {
			shown = moment;
			timeline.showMoment(moment);
		});
		const double last = 10 * kDay;
		timeline.setSpan(last - 5 * kHour - 10, last);
		CHECK(!timeline.forward.isEnabled());
		CHECK(timeline.back.isEnabled());

		timeline.previousWindow();
		CHECK_CLOSE(last - kHour, shown.value_or(0), 0.001);
		CHECK_CLOSE(last - 2 * kHour, timeline.windowStart(), 0.001);
		CHECK_EQUAL(60.0f, timeline.slider.value());

		timeline.chooseStep(30);
		CHECK_CLOSE(last - kHour - kHour / 2, shown.value_or(0), 0.001);
		timeline.previousWindow();
		CHECK_CLOSE(last - 2 * kHour - kHour / 2, shown.value_or(0), 0.001);
		CHECK_CLOSE(last - 3 * kHour, timeline.windowStart(), 0.001);
		CHECK_EQUAL(30.0f, timeline.slider.value());
		CHECK(timeline.moment.text().find("read-only") != std::string::npos);

		timeline.nextWindow();
		CHECK_CLOSE(last - kHour - kHour / 2, shown.value_or(0), 0.001);
		timeline.nextWindow();
		CHECK_CLOSE(last - kHour / 2, shown.value_or(0), 0.001);
		CHECK_CLOSE(last - kHour, timeline.windowStart(), 0.001);
		CHECK(!timeline.forward.isEnabled());

		timeline.previousWindow();
		timeline.chooseStep(60);
		timeline.nextWindow();
		CHECK(!shown.has_value());
	}

	TEST(BackStopsAtTheOldestWindow) {
		TaskTimeline timeline;
		std::optional<double> shown;
		timeline.setOnMomentChosen([&](std::optional<double> moment) {
			shown = moment;
			timeline.showMoment(moment);
		});
		const double last = 10 * kDay;
		timeline.setSpan(last - kHour - 10, last);

		timeline.previousWindow();
		CHECK(!timeline.back.isEnabled());
		timeline.previousWindow();
		CHECK_CLOSE(last - kHour, shown.value_or(0), 0.001);
	}

	TEST(SwitchingBetweenLiveAndThePastKeepsTheSliderGeometry) {
		ApplicationFrame window;
		TaskTimeline timeline;
		window.setContent(&timeline);
		UiDriver driver(window, { 1200, 200 });
		const double last = 10 * kDay;
		timeline.setSpan(last - 5 * kDay, last);
		driver.settle();
		const RectF live = timeline.slider.windowRect();

		timeline.showMoment(last - 2 * kDay - 1234);
		driver.settle();
		CHECK_CLOSE(live.pos.x, timeline.slider.windowRect().pos.x, 0.01f);
		CHECK_CLOSE(live.size.x, timeline.slider.windowRect().size.x, 0.01f);

		timeline.showMoment(std::nullopt);
		timeline.setSpan(std::nullopt, std::nullopt);
		driver.settle();
		CHECK_CLOSE(live.size.x, timeline.slider.windowRect().size.x, 0.01f);
	}

	TEST(OnlyChangesInsideTheWindowBecomeTickMarks) {
		TaskTimeline timeline;
		const double last = 10 * kDay;
		timeline.setSpan(last - 4 * kDay, last);
		timeline.setPrecision(2);
		timeline.setChangeMoments({ last - 2 * kDay, last - kDay, last - kDay / 2, last });

		const std::vector<halnf>& ticks = timeline.slider.tickMarks();
		REQUIRE CHECK_EQUAL(3u, ticks.size());
		CHECK_CLOSE(0.0f, ticks[0], 0.0001f);
		CHECK_CLOSE(0.5f, ticks[1], 0.0001f);
		CHECK_CLOSE(1.0f, ticks[2], 0.0001f);
	}

	TEST(TimelineIsHiddenByDefaultAndToggledFromTheViewMenu) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		driver.settle();
		TaskTimeline& timeline = scene.view.timeline();
		CHECK(!timeline.isVisible());

		auto toggle = [&] {
			for (const auto& item : scene.view.viewMenu.items()) {
				if (item->text() != "Timeline") continue;
				CHECK(item->entry().kind == MenuEntry::Kind::Toggle);
				CHECK_EQUAL(timeline.isVisible(), item->entry().isChecked());
				item->entry().action();
				driver.settle();
				return;
			}
			CHECK(false);
		};

		toggle();
		CHECK(timeline.isVisible());

		std::vector<std::optional<double>> chosen;
		timeline.setOnMomentChosen([&](std::optional<double> moment) { chosen.push_back(moment); });
		timeline.setSpan(1000.0, 1000.0 + 10 * kDay);
		timeline.showMoment(1000.0 + kDay);
		toggle();
		CHECK(!timeline.isVisible());
		REQUIRE CHECK_EQUAL(1u, chosen.size());
		CHECK(!chosen.front().has_value());
	}

	TEST(ReadOnlyBoardIgnoresEditsAndShortcuts) {
		TreeScene scene;
		UiDriver driver(scene.window, { 1400, 900 });
		scene.view.taskWidget().showPage(TaskWidgetPage::Task);
		driver.settle();
		scene.view.openTask(scene.widgets);
		driver.settle();
		scene.view.setReadOnly(true);
		const size_t tasksBefore = scene.board.graph().nodes().size();

		CHECK(!scene.view.page().isEffectivelyEnabled());
		CHECK(!scene.view.description().editor.sourceArea().isEffectivelyEnabled());
		scene.view.createTask();
		driver.key(InputID::N, { .control = true });
		driver.click(scene.view.graph());
		driver.key(InputID::DELETE_KEY);
		driver.settle();
		CHECK(!scene.view.graphDeleteConfirm().isOpen());
		CHECK(!scene.view.header().deleteConfirm.isOpen());
		CHECK_EQUAL(tasksBefore, scene.board.graph().nodes().size());
		CHECK_EQUAL(0, scene.edits);

		scene.view.setReadOnly(false);
		CHECK(scene.view.page().isEffectivelyEnabled());
		scene.view.createTask();
		CHECK_EQUAL(tasksBefore + 1, scene.board.graph().nodes().size());
	}

	struct RecordingSecretsStore : SecretsStore {
		void listSecrets(std::function<void(std::vector<SecretEntry>)> onResult) override {
			listCalls++;
			onResult(entries);
		}
		void setSecret(const std::string& key, const std::string& value, const std::string& description,
									 std::function<void(bool, std::string)> onDone) override {
			stored.push_back({ key, value, description });
			bool replaced = false;
			for (SecretEntry& entry : entries)
				if (entry.key == key) {
					entry.description = description;
					replaced = true;
				}
			if (!replaced) entries.push_back({ key, description });
			onDone(true, {});
		}
		void deleteSecret(const std::string& key, std::function<void(bool, std::string)> onDone) override {
			deleted.push_back(key);
			entries.erase(std::remove_if(entries.begin(), entries.end(),
																	 [&](const SecretEntry& entry) { return entry.key == key; }),
										entries.end());
			onDone(true, {});
		}

		struct Stored {
			std::string key;
			std::string value;
			std::string description;
		};

		int listCalls = 0;
		std::vector<SecretEntry> entries;
		std::vector<Stored> stored;
		std::vector<std::string> deleted;
	};

	struct SkillsScene {
		MemoryFileSystem files;
		ApplicationFrame window;
		SkillsPanel panel{ "/skills", files };

		SkillsScene() {
			files.addTextFile("/skills/elvees/SKILL.md", "---\nname: elvees\ndescription: Elvees access\n---\n\n# Elvees\n");
			files.addTextFile("/skills/generic/SKILL.md", "---\nname: generic\ndescription: Conventions\n---\n\n# Generic\n");
			panel.refresh();
			window.setContent(&panel);
		}
	};

	TEST(SkillsTabEditsSkillFileOnDisk) {
		SkillsScene scene;
		UiDriver driver(scene.window, { 1200, 800 });

		REQUIRE CHECK(scene.panel.browser.isVisible());
		REQUIRE CHECK(scene.panel.browser.row("elvees") != nullptr);

		scene.panel.browser.onFileOpened("/skills/elvees/SKILL.md");
		driver.settle();

		CHECK(shownOnScreen(scene.panel.document));
		CHECK(scene.panel.document.markdown().find("# Elvees") != std::string::npos);

		scene.panel.document.setMode(MarkdownEditor::Mode::Editor);
		driver.settle();
		driver.click(scene.panel.document.sourceArea());
		driver.type("!");
		CHECK_EQUAL(scene.panel.document.markdown(), *scene.files.readText("/skills/elvees/SKILL.md"));
		CHECK(scene.files.readText("/skills/elvees/SKILL.md")->find('!') != std::string::npos);
	}

	TEST(SkillsTabCreatesNewSkillFolder) {
		SkillsScene scene;
		UiDriver driver(scene.window, { 1200, 800 });

		CHECK(scene.panel.createSkill("brand-new"));
		driver.settle();

		CHECK(scene.files.exists("/skills/brand-new/SKILL.md"));
		CHECK(scene.files.readText("/skills/brand-new/SKILL.md")->find("name: brand-new") != std::string::npos);

		CHECK(!scene.panel.createSkill("brand-new"));
		CHECK(!scene.panel.createSkill("../escape"));
	}

	TEST(SecretsTabListsAddsAndDeletesKeysWithoutShowingValues) {
		RecordingSecretsStore store;
		store.entries = { { "elvees.gitlab_token", "Elvees GitLab PAT" } };

		ApplicationFrame window;
		SecretsPanel panel;
		window.setContent(&panel);
		UiDriver driver(window, { 1000, 800 });
		panel.setStore(&store);
		driver.settle();

		CHECK(store.listCalls >= 1);
		CHECK_EQUAL("Add", panel.save.text());

		panel.key.setText("selfhost.ssh_user");
		panel.value.setText("root");
		panel.description.setText("server login");
		driver.click(panel.save);
		driver.settle();
		REQUIRE CHECK_EQUAL(1u, store.stored.size());
		CHECK_EQUAL("selfhost.ssh_user", store.stored[0].key);
		CHECK_EQUAL("root", store.stored[0].value);
		CHECK_EQUAL("", panel.value.text());

		Button* deleteButton = findButton(panel, "Delete");
		REQUIRE CHECK(deleteButton != nullptr);
		driver.click(*deleteButton);
		driver.settle();
		REQUIRE CHECK_EQUAL(1u, store.deleted.size());
		CHECK_EQUAL("elvees.gitlab_token", store.deleted[0]);
	}

	TEST(SecretsTabEditPopulatesFormAndUpdatesEntry) {
		RecordingSecretsStore store;
		store.entries = { { "elvees.gitlab_token", "GitLab PAT" } };

		ApplicationFrame window;
		SecretsPanel panel;
		window.setContent(&panel);
		UiDriver driver(window, { 1000, 800 });
		panel.setStore(&store);
		driver.settle();

		Button* editButton = findButton(panel, "Edit");
		REQUIRE CHECK(editButton != nullptr);
		driver.click(*editButton);
		driver.settle();

		CHECK_EQUAL("elvees.gitlab_token", panel.key.text());
		CHECK_EQUAL("GitLab PAT", panel.description.text());
		CHECK_EQUAL("", panel.value.text());
		CHECK_EQUAL("Update", panel.save.text());

		panel.value.setText("glpat-newvalue");
		driver.click(panel.save);
		driver.settle();

		REQUIRE CHECK_EQUAL(1u, store.stored.size());
		CHECK_EQUAL("elvees.gitlab_token", store.stored[0].key);
		CHECK_EQUAL("glpat-newvalue", store.stored[0].value);
		CHECK_EQUAL("", panel.key.text());
		CHECK_EQUAL("Add", panel.save.text());
	}
}
