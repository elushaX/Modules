#include "BoxLayout.hpp"
#include "Menus.hpp"
#include "TextField.hpp"
#include "StackLayout.hpp"
#include "Probes.hpp"
#include "RecordingPainter.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	MenuItem* findItem(Menu& menu, const std::string& text) {
		for (const auto& item : menu.items()) {
			if (item->entry().text == text) return item.get();
		}
		return nullptr;
	}
}

SUITE(PopupButtonWidget) {
	struct Scene {
		Widget root;
		PopupButton button{ "Options" };
		EventProbe content{ { 160, 90 }, "popup content" };
		Box elsewhere{ { 100, 40 }, "elsewhere" };

		Scene() {
			root.emplaceLayout<ColumnLayout>(100).setCrossAlign(CrossAlign::Start);
			button.setPopupContent(&content);
			root.addChild(button);
			root.addChild(elsewhere);
		}
	};

	TEST(MenuItemsDrawTheirIconInTheIndicatorSlot) {
		Widget root;
		root.emplaceLayout<StackLayout>();
		Menu menu;
		MenuEntry entry = MenuEntry::item("Open", {});
		entry.icon = ImageId{ 7 };
		MenuItem& withIcon = menu.add(std::move(entry));
		menu.addItem("Plain", {});
		root.addChild(menu);
		UiDriver driver(root, { 300, 200 });

		int images = 0;
		for (const PaintCommand& command : driver.paint().commands())
			if (command.kind == PaintCommand::Kind::Image) {
				images++;
				CHECK(command.rect.pos.x < withIcon.windowRect().pos.x + withIcon.windowRect().size.x / 3);
			}
		CHECK_EQUAL(1, images);
	}

	TEST(ClickTogglesPopupBelowButton) {
		Scene scene;
		UiDriver driver(scene.root);

		driver.click(scene.button);
		CHECK(scene.button.isPopupOpen());
		CHECK_CLOSE(scene.button.windowRect().pos.x, scene.content.windowRect().pos.x, 0.01f);
		CHECK(scene.content.windowRect().pos.y >= scene.button.windowRect().pos.y + scene.button.size().y);

		driver.click(scene.button);
		CHECK(!scene.button.isPopupOpen());
	}

	TEST(OutsideClickCloses) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.button);

		driver.click(scene.elsewhere);

		CHECK(!scene.button.isPopupOpen());
	}

	TEST(InteractingWithContentKeepsItOpen) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.button);

		driver.click(scene.content);

		CHECK(scene.button.isPopupOpen());
		CHECK_EQUAL(1, scene.content.count(Event::Type::Click));
	}

	TEST(OpenOnHover) {
		Scene scene;
		scene.button.setOpenOnHover(true);
		UiDriver driver(scene.root);

		driver.moveTo(scene.button);

		CHECK(scene.button.isPopupOpen());
	}

	TEST(PlacementOptionsAreHonored) {
		Scene scene;
		scene.button.popupOptions().placement = Placement::Right;
		UiDriver driver(scene.root);

		driver.click(scene.button);

		CHECK(scene.content.windowRect().pos.x >= scene.button.windowRect().pos.x + scene.button.size().x);
	}
}

SUITE(MenuWidget) {
	struct Scene {
		Widget root;
		PopupButton file{ "File" };
		Menu menu;
		Menu recent;
		int opened = 0;
		int savedAs = 0;
		bool gridVisible = false;

		Scene() {
			root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Start);
			menu.addItem("Open", [this]() { opened++; }, "Ctrl+O");
			menu.addSubmenu("Open Recent", recent);
			menu.addSeparator();
			menu.addToggle("Show Grid", [this]() { return gridVisible; }, [this]() { gridVisible = !gridVisible; });
			recent.addItem("project.sketch2d", [this]() { savedAs++; });
			file.setPopupContent(&menu);
			root.addChild(file);
		}
	};

	TEST(ActivatingItemRunsActionAndClosesMenu) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.file);

		driver.click(*findItem(scene.menu, "Open"));

		CHECK_EQUAL(1, scene.opened);
		CHECK(!scene.menu.isOpen());
		CHECK(!scene.file.isPopupOpen());
	}

	TEST(MenuSizesToWidestItemAndStacksItems) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.file);

		MenuItem& open = *findItem(scene.menu, "Open");
		MenuItem& grid = *findItem(scene.menu, "Show Grid");
		CHECK_CLOSE(open.size().x, grid.size().x, 0.01f);
		CHECK(grid.rect().pos.y > open.rect().pos.y);
		CHECK_CLOSE(driver.ui().theme().metrics.menuItemHeight, open.size().y, 0.01f);
	}

	TEST(PaintsLabelsAndShortcuts) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.file);

		RecordingPainter& painter = driver.paint();

		CHECK(painter.hasText("Open"));
		CHECK(painter.hasText("Ctrl+O"));
		CHECK(painter.hasText("Show Grid"));
	}

	TEST(ToggleItemFlipsState) {
		Scene scene;
		UiDriver driver(scene.root);

		driver.click(scene.file);
		driver.click(*findItem(scene.menu, "Show Grid"));
		CHECK(scene.gridVisible);

		driver.click(scene.file);
		driver.click(*findItem(scene.menu, "Show Grid"));
		CHECK(!scene.gridVisible);
	}

	TEST(HoveringSubmenuOpensItToTheRight) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.file);

		MenuItem& recentItem = *findItem(scene.menu, "Open Recent");
		driver.moveTo(recentItem);

		CHECK(scene.recent.isOpen());
		CHECK(scene.recent.windowRect().pos.x >= scene.menu.windowRect().pos.x + scene.menu.size().x - 1);
		CHECK_EQUAL(2u, driver.ui().openPopupCount());
	}

	TEST(SubmenuOfAMenuOpenedLeftwardsOpensToTheLeftWithoutCoveringIt) {
		Scene scene;
		scene.root.layout()->setPadding(Insets{ 700, 0, 0, 0 });
		scene.file.popupOptions().placement = Placement::Left;
		UiDriver driver(scene.root);
		driver.click(scene.file);
		REQUIRE CHECK(scene.menu.isOpen());

		driver.click(*findItem(scene.menu, "Open Recent"));

		REQUIRE CHECK(scene.recent.isOpen());
		const RectF parent = scene.menu.windowRect();
		const RectF child = scene.recent.windowRect();
		CHECK(child.pos.x + child.size.x <= parent.pos.x + 1);
	}

	TEST(HoveringSiblingClosesSubmenu) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.file);
		driver.moveTo(*findItem(scene.menu, "Open Recent"));

		driver.moveTo(*findItem(scene.menu, "Open"));

		CHECK(!scene.recent.isOpen());
		CHECK(scene.menu.isOpen());
	}

	TEST(ActivatingSubmenuItemClosesWholeChain) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.file);
		driver.moveTo(*findItem(scene.menu, "Open Recent"));

		driver.click(*findItem(scene.recent, "project.sketch2d"));

		CHECK_EQUAL(1, scene.savedAs);
		CHECK_EQUAL(0u, driver.ui().openPopupCount());
	}

	TEST(MovingIntoSubmenuKeepsItOpen) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.file);
		driver.moveTo(*findItem(scene.menu, "Open Recent"));

		driver.moveTo(*findItem(scene.recent, "project.sketch2d"));

		CHECK(scene.recent.isOpen());
		CHECK(scene.menu.isOpen());
	}

	TEST(EscapeClosesSubmenuThenMenu) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.file);
		driver.moveTo(*findItem(scene.menu, "Open Recent"));

		driver.key(InputID::ESCAPE);
		CHECK(!scene.recent.isOpen());
		CHECK(scene.menu.isOpen());

		driver.key(InputID::ESCAPE);
		CHECK(!scene.menu.isOpen());
	}

	TEST(DisabledItemsDoNothing) {
		Widget root;
		root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Start);
		PopupButton button("Edit");
		Menu menu;
		int pasted = 0;
		MenuItem& paste = menu.addItem("Paste", [&pasted]() { pasted++; });
		paste.setEnabled(false);
		button.setPopupContent(&menu);
		root.addChild(button);
		UiDriver driver(root);

		driver.click(button);
		driver.click(paste);

		CHECK_EQUAL(0, pasted);
		CHECK(menu.isOpen());
	}
}

SUITE(TallMenu) {
	TEST(MenuTallerThanTheWindowFitsAndScrollsToEveryItem) {
		Widget root;
		root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Start);
		PopupButton button{ "Long" };
		Menu menu;
		int chosen = -1;
		for (int index = 0; index < 40; index++)
			menu.addItem("Item " + std::to_string(index), [&chosen, index]() { chosen = index; });
		button.setPopupContent(&menu);
		root.addChild(button);
		UiDriver driver(root, { 400, 300 });

		driver.click(button);

		REQUIRE CHECK(menu.isOpen());
		const RectF placed = menu.windowRect();
		CHECK(placed.pos.y >= 0);
		CHECK(placed.pos.y + placed.size.y <= 300.01f);

		MenuItem& last = *findItem(menu, "Item 39");
		driver.moveTo(placed.center());
		for (int step = 0; step < 40 && last.windowRect().pos.y + last.size().y > placed.pos.y + placed.size.y; step++)
			driver.wheel(-1);
		driver.settle();
		CHECK(last.windowRect().pos.y + last.size().y <= placed.pos.y + placed.size.y + 0.5f);

		driver.click(last);
		CHECK_EQUAL(39, chosen);
	}
}

SUITE(MenuIconButtonWidget) {
	TEST(DrawsThreeLinesWithoutTextAndOpensItsMenu) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		MenuIconButton button;
		Menu menu;
		int chosen = 0;
		menu.addItem("Delete", [&]() { chosen++; });
		button.setPopupContent(&menu);
		root.addChild(button);
		UiDriver driver(root, { 400, 300 });

		size_t lines = 0;
		size_t texts = 0;
		for (const PaintCommand& command : driver.paint().commands()) {
			if (command.kind == PaintCommand::Kind::Line) lines++;
			if (command.kind == PaintCommand::Kind::Text) texts++;
		}
		CHECK_EQUAL(3u, lines);
		CHECK_EQUAL(0u, texts);
		CHECK(button.windowRect().size.x >= 16);

		driver.click(button);
		CHECK(button.isPopupOpen());
	}
}

SUITE(MenuBarWidget) {
	struct Scene {
		Widget root;
		MenuBar bar;
		Menu file;
		Menu edit;
		Box body;
		int newClicks = 0;
		int undoClicks = 0;

		Scene() {
			root.emplaceLayout<ColumnLayout>();
			file.addItem("New", [this]() { newClicks++; });
			edit.addItem("Undo", [this]() { undoClicks++; });
			bar.addMenu("File", file);
			bar.addMenu("Edit", edit);
			body.setSizing(AxisSizing::fill(), AxisSizing::fill());
			root.addChild(bar);
			root.addChild(body);
		}

		[[nodiscard]] PopupButton& title(size_t index) const { return *bar.titles()[index]; }
	};

	TEST(CenterWidgetSitsInTheMiddleOfTheBar) {
		Scene scene;
		TextField search;
		search.setSizing(AxisSizing::preferred(300), AxisSizing::fit());
		scene.bar.setCenterWidget(&search);
		UiDriver driver(scene.root, { 1000, 600 });

		const RectF bar = scene.bar.windowRect();
		const RectF field = search.windowRect();
		CHECK_CLOSE(bar.pos.x + bar.size.x / 2, field.pos.x + field.size.x / 2, 1.0f);
		CHECK_CLOSE(300.0f, field.size.x, 1.0f);
		CHECK(field.pos.x > scene.title(1).windowRect().pos.x + scene.title(1).size().x);
		CHECK(field.pos.y >= bar.pos.y + 2.5f);
		CHECK(field.pos.y + field.size.y <= bar.pos.y + bar.size.y - 2.5f);

		driver.click(search);
		CHECK(search.isEditing());

		scene.bar.setCenterWidget(nullptr);
		CHECK(search.parent() == nullptr);
	}

	TEST(TitlesAreLaidOutInARow) {
		Scene scene;
		UiDriver driver(scene.root);

		CHECK(scene.title(1).rect().pos.x >= scene.title(0).rect().pos.x + scene.title(0).size().x);
		CHECK_CLOSE(scene.title(0).rect().pos.y, scene.title(1).rect().pos.y, 0.01f);
		CHECK(scene.title(0).size().y <= scene.bar.size().y);
	}

	TEST(ClickingTitleOpensItsMenu) {
		Scene scene;
		UiDriver driver(scene.root);

		driver.click(scene.title(0));

		CHECK(scene.file.isOpen());
		const RectF title = scene.title(0).windowRect();
		const halnf gap = scene.file.windowRect().pos.y - (title.pos.y + title.size.y);
		CHECK_CLOSE(Theme::standard().metrics.popupGap, gap, 0.01f);
	}

	TEST(HoveringAnotherTitleWhileOpenSwitchesMenus) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.title(0));

		driver.moveTo(scene.title(1));

		CHECK(!scene.file.isOpen());
		CHECK(scene.edit.isOpen());
	}

	TEST(ClickingAnotherTitleWhileOpenKeepsItsMenuOpen) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.title(0));

		driver.click(scene.title(1));

		CHECK(!scene.file.isOpen());
		CHECK(scene.edit.isOpen());
	}

	TEST(ClickingTheOpenTitleAgainClosesItsMenu) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.title(0));
		driver.click(scene.title(1));

		driver.click(scene.title(1));

		CHECK_EQUAL(0u, driver.ui().openPopupCount());
	}

	TEST(ClickingTitleAfterHoverSwitchAndMovingWithinItClosesMenu) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.title(0));
		driver.moveTo(scene.title(1));

		driver.click(driver.centerOf(scene.title(1)) + Vec2F(1.0f, 0.0f));

		CHECK_EQUAL(0u, driver.ui().openPopupCount());
	}

	TEST(HoveringTitlesWhileClosedOpensNothing) {
		Scene scene;
		UiDriver driver(scene.root);

		driver.moveTo(scene.title(0));
		driver.moveTo(scene.title(1));

		CHECK_EQUAL(0u, driver.ui().openPopupCount());
	}

	TEST(ItemActivationClosesMenuAndResetsTitle) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.title(1));

		driver.click(*findItem(scene.edit, "Undo"));

		CHECK_EQUAL(1, scene.undoClicks);
		CHECK(!scene.title(1).isPopupOpen());
	}

	TEST(ClickingBodyClosesMenu) {
		Scene scene;
		UiDriver driver(scene.root);
		driver.click(scene.title(0));

		driver.click(Vec2F(700.0f, 500.0f));

		CHECK_EQUAL(0u, driver.ui().openPopupCount());
	}
}

SUITE(DropdownWidget) {
	struct Scene {
		Widget root;
		Dropdown dropdown{ { "Normal", "Multiply", "Screen" } };
		std::vector<long> selections;

		Scene() {
			root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Start);
			dropdown.setOnSelected([this](long index) { selections.push_back(index); });
			root.addChild(dropdown);
		}
	};

	TEST(ShowsFirstOptionByDefault) {
		Scene scene;
		UiDriver driver(scene.root);

		CHECK_EQUAL(0, scene.dropdown.selectedIndex());
		CHECK(driver.paint().hasText("Normal"));
	}

	TEST(WidthFitsWidestOption) {
		Scene scene;
		UiDriver driver(scene.root);

		CHECK(scene.dropdown.size().x >= MonospaceTextMetrics().textWidth("Multiply", 14));
	}

	TEST(PickingOptionSelectsItAndCloses) {
		Scene scene;
		UiDriver driver(scene.root);

		driver.click(scene.dropdown);
		REQUIRE CHECK(scene.dropdown.isPopupOpen());
		driver.click(*findItem(scene.dropdown.menu(), "Screen"));

		CHECK_EQUAL(2, scene.dropdown.selectedIndex());
		CHECK_EQUAL("Screen", scene.dropdown.selectedText());
		CHECK(!scene.dropdown.isPopupOpen());
		REQUIRE CHECK_EQUAL(1u, scene.selections.size());
		CHECK_EQUAL(2, scene.selections[0]);
	}

	TEST(MenuIsAtLeastAsWideAsDropdown) {
		Scene scene;
		scene.dropdown.setSizing(AxisSizing::fixed(300), AxisSizing::fit());
		UiDriver driver(scene.root);

		driver.click(scene.dropdown);

		CHECK(scene.dropdown.menu().size().x >= 300);
	}

	TEST(ChangingOptionsKeepsValidSelection) {
		Scene scene;
		UiDriver driver(scene.root);
		scene.dropdown.setSelectedIndex(2);

		scene.dropdown.setOptions({ "A" });

		CHECK_EQUAL(0, scene.dropdown.selectedIndex());
		CHECK_EQUAL(1u, scene.dropdown.menu().items().size());
	}

	TEST(SelectingCurrentOptionDoesNotNotify) {
		Scene scene;
		UiDriver driver(scene.root);

		scene.dropdown.setSelectedIndex(0);

		CHECK(scene.selections.empty());
	}
}

SUITE(MenuRefresh) {
	TEST(DynamicTextAndEnabledStateRefreshWhenMenuOpens) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		MenuBar bar;
		Menu edit;
		root.addChild(bar);

		std::string undoName = "Paint";
		bool canUndo = false;
		MenuEntry entry = MenuEntry::item("Undo", {}, "Ctrl+Z");
		entry.dynamicText = [&undoName]() { return "Undo: " + undoName; };
		entry.isEnabled = [&canUndo]() { return canUndo; };
		MenuItem& undo = edit.add(entry);
		bar.addMenu("Edit", edit);
		UiDriver driver(root);

		CHECK_EQUAL("Undo: Paint", undo.text());
		CHECK(!undo.isEnabled());

		undoName = "Create layer";
		canUndo = true;
		driver.click(*bar.titles()[0]);

		CHECK(edit.isOpen());
		CHECK_EQUAL("Undo: Create layer", undo.text());
		CHECK(undo.isEnabled());
		CHECK(driver.paint().hasText("Undo: Create layer"));
	}

	TEST(OnOpenCanRebuildItems) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		MenuBar bar;
		Menu history;
		root.addChild(bar);
		int states = 2;
		history.setOnOpen([&states](Menu& menu) {
			menu.clear();
			for (int state = 0; state < states; state++)
				menu.addItem("State " + std::to_string(state), {});
		});
		bar.addMenu("History", history);
		UiDriver driver(root);

		driver.click(*bar.titles()[0]);
		CHECK_EQUAL(2u, history.items().size());

		driver.key(InputID::ESCAPE);
		states = 5;
		driver.click(*bar.titles()[0]);
		CHECK_EQUAL(5u, history.items().size());
	}
}
