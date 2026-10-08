#include "BoxLayout.hpp"
#include "Probes.hpp"
#include "SettingsView.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	struct SettingsScene {
		Widget root;
		Menu main;
		Menu display;
		Menu advanced;
		SettingsView settings;
		int saved = 0;
		bool dark = false;
		int menuClosed = 0;

		SettingsScene() {
			root.emplaceLayout<ColumnLayout>();
			advanced.addItem("Reset", {});
			display.addToggle("Dark mode", [this]() { return dark; }, [this]() { dark = !dark; });
			display.addSubmenu("Advanced", advanced);
			main.addItem("Save", [this]() { saved++; }, "Ctrl+S");
			main.addSeparator();
			main.addSubmenu("Display", display);
			main.setOnItemActivated([this]() { menuClosed++; });
			settings.setRoot("Settings", main);
		}
	};
}

SUITE(SettingsViewNavigation) {
	TEST(OpensFullscreenWithTheRootPage) {
		SettingsScene scene;
		UiDriver driver(scene.root, { 900, 600 });

		scene.settings.open(driver.ui());
		driver.settle();

		CHECK(scene.settings.isOpen());
		CHECK_CLOSE(900.0f, scene.settings.windowRect().size.x, 0.5f);
		CHECK_CLOSE(600.0f, scene.settings.windowRect().size.y, 0.5f);
		CHECK_EQUAL("Settings", scene.settings.currentPage().title());
		CHECK_EQUAL(2u, scene.settings.currentPage().rows().size());
	}

	TEST(TitleSitsOnTheRightOppositeTheBackButton) {
		SettingsScene scene;
		UiDriver driver(scene.root, { 900, 600 });
		scene.settings.open(driver.ui());
		driver.settle();

		SettingsPage& page = scene.settings.currentPage();
		CHECK(page.titleLabel().alignment() == TextAlign::End);
		CHECK(page.titleLabel().windowRect().center().x > page.backButton().windowRect().center().x);
	}

	TEST(TitleAndCloseUseTheRegularFontSize) {
		SettingsScene scene;
		UiDriver driver(scene.root, { 900, 600 });
		Theme large = Theme::standard();
		large.metrics.fontSize = 21;
		driver.ui().setTheme(large);
		scene.settings.open(driver.ui());
		driver.settle();

		SettingsPage& page = scene.settings.currentPage();
		CHECK_CLOSE(21.0f, page.titleLabel().effectiveFontSize(), 0.01f);
		CHECK(page.titleLabel().windowRect().size.y <= page.backButton().windowRect().size.y + 0.5f);
	}

	TEST(SubmenusOpenAsPagesAndBackReturns) {
		SettingsScene scene;
		UiDriver driver(scene.root, { 900, 600 });
		scene.settings.open(driver.ui());

		driver.click(*scene.settings.currentPage().row("Display"));
		CHECK_EQUAL(2u, scene.settings.depth());
		CHECK_EQUAL("Display", scene.settings.currentPage().title());
		driver.click(*scene.settings.currentPage().row("Advanced"));
		CHECK_EQUAL(3u, scene.settings.depth());

		driver.click(scene.settings.currentPage().backButton());
		CHECK_EQUAL("Display", scene.settings.currentPage().title());
		driver.key(InputID::ESCAPE);
		CHECK_EQUAL("Settings", scene.settings.currentPage().title());
		CHECK(scene.settings.isOpen());

		driver.click(scene.settings.currentPage().backButton());
		CHECK(!scene.settings.isOpen());
	}

	TEST(BackToJumpsSeveralPagesAtOnce) {
		SettingsScene scene;
		UiDriver driver(scene.root, { 900, 600 });
		scene.settings.open(driver.ui());
		driver.click(*scene.settings.currentPage().row("Display"));
		driver.click(*scene.settings.currentPage().row("Advanced"));
		REQUIRE CHECK_EQUAL(3u, scene.settings.depth());

		scene.settings.backTo(1);

		CHECK_EQUAL(1u, scene.settings.depth());
		CHECK_EQUAL("Settings", scene.settings.currentPage().title());
		CHECK(scene.settings.isOpen());
	}

	TEST(ActionsRunAndCloseTheView) {
		SettingsScene scene;
		UiDriver driver(scene.root, { 900, 600 });
		scene.settings.open(driver.ui());

		driver.click(*scene.settings.currentPage().row("Save"));

		CHECK_EQUAL(1, scene.saved);
		CHECK_EQUAL(1, scene.menuClosed);
		CHECK(!scene.settings.isOpen());
	}

	TEST(TogglesFlipAndStayOnThePage) {
		SettingsScene scene;
		UiDriver driver(scene.root, { 900, 600 });
		scene.settings.open(driver.ui());
		driver.click(*scene.settings.currentPage().row("Display"));

		driver.click(*scene.settings.currentPage().row("Dark mode"));

		CHECK(scene.dark);
		CHECK(scene.settings.isOpen());
		CHECK_EQUAL("Display", scene.settings.currentPage().title());
	}

	TEST(SeparatorsSplitRowsIntoSections) {
		SettingsScene scene;

		const std::vector<const MenuItem*> order = scene.main.itemsWithSeparators();

		REQUIRE CHECK_EQUAL(3u, order.size());
		CHECK(order[0] != nullptr);
		CHECK(order[1] == nullptr);
		CHECK(order[2] != nullptr);
	}

	TEST(DisabledEntriesCannotBeActivated) {
		SettingsScene scene;
		bool enabled = false;
		int ran = 0;
		scene.main.add([&]() {
			MenuEntry entry = MenuEntry::item("Locked", [&ran]() { ran++; });
			entry.isEnabled = [&enabled]() { return enabled; };
			return entry;
		}());
		UiDriver driver(scene.root, { 900, 600 });
		scene.settings.open(driver.ui());

		driver.click(*scene.settings.currentPage().row("Locked"));

		CHECK_EQUAL(0, ran);
		CHECK(scene.settings.isOpen());
	}

	TEST(CustomContentPagesHaveABackButton) {
		SettingsScene scene;
		Box content;
		content.setPreferred({ 200, 100 });
		UiDriver driver(scene.root, { 900, 600 });
		scene.settings.open(driver.ui());

		scene.settings.openContent("Custom", content);
		driver.settle();
		CHECK_EQUAL("Custom", scene.settings.currentPage().title());
		CHECK(content.ui() != nullptr);

		driver.click(scene.settings.currentPage().backButton());
		CHECK_EQUAL("Settings", scene.settings.currentPage().title());
		CHECK(content.ui() == nullptr);
	}

	TEST(AnEntryCanOpenItsOwnContentPage) {
		SettingsScene scene;
		Box content;
		content.setPreferred({ 200, 100 });
		scene.settings.setOnContentRequested([&](const MenuEntry& entry) {
			if (entry.text != "Save") return false;
			scene.settings.openContent("Save options", content);
			return true;
		});
		UiDriver driver(scene.root, { 900, 600 });
		scene.settings.open(driver.ui());

		driver.click(*scene.settings.currentPage().row("Save"));

		CHECK_EQUAL(0, scene.saved);
		CHECK(scene.settings.isOpen());
		CHECK_EQUAL("Save options", scene.settings.currentPage().title());
	}

	TEST(OpeningSlidesUpFromTheBottom) {
		SettingsScene scene;
		UiDriver driver(scene.root, { 900, 600 });
		driver.setAutoSettle(false);

		scene.settings.open(driver.ui());
		driver.layout();
		driver.advance(16);

		CHECK(scene.settings.windowRect().pos.y > 100);
		driver.settle();
		CHECK_CLOSE(0.0f, scene.settings.windowRect().pos.y, 0.5f);
	}

	TEST(PagesSlideInFromTheSide) {
		SettingsScene scene;
		UiDriver driver(scene.root, { 900, 600 });
		scene.settings.open(driver.ui());
		driver.settle();
		driver.setAutoSettle(false);

		scene.settings.openMenu("Display", scene.display);
		driver.layout();
		driver.advance(16);

		const RectF settled = scene.settings.windowRect();
		CHECK(scene.settings.currentPage().windowRect().pos.x > settled.pos.x + 100);
		driver.settle();
		CHECK_CLOSE(settled.pos.x + 16.0f, scene.settings.currentPage().windowRect().pos.x, 0.5f);
	}

	TEST(LongPagesScroll) {
		SettingsScene scene;
		for (int index = 0; index < 40; index++)
			scene.main.addItem("Item " + std::to_string(index), {});
		UiDriver driver(scene.root, { 900, 400 });
		scene.settings.open(driver.ui());
		driver.settle();

		const RectF view = scene.settings.windowRect();
		CHECK(scene.settings.currentPage().row("Item 39")->windowRect().pos.y > view.pos.y + view.size.y);
	}
}
