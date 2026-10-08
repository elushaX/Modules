#include "Gallery.hpp"
#include "Theme.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cstdint>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::gallery;
using namespace tp::ui::testing;

namespace {
	MenuItem& itemNamed(Menu& menu, const std::string& text) {
		for (const auto& item : menu.items()) {
			if (item->entry().text == text) return *item;
		}
		throw std::runtime_error("menu item not found: " + text);
	}

	bool isInside(const Widget& inner, const Widget& outer) {
		const RectF a = inner.windowRect();
		const RectF b = outer.windowRect();
		return a.pos.x >= b.pos.x - 0.5f && a.pos.y >= b.pos.y - 0.5f && a.pos.x + a.size.x <= b.pos.x + b.size.x + 0.5f &&
					 a.pos.y + a.size.y <= b.pos.y + b.size.y + 0.5f;
	}
}

SUITE(GalleryApplication) {
	TEST(InitialLayoutFillsWindowAndShowsControlsPage) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });

		CHECK_CLOSE(1000, gallery.size().x, 0.01f);
		CHECK_CLOSE(700, gallery.size().y, 0.01f);
		CHECK(gallery.controls.isVisible());
		CHECK(!gallery.listPage.isVisible());
		CHECK(isInside(gallery.controls.blendMode, gallery.tabs));
		CHECK(gallery.statusBar.windowRect().pos.y > gallery.tabs.windowRect().pos.y);

		RecordingPainter& painter = driver.paint();
		CHECK(painter.isBalanced());
		CHECK(painter.hasText("File"));
		CHECK(painter.hasText("Click me"));
		CHECK(painter.hasText("Volume"));
	}

	TEST(UserSessionOnControlsPage) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		ControlsPage& page = gallery.controls;

		driver.click(page.clickMe);
		driver.click(page.clickMe);
		driver.click(page.disabled);
		CHECK_EQUAL(2, gallery.state.buttonClicks);
		CHECK_EQUAL("clicked 2 times", page.clickCount.text());

		driver.click(page.notifications);
		CHECK(!gallery.state.notificationsEnabled);

		driver.click(page.high);
		CHECK_EQUAL(2, gallery.state.qualityIndex);
		CHECK(!page.medium.isChecked());

		const RectF slider = page.volume.slider().windowRect();
		driver.drag(slider.center(), { slider.pos.x + slider.size.x, slider.center().y });
		CHECK_CLOSE(1.0f, gallery.state.volume, 0.01f);
		driver.settle();
		CHECK_CLOSE(1.0f, page.progress.fraction(), 0.01f);

		driver.click(page.blendMode);
		driver.click(itemNamed(page.blendMode.menu(), "Overlay"));
		CHECK_EQUAL("Overlay", page.blendMode.selectedText());
		CHECK_EQUAL(0u, driver.ui().openPopupCount());

		driver.settle();
		CHECK(!driver.ui().needsFrame());
	}

	TEST(MenusDriveTheWholeApp) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		PopupButton& file = *gallery.menuBar.titles()[0];
		PopupButton& view = *gallery.menuBar.titles()[1];

		driver.click(file);
		driver.moveTo(itemNamed(gallery.fileMenu, "Open Recent"));
		driver.click(itemNamed(gallery.recentMenu, "portrait.sketch2d"));
		CHECK_EQUAL("open portrait.sketch2d", gallery.state.lastAction);
		CHECK_EQUAL("open portrait.sketch2d", gallery.statusBar.text());
		CHECK_EQUAL(0u, driver.ui().openPopupCount());

		driver.click(file);
		driver.moveTo(view);
		CHECK(gallery.viewMenu.isOpen());
		CHECK(!gallery.fileMenu.isOpen());

		driver.click(itemNamed(gallery.viewMenu, "Dark Mode"));
		CHECK(!gallery.state.darkMode);
		CHECK(!gallery.controls.darkMode.isChecked());

		driver.click(view);
		driver.click(itemNamed(gallery.viewMenu, "Canvas"));
		CHECK_EQUAL(2, gallery.tabs.currentIndex());
		CHECK(gallery.canvasPage.isVisible());
	}

	TEST(ScrollingTheListPage) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		driver.click(gallery.tabs.tabButton(1));
		ListPage& page = gallery.listPage;

		CHECK(!page.allRows.isExpanded());
		driver.click(page.allRows.header());
		CHECK(page.allRows.isExpanded());
		CHECK(page.groups.front()->windowRect().pos.y < page.allRows.windowRect().pos.y);
		CHECK(page.scroll.isScrollable());
		Button& lastRow = *page.rows.back();
		CHECK(lastRow.windowRect().pos.y > 700);

		driver.moveTo(page.scroll);
		for (int step = 0; step < 100; step++)
			driver.wheel(-1);
		driver.settle();

		CHECK(isInside(lastRow, page.scroll));
		int clicked = 0;
		lastRow.setOnActivate([&clicked]() { clicked++; });
		driver.click(lastRow);
		CHECK_EQUAL(1, clicked);
	}

	TEST(CanvasPageFloatingPanelAndSplitter) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		driver.click(gallery.tabs.tabButton(2));
		CanvasPage& page = gallery.canvasPage;

		const Vec2F titleGrab = page.colorPanel.windowRect().pos + Vec2F(60.0f, 10.0f);
		driver.drag(titleGrab, titleGrab + Vec2F(80.0f, 40.0f));
		CHECK_CLOSE(104, page.colorPanel.rect().pos.x, 0.5f);
		CHECK_CLOSE(64, page.colorPanel.rect().pos.y, 0.5f);

		const RectF wheel = page.wheel.windowRect();
		const halnf ringRadius = (wheel.size.x < wheel.size.y ? wheel.size.x : wheel.size.y) / 2 - 4;
		driver.click(wheel.center() + Vec2F(0.0f, ringRadius));
		CHECK(!page.colorReadout.text().empty());
		CHECK(gallery.state.brushColor.g > gallery.state.brushColor.r || gallery.state.brushColor.b > 0.1f);

		const halnf inspectorBefore = page.inspector.size().x;
		driver.drag(page.split.handleRect().center() + page.split.windowPosition(), Vec2F(500.0f, 300.0f));
		CHECK(page.inspector.size().x > inspectorBefore);

		driver.settle();
		CHECK(!driver.ui().needsFrame());
	}

	TEST(LongListDoesNotSquashMenuAndTabBars) {
		Gallery gallery;
		UiDriver driver(gallery, { 1508, 415 });
		const halnf menuHeight = gallery.menuBar.size().y;
		const halnf tabHeight = gallery.tabs.tabButton(0).size().y;

		driver.click(gallery.tabs.tabButton(1));
		driver.click(gallery.listPage.allRows.header());

		CHECK_CLOSE(menuHeight, gallery.menuBar.size().y, 0.01f);
		CHECK_CLOSE(tabHeight, gallery.tabs.tabButton(0).size().y, 0.01f);
		CHECK_CLOSE(driver.ui().theme().metrics.lineHeight, gallery.statusBar.size().y, 0.01f);
		CHECK(gallery.listPage.scroll.isScrollable());
	}

	TEST(DockingTabSupportsTheFullPanelWorkflow) {
		Gallery gallery;
		UiDriver driver(gallery, { 1200, 800 });
		driver.click(gallery.tabs.tabButton(3));
		DockPage& page = gallery.dockPage;

		REQUIRE CHECK(page.project.isOpen());
		CHECK(page.terminal.isOpen());
		CHECK_CLOSE(
			page.project.windowRect().pos.x + page.project.size().x + Theme::standard().metrics.surfaceGap,
			page.editor.windowRect().pos.x,
			0.01f
		);

		const Vec2F header = page.project.windowPosition() + Vec2F(20.0f, page.project.headerRect().center().y);
		driver.moveTo(header);
		driver.press();
		driver.moveTo(page.dock.windowPosition() + page.dock.dropTarget(DockSide::Right).center());
		CHECK(page.dock.isPreviewingDrop());
		CHECK(driver.paint().isBalanced());
		driver.release();

		CHECK(page.project.side() == DockSide::Right);
		CHECK(!page.properties.isOpen());

		driver.click(page.project.optionsButton());
		driver.hoverFor(*page.project.optionsMenu().items()[0], 50);
		Menu& modes = *page.project.optionsMenu().items()[0]->entry().submenu;
		driver.click(*modes.items()[1]);
		driver.click(page.editor);
		CHECK(!page.project.isOpen());

		driver.click(*page.dock.stripeButton(page.project));
		CHECK(page.project.isOpen());
		driver.click(page.project.optionsButton());
		driver.hoverFor(*page.project.optionsMenu().items()[0], 50);
		driver.click(*modes.items()[2]);
		CHECK(page.project.viewMode() == DockPanel::ViewMode::Float);

		driver.settle();
		CHECK(!driver.ui().needsFrame());
	}

	TEST(EveryVisibleBoxIsRoundedOnEveryTab) {
		Gallery gallery;
		UiDriver driver(gallery, { 1200, 800 });
		const RGBA window = Theme::standard().palette.window;

		for (size_t tab = 0; tab < gallery.tabs.tabCount(); tab++) {
			driver.click(gallery.tabs.tabButton(tab));
			driver.moveTo(Vec2F(600.0f, 400.0f));
			driver.settle();

			for (const PaintCommand& command : driver.paint().commands()) {
				const bool box = command.kind == PaintCommand::Kind::FillRect || command.kind == PaintCommand::Kind::StrokeRect;
				const bool visible = command.color.a > 0.01f && command.rect.size.x > 2 && command.rect.size.y > 2;
				const bool isWindowBackground =
					std::abs(command.color.r - window.r) < 0.002f && std::abs(command.color.g - window.g) < 0.002f;
				const bool fullWidth = command.rect.size.x >= 1199;
				if (!box || !visible || isWindowBackground || fullWidth) continue;

				CHECK(command.cornerRadius > 0);
			}
		}
	}

	TEST(NoControlStaysHoveredOrPressedAfterRandomPointerJourneys) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		uint32_t seed = 12345;
		const auto random = [&seed](halnf limit) {
			seed = seed * 1664525u + 1013904223u;
			return (halnf) (seed >> 8) / (halnf) (1u << 24) * limit;
		};

		for (size_t tab = 0; tab < gallery.tabs.tabCount(); tab++) {
			driver.click(gallery.tabs.tabButton(tab));
			for (int step = 0; step < 120; step++) {
				driver.moveTo(Vec2F(random(1000), random(700)));
				if (step % 9 == 0) driver.press();
				if (step % 9 == 3) driver.release();
				driver.advance(16);
			}
			driver.release();
			driver.key(InputID::ESCAPE);
			driver.key(InputID::ESCAPE);
			driver.key(InputID::ESCAPE);
			driver.moveTo(Vec2F(-100.0f, -100.0f));
			driver.settle();

			CHECK(driver.ui().hoveredWidget() == nullptr);
			CHECK(driver.ui().pointerCapture() == nullptr);
			CHECK(!driver.ui().needsFrame());

			const std::string tree = driver.dump();
			CHECK(tree.find("hovered") == std::string::npos);
		}
	}

	TEST(TooltipsInTheRealApp) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });

		driver.hoverFor(gallery.controls.clickMe, 1000);

		CHECK(driver.ui().openTooltip() != nullptr);
		CHECK(driver.paint().hasText("Counts how many times it was clicked"));
	}

	TEST(ResizingWindowKeepsEverythingInside) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });

		driver.resize({ 640, 480 });

		CHECK_CLOSE(640, gallery.tabs.size().x, 0.01f);
		CHECK(isInside(gallery.menuBar, gallery));
		CHECK(isInside(gallery.statusBar, gallery));
		CHECK(driver.paint().isBalanced());
	}
}

SUITE(GalleryInputPage) {
	TEST(TypingNameUpdatesGreeting) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		driver.click(gallery.tabs.tabButton(4));

		driver.click(gallery.inputPage.name);
		driver.type("Ada");
		CHECK_EQUAL("Ada", gallery.state.userName);
		CHECK(driver.paint().hasText("Hello, Ada - brush 14 px"));
	}

	TEST(NumberFieldStepsBrushSize) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		driver.click(gallery.tabs.tabButton(4));

		driver.click(gallery.inputPage.size.incrementButton());
		CHECK_CLOSE(15, gallery.state.brushSize, 0.001f);
		CHECK_EQUAL("15 px", gallery.inputPage.size.field().text());
	}

	TEST(TreePageShowsATenLevelTreeThatOpensByHorizontalZoom) {
		Gallery gallery;
		UiDriver driver(gallery, { 1200, 800 });
		driver.click(gallery.tabs.tabButton(10));
		driver.settle();

		TreePage& page = gallery.treePage;
		CHECK(page.model.parents.size() > 100);
		CHECK_EQUAL(TreePage::kLevels - 1, page.view.structure().height());
		CHECK(page.view.openDepth(std::nullopt) < TreePage::kLevels - 1);

		page.view.select(std::nullopt);
		driver.moveTo(page.view.windowRect().center());
		std::vector<size_t> depths = { page.view.openDepth(std::nullopt) };
		for (int notch = 0; notch < 400 && depths.back() < TreePage::kLevels - 1; notch++) {
			driver.horizontalWheel(1);
			const size_t depth = page.view.openDepth(std::nullopt);
			if (depth != depths.back()) depths.push_back(depth);
		}
		driver.settle();
		CHECK_EQUAL(TreePage::kLevels - 1, depths.back());
		for (size_t index = 1; index < depths.size(); index++)
			CHECK_EQUAL(depths[index - 1] + 1, depths[index]);
	}

	TEST(TreePageCardsHoldWorkingControlsWhenDetailed) {
		Gallery gallery;
		UiDriver driver(gallery, { 1200, 800 });
		driver.click(gallery.tabs.tabButton(10));
		driver.settle();

		TreePage& page = gallery.treePage;
		page.view.setView(page.view.pan(), 1, false);
		page.view.select(1);
		driver.settle();
		TreeCard* card = page.view.card(1);
		REQUIRE CHECK(card != nullptr);
		REQUIRE CHECK(card->content().isVisible());
		auto* slider = dynamic_cast<Slider*>(card->content().children().front());
		REQUIRE CHECK(slider != nullptr);
		driver.setSlider(*slider, 80);
		CHECK_CLOSE(80.0f, slider->value(), 2.0f);
		CHECK(driver.paint().hasText("Node 1"));
	}

	TEST(TreePageAddsAChildWhenAChildrenPortIsDroppedOnEmptySpace) {
		Gallery gallery;
		UiDriver driver(gallery, { 1200, 800 });
		driver.click(gallery.tabs.tabButton(10));
		driver.settle();

		TreePage& page = gallery.treePage;
		const size_t before = page.model.parents.size();
		page.view.select(1);
		driver.settle();
		const Vec2F port = page.view.windowPosition() + *page.view.portPosition(1, TreeView::Port::Children);
		driver.drag(port, port + Vec2F(150.0f, 220.0f), 6);
		driver.settle();
		REQUIRE CHECK_EQUAL(before + 1, page.model.parents.size());
		CHECK(page.model.parents.rbegin()->second == TreeItemId(1));
		CHECK(page.view.selection() == page.model.parents.rbegin()->first);
	}

	TEST(ConfirmDialogCountsConfirmations) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		driver.click(gallery.tabs.tabButton(4));

		driver.click(gallery.inputPage.askConfirm);
		CHECK(gallery.inputPage.confirm.isOpen());
		driver.key(InputID::ENTER);
		CHECK_EQUAL(1, gallery.state.confirmations);

		driver.click(gallery.inputPage.openDialog);
		CHECK(gallery.inputPage.dialog.isOpen());
		driver.key(InputID::ESCAPE);
		CHECK(!gallery.inputPage.dialog.isOpen());
	}

	TEST(ShortcutsRunMenuActionsAndMenuShowsThem) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		Modifiers control;
		control.control = true;

		driver.key(InputID::O, control);
		CHECK_EQUAL("open", gallery.state.lastAction);
		CHECK_EQUAL("Ctrl+O", itemNamed(gallery.fileMenu, "Open...").entry().shortcut);

		driver.holdKey(InputID::TAB, 2, control);
		CHECK_EQUAL(3, gallery.tabs.currentIndex());
	}

	TEST(DynamicMenuEntryShowsLastAction) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		driver.click(*gallery.menuBar.titles()[1]);
		CHECK_EQUAL("No action yet", gallery.viewMenu.items()[1]->text());
		driver.key(InputID::ESCAPE);

		Modifiers control;
		control.control = true;
		driver.key(InputID::N, control);
		driver.click(*gallery.menuBar.titles()[1]);
		CHECK_EQUAL("Last action: new", gallery.viewMenu.items()[1]->text());
	}
}

SUITE(GalleryNotice) {
	TEST(NoticeButtonShowsTimedBalloon) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		driver.click(gallery.tabs.tabButton(4));

		driver.click(gallery.inputPage.showNotice);
		CHECK(gallery.inputPage.notice.isShown());
		CHECK_EQUAL("Saved the reference app's settings", gallery.inputPage.notice.text());

		driver.advance(3000);
		CHECK(!gallery.inputPage.notice.isShown());
	}
}

SUITE(GalleryReorderPage) {
	TEST(DraggingTaskReordersAndReportsMove) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		driver.click(gallery.tabs.tabButton(5));
		ReorderableList& list = gallery.reorderPage.list;
		CHECK_EQUAL("Nothing moved yet", gallery.reorderPage.status.text());

		const Vec2F from = list.windowPosition() + list.gripRect(0).center();
		const Vec2F to = list.windowPosition() + list.gripRect(2).center() + Vec2F(0, 8);
		driver.drag(from, to, 6);

		CHECK_EQUAL("Line art", static_cast<Checkbox&>(list.item(0)).text());
		CHECK_EQUAL("Sketch", static_cast<Checkbox&>(list.item(2)).text());
		CHECK_EQUAL("Moved \"Sketch\" from 1 to 3", gallery.reorderPage.status.text());
	}

	TEST(ClickingTaskStillTogglesIt) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		driver.click(gallery.tabs.tabButton(5));
		Checkbox& first = *gallery.reorderPage.tasks[0];

		driver.click(first);
		CHECK(first.isChecked());
	}
}

SUITE(GalleryNestedCollapsibles) {
	TEST(NestedGroupsOpenInsideTheScrollableList) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 600 });
		driver.click(gallery.tabs.tabButton(1));
		ListPage& page = gallery.listPage;
		REQUIRE CHECK_EQUAL(3u, page.groups.size());
		REQUIRE CHECK_EQUAL(9u, page.subgroups.size());

		Collapsible& brushes = *page.groups[0];
		Collapsible& favorites = *page.subgroups[1];
		const halnf firstRowBefore = page.allRows.windowRect().pos.y;
		CHECK(!brushes.isExpanded());
		CHECK(!favorites.isExpanded());

		driver.click(brushes.header());
		CHECK(brushes.isBodyShown());
		CHECK(!favorites.isBodyShown());
		const halnf afterGroup = page.allRows.windowRect().pos.y;
		CHECK(afterGroup > firstRowBefore + 60);

		driver.click(favorites.header());
		CHECK(favorites.isExpanded());
		CHECK(page.allRows.windowRect().pos.y > afterGroup + 100);

		Button& nested = *page.groupRows[5];
		CHECK_EQUAL("Brushes / Favorites 1", nested.text());
		CHECK(nested.windowRect().pos.x > favorites.header().windowRect().pos.x);
		CHECK_CLOSE(
			favorites.body().windowRect().pos.x + favorites.body().windowRect().size.x - ReorderableList::kGripWidth,
			nested.windowRect().pos.x + nested.windowRect().size.x,
			0.5f
		);

		driver.click(brushes.header());
		CHECK(!brushes.isBodyShown());
		CHECK(favorites.isExpanded());
		CHECK_CLOSE(firstRowBefore, page.allRows.windowRect().pos.y, 0.5f);
	}

	TEST(ScrollingReachesRowsInsideAnOpenGroup) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 400 });
		driver.click(gallery.tabs.tabButton(1));
		ListPage& page = gallery.listPage;
		Collapsible& documents = *page.groups[2];
		driver.click(documents.header());

		Collapsible& archived = *page.subgroups[8];
		driver.moveTo(page.scroll);
		for (int step = 0; step < 100 && !isInside(archived.header(), page.scroll); step++) {
			driver.wheel(-1);
			driver.settle();
		}
		REQUIRE CHECK(isInside(archived.header(), page.scroll));
		driver.click(archived.header());
		CHECK(archived.isExpanded());

		Button& last = *page.groupRows.back();
		for (int step = 0; step < 100 && !isInside(last, page.scroll); step++) {
			driver.wheel(-1);
			driver.settle();
		}
		CHECK(isInside(last, page.scroll));
		int clicked = 0;
		last.setOnActivate([&clicked]() { clicked++; });
		driver.click(last);
		CHECK_EQUAL(1, clicked);
	}
}

SUITE(GalleryReorderableGroups) {
	TEST(RootGroupsCanBeReordered) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		driver.click(gallery.tabs.tabButton(1));
		ListPage& page = gallery.listPage;
		ReorderableList& list = page.list;
		REQUIRE CHECK_EQUAL(6u, list.itemCount());
		CHECK(&list.item(0) == page.groups[0].get());
		CHECK(&list.item(3) == &page.allRows);

		const Vec2F from = list.windowPosition() + list.gripRect(0).center();
		const RectF third = list.item(2).windowRect();
		driver.drag(from, { from.x, third.pos.y + third.size.y - 2 }, 10);

		CHECK(&list.item(0) == page.groups[1].get());
		CHECK(&list.item(2) == page.groups[0].get());
		CHECK(!page.groups[0]->isExpanded());
	}

	TEST(DraggingAGroupHeaderDoesNotToggleIt) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 700 });
		driver.click(gallery.tabs.tabButton(1));
		ListPage& page = gallery.listPage;
		Collapsible& layers = *page.groups[1];

		const Vec2F start = driver.centerOf(layers.header());
		driver.drag(start, start - Vec2F(0, 40), 8);
		CHECK(&page.list.item(0) == &layers);
		CHECK(!layers.isExpanded());

		driver.click(layers.header());
		CHECK(layers.isExpanded());
	}

	TEST(OpenGroupMovesWithItsContent) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 900 });
		driver.click(gallery.tabs.tabButton(1));
		ListPage& page = gallery.listPage;
		Collapsible& documents = *page.groups[2];
		driver.click(documents.header());
		driver.click(*page.subgroups[6]);
		REQUIRE CHECK(documents.isExpanded());

		const Vec2F from = page.list.windowPosition() + page.list.gripRect(2).center();
		const Vec2F to = page.list.windowPosition() + page.list.gripRect(0).center() - Vec2F(0, 10);
		driver.drag(from, to, 10);
		CHECK(&page.list.item(0) == &documents);
		CHECK(documents.isExpanded());
		CHECK(page.groupRows[30]->windowRect().pos.y > documents.header().windowRect().pos.y);
	}
}

SUITE(GalleryListShowcases) {
	TEST(DraggableListReordersAndReportsTheMove) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 900 });
		driver.click(gallery.tabs.tabButton(1));
		ListPage& page = gallery.listPage;
		driver.click(page.draggableSection.header());
		REQUIRE CHECK(page.draggableSection.isExpanded());

		Checkbox& sketch = *page.draggableItems[0];
		const Vec2F from = driver.centerOf(sketch);
		const RectF third = page.draggableItems[2]->windowRect();
		driver.drag(from, { from.x, third.pos.y + third.size.y - 2 }, 10);

		CHECK(&page.draggable.item(2) == &sketch);
		CHECK(!sketch.isChecked());
		CHECK_EQUAL("Moved \"Sketch\" from 1 to 3", page.draggableStatus.text());
	}

	TEST(StaticListKeepsItsOrderAndStillToggles) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 900 });
		driver.click(gallery.tabs.tabButton(1));
		ListPage& page = gallery.listPage;
		driver.click(page.staticSection.header());

		Checkbox& sketch = *page.staticItems[0];
		const Vec2F from = driver.centerOf(sketch);
		driver.drag(from, from + Vec2F(0, 60), 10);
		CHECK(page.staticList.children()[0] == &sketch);
		CHECK(sketch.windowRect().pos.y < page.staticItems[1]->windowRect().pos.y);

		driver.click(*page.staticItems[1]);
		CHECK(page.staticItems[1]->isChecked());
	}
}

SUITE(GalleryNestedReorder) {
	TEST(FavoritesInsideAGroupAreReorderable) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 900 });
		driver.click(gallery.tabs.tabButton(1));
		ListPage& page = gallery.listPage;
		REQUIRE CHECK_EQUAL(3u, page.favoriteLists.size());

		Collapsible& layers = *page.groups[1];
		Collapsible& favorites = *page.subgroups[4];
		CHECK_EQUAL("Favorites", favorites.header().title());
		driver.click(layers.header());
		driver.click(favorites.header());

		ReorderableList& items = *page.favoriteLists[1];
		REQUIRE CHECK_EQUAL(5u, items.itemCount());
		Button& first = static_cast<Button&>(items.item(0));
		CHECK_EQUAL("Layers / Favorites 1", first.text());

		const Vec2F start = items.windowPosition() + items.gripRect(0).center();
		const RectF third = items.item(2).windowRect();
		driver.drag(start, { start.x, third.pos.y + third.size.y / 2 + 4 }, 10);

		CHECK(&items.item(2) == &first);
		CHECK(&page.list.item(1) == &layers);
		CHECK(favorites.isExpanded());
		CHECK(layers.isExpanded());
	}

	TEST(OtherSubgroupsStayPlain) {
		Gallery gallery;
		UiDriver driver(gallery, { 1000, 900 });
		driver.click(gallery.tabs.tabButton(1));
		ListPage& page = gallery.listPage;
		Collapsible& recent = *page.subgroups[0];
		CHECK(page.groupRows[0]->parent() == &recent.body());
		CHECK(dynamic_cast<ReorderableList*>(page.groupRows[5]->parent()) != nullptr);
	}
}

SUITE(GalleryFilesPage) {
	TEST(FilesTabBrowsesHomeAndOpensTheSaveDialog) {
		Gallery gallery;
		UiDriver driver(gallery, { 1200, 800 });
		size_t filesTab = gallery.tabs.tabCount();
		for (size_t tab = 0; tab < gallery.tabs.tabCount(); tab++) {
			if (gallery.tabs.tabButton(tab).text() == "Files") filesTab = tab;
		}
		REQUIRE CHECK(filesTab < gallery.tabs.tabCount());
		driver.click(gallery.tabs.tabButton(filesTab));
		driver.settle();

		FilesPage& page = gallery.filesPage;
		CHECK_EQUAL(FileSystem::local().home(), page.browser.directory());
		CHECK(isInside(page.browser, gallery.tabs));
		CHECK(page.browser.windowRect().size.y > 200);

		driver.click(page.saveFile);
		driver.settle();
		REQUIRE CHECK(page.dialog.isOpen());
		CHECK_EQUAL("Save as", page.dialog.title());
		CHECK_EQUAL("untitled.txt", page.dialog.name.text());

		driver.key(InputID::ESCAPE);
		page.dialog.close();
		driver.click(page.chooseFolder);
		driver.settle();
		REQUIRE CHECK(page.dialog.isOpen());
		page.dialog.confirm();
		CHECK_EQUAL("Chose " + page.browser.directory(), page.status.text());
	}
}
