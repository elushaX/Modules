#include "UnitTest++/UnitTest++.h"

#include "BoxLayout.hpp"
#include "FakeWindowHost.hpp"
#include "Menus.hpp"
#include "UiDriver.hpp"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	struct ChromeScene {
		Widget root;
		MenuBar bar;
		Menu file;
		Widget body;
		FakeWindowHost host;

		ChromeScene() {
			root.emplaceLayout<ColumnLayout>();
			file.addItem("New", []() {});
			bar.addMenu("File", file);
			body.setSizing(AxisSizing::fill(), AxisSizing::fill());
			root.addChild(bar);
			root.addChild(body);
		}

		[[nodiscard]] Vec2F emptyBarPoint() {
			const RectF title = bar.titles().front()->windowRect();
			const RectF controls = bar.windowControls().windowRect();
			const halnf right = bar.windowControls().isVisible() ? controls.pos.x : bar.windowRect().pos.x + bar.size().x;
			return { (title.pos.x + title.size.x + right) / 2, title.center().y };
		}
	};

	struct HostedChrome {
		ChromeScene scene;
		UiDriver driver{ scene.root };

		HostedChrome() {
			driver.ui().setWindowHost(&scene.host);
			driver.layout();
		}
	};

	struct TabletChrome {
		ChromeScene scene;
		UiDriver driver{ scene.root };

		TabletChrome() {
			scene.host.platform.formFactor = FormFactor::Tablet;
			scene.host.platform.hasWindowControls = false;
			scene.host.platform.canMoveWindow = false;
			driver.ui().setWindowHost(&scene.host);
			driver.layout();
		}
	};
}

SUITE(WindowControlButtons) {
	TEST(ControlsStayHiddenWithoutWindowHost) {
		ChromeScene scene;
		UiDriver driver(scene.root);

		CHECK(!scene.bar.windowControls().isVisible());
	}

	TEST(ControlsAppearWhenHostAttachesLate) {
		ChromeScene scene;
		UiDriver driver(scene.root);

		driver.ui().setWindowHost(&scene.host);
		driver.layout();

		CHECK(scene.bar.windowControls().isVisible());
		CHECK(scene.bar.windowControls().size().x > 0);
	}

	TEST(ControlsSitAtTheRightEdgeOfTheBar) {
		HostedChrome hosted;
		ChromeScene& scene = hosted.scene;
		UiDriver& driver = hosted.driver;

		const RectF bar = scene.bar.windowRect();
		const RectF close = scene.bar.windowControls().closeButton().windowRect();
		const RectF title = scene.bar.titles().front()->windowRect();

		CHECK(close.pos.x + close.size.x <= bar.pos.x + bar.size.x + 0.01f);
		CHECK(close.pos.x + close.size.x >= bar.pos.x + bar.size.x - 8);
		CHECK(scene.bar.windowControls().minimizeButton().windowRect().pos.x > title.pos.x + title.size.x);
	}

	TEST(ButtonsDriveTheWindowHost) {
		HostedChrome hosted;
		ChromeScene& scene = hosted.scene;
		UiDriver& driver = hosted.driver;

		driver.click(scene.bar.windowControls().minimizeButton());
		driver.click(scene.bar.windowControls().maximizeButton());
		driver.click(scene.bar.windowControls().closeButton());

		CHECK_EQUAL(1, scene.host.minimizeCount);
		CHECK_EQUAL(1, scene.host.toggleMaximizeCount);
		CHECK_EQUAL(1, scene.host.closeCount);
		CHECK_EQUAL(0, scene.host.moveCount);
	}

	TEST(PlatformWithoutWindowControlsHidesThem) {
		TabletChrome tablet;

		CHECK(!tablet.scene.bar.windowControls().isVisible());
	}

	TEST(MenuTitlesKeepOpeningMenus) {
		HostedChrome hosted;
		ChromeScene& scene = hosted.scene;
		UiDriver& driver = hosted.driver;

		driver.click(*scene.bar.titles().front());

		CHECK(scene.file.isOpen());
		CHECK_EQUAL(0, scene.host.moveCount);
	}
}

SUITE(MenuBarWindowDragging) {
	TEST(PlatformThatCannotMoveWindowsIgnoresBarDrags) {
		TabletChrome tablet;
		const Vec2F start = tablet.scene.emptyBarPoint();

		tablet.driver.moveTo(start);
		tablet.driver.press();
		tablet.driver.moveTo(start + Vec2F(12.0f, 0.0f));
		tablet.driver.release();
		tablet.driver.click(start);
		tablet.driver.click(start);

		CHECK_EQUAL(0, tablet.scene.host.moveCount);
		CHECK_EQUAL(0, tablet.scene.host.toggleMaximizeCount);
	}

	TEST(DraggingEmptyBarStartsWindowMove) {
		HostedChrome hosted;
		ChromeScene& scene = hosted.scene;
		UiDriver& driver = hosted.driver;
		const Vec2F start = scene.emptyBarPoint();

		driver.moveTo(start);
		driver.press();
		CHECK_EQUAL(0, scene.host.moveCount);

		driver.moveTo(start + Vec2F(12.0f, 0.0f));
		CHECK_EQUAL(1, scene.host.moveCount);
		CHECK_EQUAL(0, scene.host.toggleMaximizeCount);
	}

	TEST(ClickWithoutDraggingDoesNotMoveWindow) {
		HostedChrome hosted;
		ChromeScene& scene = hosted.scene;
		UiDriver& driver = hosted.driver;

		driver.click(scene.emptyBarPoint());

		CHECK_EQUAL(0, scene.host.moveCount);
		CHECK_EQUAL(0, scene.host.toggleMaximizeCount);
	}

	TEST(DoubleClickTogglesMaximizeBackAndForth) {
		HostedChrome hosted;
		ChromeScene& scene = hosted.scene;
		UiDriver& driver = hosted.driver;

		for (int round = 0; round < 2; round++) {
			driver.advance(1000);
			driver.click(scene.emptyBarPoint());
			driver.advance(120);
			driver.click(scene.emptyBarPoint());
		}

		CHECK_EQUAL(2, scene.host.toggleMaximizeCount);
		CHECK(!scene.host.maximized);
		CHECK_EQUAL(0, scene.host.moveCount);
	}

	TEST(DoubleClickOnEmptyBarTogglesMaximize) {
		HostedChrome hosted;
		ChromeScene& scene = hosted.scene;
		UiDriver& driver = hosted.driver;

		driver.click(scene.emptyBarPoint());
		driver.advance(120);
		driver.click(scene.emptyBarPoint());

		CHECK_EQUAL(0, scene.host.moveCount);
		CHECK_EQUAL(1, scene.host.toggleMaximizeCount);
		CHECK(scene.host.maximized);
	}

	TEST(SlowSecondClickDoesNotToggleMaximize) {
		HostedChrome hosted;
		ChromeScene& scene = hosted.scene;
		UiDriver& driver = hosted.driver;

		driver.click(scene.emptyBarPoint());
		driver.advance(900);
		driver.click(scene.emptyBarPoint());

		CHECK_EQUAL(0, scene.host.toggleMaximizeCount);
	}

	TEST(SecondaryPressOpensWindowMenuAtPointer) {
		HostedChrome hosted;
		ChromeScene& scene = hosted.scene;
		UiDriver& driver = hosted.driver;
		const Vec2F point = scene.emptyBarPoint();

		driver.moveTo(point);
		driver.press(PointerButton::Secondary);
		driver.release(PointerButton::Secondary);

		CHECK_EQUAL(1, scene.host.windowMenuCount);
		CHECK_CLOSE(point.x, scene.host.windowMenuPoint.x, 0.01f);
		CHECK_EQUAL(0, scene.host.moveCount);
	}

	TEST(PressingBodyDoesNotMoveWindow) {
		HostedChrome hosted;
		ChromeScene& scene = hosted.scene;
		UiDriver& driver = hosted.driver;

		driver.click(scene.body);

		CHECK_EQUAL(0, scene.host.moveCount);
	}

	TEST(BarIgnoresDragWithoutWindowHost) {
		ChromeScene scene;
		UiDriver driver(scene.root);

		driver.click(scene.emptyBarPoint());

		CHECK_EQUAL(0, scene.host.moveCount);
	}
}

SUITE(PointerClickCount) {
	struct Counter : Widget {
		std::vector<int> counts;

		void onEvent(Event& event) override {
			if (event.type() != Event::Type::PointerDown) return;
			counts.push_back(event.clickCount());
			event.accept();
		}
	};

	TEST(QuickNearbyPressesCountUp) {
		Counter counter;
		UiDriver driver(counter);

		driver.click({ 100, 100 });
		driver.advance(100);
		driver.click({ 102, 101 });
		driver.advance(100);
		driver.click({ 101, 100 });

		CHECK_EQUAL(3u, counter.counts.size());
		CHECK_EQUAL(1, counter.counts[0]);
		CHECK_EQUAL(2, counter.counts[1]);
		CHECK_EQUAL(3, counter.counts[2]);
	}

	TEST(DistantOrLatePressesRestartCount) {
		Counter counter;
		UiDriver driver(counter);

		driver.click({ 100, 100 });
		driver.advance(100);
		driver.click({ 140, 100 });
		driver.advance(500);
		driver.click({ 140, 100 });

		CHECK_EQUAL(1, counter.counts[1]);
		CHECK_EQUAL(1, counter.counts[2]);
	}
}

SUITE(WidgetCursors) {
	TEST(HoveredWidgetCursorReachesHost) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Widget field;
		field.setSizing(AxisSizing::fill(), AxisSizing::fixed(40));
		field.setCursor(CursorShape::Text);
		Widget plain;
		plain.setSizing(AxisSizing::fill(), AxisSizing::fill());
		root.addChild(field);
		root.addChild(plain);

		FakeWindowHost host;
		UiDriver driver(root);
		driver.ui().setWindowHost(&host);

		driver.moveTo(field);
		CHECK(!host.cursors.empty() && host.cursors.back() == CursorShape::Text);

		driver.moveTo(plain);
		CHECK(host.cursors.back() == CursorShape::Default);
	}

	TEST(ChildInheritsAncestorCursor) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Widget panel;
		panel.setCursor(CursorShape::Grab);
		panel.setSizing(AxisSizing::fill(), AxisSizing::fill());
		panel.emplaceLayout<ColumnLayout>();
		Widget child;
		child.setSizing(AxisSizing::fill(), AxisSizing::fixed(30));
		panel.addChild(child);
		root.addChild(panel);

		FakeWindowHost host;
		UiDriver driver(root);
		driver.ui().setWindowHost(&host);

		driver.moveTo(child);
		CHECK(host.cursors.back() == CursorShape::Grab);
	}
}

namespace {
	struct ManyMenus {
		Widget root;
		MenuBar bar;
		Menu menus[8];
		Widget body;
		FakeWindowHost host;

		ManyMenus() {
			root.emplaceLayout<ColumnLayout>();
			const char* titles[] = { "File", "Edit", "View", "Canvas", "Layer", "Select", "History", "Help" };
			for (int index = 0; index < 8; index++) {
				menus[index].addItem(std::string(titles[index]) + " item", []() {});
				bar.addMenu(titles[index], menus[index]);
			}
			body.setSizing(AxisSizing::fill(), AxisSizing::fill());
			root.addChild(bar);
			root.addChild(body);
		}
	};

	bool insideWidth(const RectF& rect, halnf width) {
		return rect.pos.x >= -0.01f && rect.pos.x + rect.size.x <= width + 0.01f;
	}
}

SUITE(MenuBarOverflow) {
	TEST(WideWindowShowsEveryTitleAndNoOverflow) {
		ManyMenus scene;
		UiDriver driver(scene.root, { 1600, 600 });
		driver.ui().setWindowHost(&scene.host);
		driver.layout();
		CHECK_EQUAL(8u, scene.bar.shownTitleCount());
		CHECK(!scene.bar.overflowButton().isVisible());
	}

	TEST(NarrowWindowKeepsWindowControlsOnScreen) {
		ManyMenus scene;
		UiDriver driver(scene.root, { 360, 600 });
		driver.ui().setWindowHost(&scene.host);
		driver.layout();
		REQUIRE CHECK(scene.bar.windowControls().isVisible());
		for (WindowButton* button : { &scene.bar.windowControls().minimizeButton(),
																	&scene.bar.windowControls().maximizeButton(),
																	&scene.bar.windowControls().closeButton() })
			CHECK(insideWidth(button->windowRect(), 360));
		CHECK(scene.bar.shownTitleCount() < 8u);
		CHECK(scene.bar.overflowButton().isVisible());
		CHECK(insideWidth(scene.bar.overflowButton().windowRect(), 360));
		for (size_t index = 0; index < scene.bar.shownTitleCount(); index++)
			CHECK(insideWidth(scene.bar.titles()[index]->windowRect(), 360));
	}

	TEST(OverflowMenuOpensTheHiddenMenus) {
		ManyMenus scene;
		UiDriver driver(scene.root, { 360, 600 });
		driver.ui().setWindowHost(&scene.host);
		driver.layout();
		const size_t shown = scene.bar.shownTitleCount();
		REQUIRE CHECK_EQUAL(8u - shown, scene.bar.overflowMenu().items().size());
		CHECK(scene.bar.overflowMenu().items().back()->entry().submenu == &scene.menus[7]);
		driver.click(scene.bar.overflowButton());
		CHECK(scene.bar.overflowButton().isPopupOpen());
	}

	TEST(GrowingTheWindowBringsTheTitlesBack) {
		ManyMenus scene;
		UiDriver driver(scene.root, { 360, 600 });
		driver.ui().setWindowHost(&scene.host);
		driver.layout();
		driver.resize({ 1600, 600 });
		driver.layout();
		CHECK_EQUAL(8u, scene.bar.shownTitleCount());
		CHECK(!scene.bar.overflowButton().isVisible());
		CHECK(scene.bar.titles().back()->isVisible());
	}
}
