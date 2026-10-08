#include "BoxLayout.hpp"
#include "DebugInspector.hpp"
#include "Probes.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

SUITE(DebugInspectorLayout) {
	TEST(HiddenPanelLeavesWholeWindowToUi) {
		Widget root;
		UiDriver driver(root);
		DebugInspector inspector(driver.ui());

		const DebugInspector::WindowSplit split = inspector.splitWindow({ 1500, 900 });

		CHECK(split.content == makeRect(0, 0, 1500, 900));
		CHECK_CLOSE(0, split.panel.size.x, 0.01f);
	}

	TEST(VisiblePanelTakesRightColumnWithoutOverlap) {
		Widget root;
		UiDriver driver(root);
		DebugInspector inspector(driver.ui());
		inspector.setVisible(true);

		const DebugInspector::WindowSplit split = inspector.splitWindow({ 1500, 900 });

		CHECK_CLOSE(0, split.content.pos.x, 0.01f);
		CHECK_CLOSE(split.content.size.x, split.panel.pos.x, 0.01f);
		CHECK_CLOSE(1500, split.content.size.x + split.panel.size.x, 0.01f);
		CHECK_CLOSE(900, split.panel.size.y, 0.01f);
		CHECK(split.panel.size.x >= 320);
	}

	TEST(NarrowWindowKeepsUsablePanelWidth) {
		Widget root;
		UiDriver driver(root);
		DebugInspector inspector(driver.ui());
		inspector.setVisible(true);

		const DebugInspector::WindowSplit split = inspector.splitWindow({ 600, 400 });

		CHECK_CLOSE(320, split.panel.size.x, 0.01f);
		CHECK_CLOSE(280, split.content.size.x, 0.01f);
	}

	TEST(UiLaidOutInContentAreaNeverReachesPanel) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		Box fill;
		fill.setSizing(AxisSizing::fill(), AxisSizing::fill());
		root.addChild(fill);
		UiDriver driver(root, { 1500, 900 });
		DebugInspector inspector(driver.ui());
		inspector.setVisible(true);

		const DebugInspector::WindowSplit split = inspector.splitWindow({ 1500, 900 });
		driver.ui().setViewport(split.content);
		driver.layout();

		const RectF area = fill.windowRect();
		CHECK(area.pos.x + area.size.x <= split.panel.pos.x + 0.01f);
		CHECK(driver.ui().widgetAt(split.panel.center()) == nullptr);
	}
}

SUITE(DebugInspectorSizingEditor) {
	TEST(EveryModeMapsToItsOwnSizing) {
		CHECK(DebugInspector::editedSizing(AxisSizing::Mode::Fit, 0, 0, 0, -1).mode() == AxisSizing::Mode::Fit);
		CHECK(DebugInspector::editedSizing(AxisSizing::Mode::Fixed, 120, 0, 0, -1) == AxisSizing::fixed(120));
		CHECK(DebugInspector::editedSizing(AxisSizing::Mode::Preferred, 80, 0, 0, -1) == AxisSizing::preferred(80));
		CHECK(DebugInspector::editedSizing(AxisSizing::Mode::Fill, 0, 2, 0, -1) == AxisSizing::fill(2));
		CHECK(DebugInspector::editedSizing(AxisSizing::Mode::Fill, 0, 0, 0, -1) == AxisSizing::fill(1));
	}

	TEST(MinimumAndMaximumApplyAndNegativeMaximumMeansNone) {
		const AxisSizing bounded = DebugInspector::editedSizing(AxisSizing::Mode::Fill, 0, 1, 50, 300);
		CHECK_CLOSE(50, bounded.minimum(), 0.01f);
		CHECK_CLOSE(300, bounded.maximum(), 0.01f);
		CHECK(DebugInspector::editedSizing(AxisSizing::Mode::Fill, 0, 1, 0, -1).maximum() >= kUnbounded);
	}

	TEST(EditedSizingReflowsTheWidget) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		Box box(Vec2F(100.0f, 40.0f), "box");
		root.addChild(box);
		UiDriver driver(root, { 800, 200 });
		CHECK_CLOSE(100, box.size().x, 0.5f);

		box.setSizing(Axis::Horizontal, DebugInspector::editedSizing(AxisSizing::Mode::Fill, 0, 1, 0, -1));
		driver.settle();
		CHECK_CLOSE(800, box.size().x, 0.5f);

		box.setSizing(Axis::Horizontal, DebugInspector::editedSizing(AxisSizing::Mode::Fixed, 240, 0, 0, -1));
		driver.settle();
		CHECK_CLOSE(240, box.size().x, 0.5f);
	}
}
