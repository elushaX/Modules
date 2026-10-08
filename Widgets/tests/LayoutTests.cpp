#include "BoxLayout.hpp"
#include "Containers.hpp"
#include "Label.hpp"
#include "Probes.hpp"
#include "StackLayout.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <memory>
#include <string>
#include <vector>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	void checkRect(const RectF& expected, const RectF& actual) {
		CHECK_CLOSE(expected.pos.x, actual.pos.x, 0.01f);
		CHECK_CLOSE(expected.pos.y, actual.pos.y, 0.01f);
		CHECK_CLOSE(expected.size.x, actual.size.x, 0.01f);
		CHECK_CLOSE(expected.size.y, actual.size.y, 0.01f);
	}
}

SUITE(BoxLayoutArrangement) {
	TEST(ColumnStacksChildrenWithGapAndPadding) {
		Widget root;
		root.emplaceLayout<ColumnLayout>(5).setPadding(Insets::uniform(10));
		Box first({ 40, 20 });
		Box second({ 60, 30 });
		root.addChild(first);
		root.addChild(second);

		UiDriver driver(root, { 200, 200 });

		checkRect(makeRect(0, 0, 200, 200), root.rect());
		checkRect(makeRect(10, 10, 180, 20), first.rect());
		checkRect(makeRect(10, 35, 180, 30), second.rect());
	}

	TEST(RowLaysOutHorizontallyAndStretchesCrossAxis) {
		Widget root;
		root.emplaceLayout<RowLayout>(4);
		Box first({ 40, 20 });
		Box second({ 60, 30 });
		root.addChild(first);
		root.addChild(second);

		UiDriver driver(root, { 300, 100 });

		checkRect(makeRect(0, 0, 40, 100), first.rect());
		checkRect(makeRect(44, 0, 60, 100), second.rect());
	}

	TEST(CrossAlignCenterUsesPreferredCrossSize) {
		Widget root;
		root.emplaceLayout<RowLayout>().setCrossAlign(CrossAlign::Center);
		Box child({ 40, 20 });
		root.addChild(child);

		UiDriver driver(root, { 300, 100 });

		checkRect(makeRect(0, 40, 40, 20), child.rect());
	}

	TEST(CrossAlignEnd) {
		Widget root;
		root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::End);
		Box child({ 40, 20 });
		root.addChild(child);

		UiDriver driver(root, { 300, 100 });

		checkRect(makeRect(260, 0, 40, 20), child.rect());
	}

	TEST(FillChildrenShareRemainingSpaceByWeight) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		Box fixed({ 100, 10 });
		Box single;
		Box triple;
		fixed.setSizing(AxisSizing::fixed(100), AxisSizing::fit());
		single.setSizing(AxisSizing::fill(1), AxisSizing::fit());
		triple.setSizing(AxisSizing::fill(3), AxisSizing::fit());
		root.addChild(fixed);
		root.addChild(single);
		root.addChild(triple);

		UiDriver driver(root, { 500, 50 });

		CHECK_CLOSE(100, fixed.rect().size.x, 0.01f);
		CHECK_CLOSE(100, single.rect().size.x, 0.01f);
		CHECK_CLOSE(300, triple.rect().size.x, 0.01f);
		CHECK_CLOSE(200, triple.rect().pos.x, 0.01f);
	}

	TEST(FillRespectsMaximumAndRedistributes) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Box capped;
		Box free;
		capped.setSizing(AxisSizing::fit(), AxisSizing::fill().atMost(50));
		free.setSizing(AxisSizing::fit(), AxisSizing::fill());
		root.addChild(capped);
		root.addChild(free);

		UiDriver driver(root, { 100, 400 });

		CHECK_CLOSE(50, capped.rect().size.y, 0.01f);
		CHECK_CLOSE(350, free.rect().size.y, 0.01f);
	}

	TEST(MainAlignCenterAndEnd) {
		Widget root;
		BoxLayout& layout = root.emplaceLayout<RowLayout>();
		Box child({ 100, 10 });
		root.addChild(child);
		UiDriver driver(root, { 400, 50 });

		layout.setMainAlign(MainAlign::Center);
		driver.layout();
		CHECK_CLOSE(150, child.rect().pos.x, 0.01f);

		layout.setMainAlign(MainAlign::End);
		driver.layout();
		CHECK_CLOSE(300, child.rect().pos.x, 0.01f);
	}

	TEST(SpaceBetweenSpreadsChildren) {
		Widget root;
		root.emplaceLayout<RowLayout>().setMainAlign(MainAlign::SpaceBetween);
		Box first({ 50, 10 });
		Box second({ 50, 10 });
		Box third({ 50, 10 });
		root.addChild(first);
		root.addChild(second);
		root.addChild(third);

		UiDriver driver(root, { 350, 50 });

		CHECK_CLOSE(0, first.rect().pos.x, 0.01f);
		CHECK_CLOSE(150, second.rect().pos.x, 0.01f);
		CHECK_CLOSE(300, third.rect().pos.x, 0.01f);
	}

	TEST(HiddenChildrenTakeNoSpace) {
		Widget root;
		root.emplaceLayout<ColumnLayout>(10);
		Box first({ 10, 20 });
		Box hidden;
		Box last({ 10, 20 });
		hidden.setSizing(AxisSizing::fit(), AxisSizing::fixed(100));
		root.addChild(first);
		root.addChild(hidden);
		root.addChild(last);
		hidden.setVisible(false);

		UiDriver driver(root, { 100, 300 });

		CHECK_CLOSE(30, last.rect().pos.y, 0.01f);

		hidden.setVisible(true);
		driver.layout();
		CHECK_CLOSE(140, last.rect().pos.y, 0.01f);
	}

	TEST(OverflowShrinksFlexibleChildrenButNotRigidOnes) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Box rigid;
		Box flexible({ 10, 400 });
		rigid.setSizing(AxisSizing::fit(), AxisSizing::fixed(100));
		root.addChild(rigid);
		root.addChild(flexible);

		UiDriver driver(root, { 100, 300 });

		CHECK_CLOSE(100, rigid.rect().size.y, 0.01f);
		CHECK_CLOSE(200, flexible.rect().size.y, 0.01f);
	}

	TEST(OverflowTakesSpaceFromScrollableContentNotFromControls) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Label header("Header");
		Widget content;
		content.setSizing(AxisSizing::fill(), AxisSizing::fill());
		content.emplaceLayout<ColumnLayout>();
		Box tall({ 10, 2000 });
		content.addChild(tall);
		root.addChild(header);
		root.addChild(content);

		UiDriver driver(root, { 200, 300 });

		CHECK_CLOSE(driver.ui().theme().metrics.lineHeight, header.size().y, 0.01f);
		CHECK_CLOSE(300 - header.size().y, content.size().y, 0.01f);
	}

	TEST(LayoutChangesAnimateWidgetsAndTheirSiblings) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		Box first({ 100, 50 });
		Box second({ 100, 50 });
		root.addChild(first);
		root.addChild(second);
		UiDriver driver(root, { 600, 100 });
		driver.setAutoSettle(false);

		first.setPreferred({ 300, 50 });
		driver.ui().updateLayout();
		driver.advance(32);

		CHECK(first.size().x > 100 && first.size().x < 300);
		CHECK(second.rect().pos.x > 100 && second.rect().pos.x < 300);
		CHECK(driver.ui().needsFrame());

		driver.settle();
		CHECK_CLOSE(300, first.size().x, 0.01f);
		CHECK_CLOSE(300, second.rect().pos.x, 0.01f);
	}

	TEST(ShownWidgetGrowsIntoPlace) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Box header({ 100, 30 });
		Box details({ 100, 80 });
		details.setVisible(false);
		root.addChild(header);
		root.addChild(details);
		UiDriver driver(root, { 200, 300 });
		driver.setAutoSettle(false);

		details.setVisible(true);
		driver.ui().updateLayout();
		CHECK(details.size().y < 1);

		driver.advance(48);
		CHECK(details.size().y > 1 && details.size().y < 80);

		driver.settle();
		CHECK_CLOSE(80, details.size().y, 0.01f);
	}

	TEST(DisablingGeometryAnimationAppliesLayoutImmediately) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		Box first({ 100, 50 });
		first.setGeometryAnimated(false);
		root.addChild(first);
		UiDriver driver(root, { 600, 100 });
		driver.setAutoSettle(false);

		first.setPreferred({ 300, 50 });
		driver.ui().updateLayout();

		CHECK_CLOSE(300, first.size().x, 0.01f);
	}

	TEST(EmptyContainerIsHarmless) {
		Widget root;
		root.emplaceLayout<ColumnLayout>(10);
		UiDriver driver(root, { 100, 100 });
		checkRect(makeRect(0, 0, 100, 100), root.rect());
	}
}

SUITE(FitSizing) {
	TEST(FitContainerWrapsItsChildren) {
		Widget root;
		root.emplaceLayout<FreeLayout>();

		Widget card;
		card.emplaceLayout<ColumnLayout>(4).setPadding(Insets::uniform(8));
		Box a({ 100, 20 });
		Box b({ 60, 30 });
		card.addChild(a);
		card.addChild(b);
		root.addChild(card);

		UiDriver driver(root, { 800, 600 });

		CHECK_CLOSE(116, card.rect().size.x, 0.01f);
		CHECK_CLOSE(8 + 20 + 4 + 30 + 8, card.rect().size.y, 0.01f);
	}

	TEST(FitContainerFollowsChildGrowthInSameFrame) {
		Widget root;
		root.emplaceLayout<FreeLayout>();
		Widget card;
		card.emplaceLayout<ColumnLayout>();
		Box child({ 50, 20 });
		card.addChild(child);
		root.addChild(card);
		UiDriver driver(root);

		child.setPreferred({ 70, 90 });
		driver.layout();

		CHECK_CLOSE(70, card.rect().size.x, 0.01f);
		CHECK_CLOSE(90, card.rect().size.y, 0.01f);
	}

	TEST(NestedFillInsideFitInsideFixedResolvesInOnePass) {
		Widget root;
		root.emplaceLayout<FreeLayout>();

		Widget fixedFrame;
		fixedFrame.setSizing(AxisSizing::fixed(300), AxisSizing::fixed(200));
		fixedFrame.emplaceLayout<RowLayout>();

		Widget fitColumn;
		fitColumn.emplaceLayout<ColumnLayout>();
		Box content({ 80, 40 });
		Box stretch;
		stretch.setSizing(AxisSizing::fill(), AxisSizing::fill());
		fitColumn.addChild(content);
		fitColumn.addChild(stretch);

		Box sibling;
		sibling.setSizing(AxisSizing::fill(), AxisSizing::fill());

		fixedFrame.addChild(fitColumn);
		fixedFrame.addChild(sibling);
		root.addChild(fixedFrame);

		UiDriver driver(root);

		CHECK_CLOSE(80, fitColumn.rect().size.x, 0.01f);
		CHECK_CLOSE(200, fitColumn.rect().size.y, 0.01f);
		CHECK_CLOSE(160, stretch.rect().size.y, 0.01f);
		CHECK_CLOSE(220, sibling.rect().size.x, 0.01f);
	}
}

SUITE(OtherLayouts) {
	TEST(StackPlacesEveryChildOverWholeArea) {
		Widget root;
		root.emplaceLayout<StackLayout>().setPadding(Insets::uniform(5));
		Box bottom;
		Box top;
		root.addChild(bottom);
		root.addChild(top);

		UiDriver driver(root, { 100, 80 });

		checkRect(makeRect(5, 5, 90, 70), bottom.rect());
		checkRect(makeRect(5, 5, 90, 70), top.rect());
	}

	TEST(FreeLayoutUsesRequestedPositionAndPreferredSize) {
		Widget root;
		root.emplaceLayout<FreeLayout>();
		Box child({ 30, 40 });
		child.requestPosition({ 25, 35 });
		root.addChild(child);

		UiDriver driver(root);

		checkRect(makeRect(25, 35, 30, 40), child.rect());

		child.requestPosition({ 100, 10 });
		driver.layout();
		checkRect(makeRect(100, 10, 30, 40), child.rect());
	}

	TEST(GridPlacesChildrenInRowsOfEqualColumns) {
		Widget root;
		root.emplaceLayout<GridLayout>(2, 10);
		Box cells[3] = { Box({ 10, 20 }), Box({ 10, 40 }), Box({ 10, 30 }) };
		for (Box& cell : cells)
			root.addChild(cell);

		UiDriver driver(root, { 210, 300 });

		checkRect(makeRect(0, 0, 100, 40), cells[0].rect());
		checkRect(makeRect(110, 0, 100, 40), cells[1].rect());
		checkRect(makeRect(0, 50, 100, 30), cells[2].rect());
	}
}

SUITE(IncrementalLayout) {
	TEST(SecondPassWithoutChangesArrangesNothing) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Box children[10];
		for (Box& child : children) {
			child.setPreferred({ 10, 10 });
			root.addChild(child);
		}
		UiDriver driver(root);

		driver.ui().resetStats();
		driver.layout();
		CHECK_EQUAL(0u, driver.ui().stats().arrangedWidgets);
		CHECK_EQUAL(0u, driver.ui().stats().layoutPasses);
	}

	TEST(ChangingOneLeafDoesNotRemeasureSiblingsSubtrees) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();

		Widget left;
		left.emplaceLayout<ColumnLayout>();
		Box leftLeaves[20];
		for (Box& leaf : leftLeaves) {
			leaf.setPreferred({ 5, 5 });
			left.addChild(leaf);
		}

		Box changing({ 10, 10 });
		root.addChild(left);
		root.addChild(changing);

		UiDriver driver(root);
		driver.ui().resetStats();

		changing.setPreferred({ 10, 20 });
		driver.layout();

		CHECK(driver.ui().stats().measuredWidgets < 6);
		CHECK_CLOSE(20, changing.rect().size.y, 0.01f);
	}

	TEST(ViewportResizeRelayouts) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		Box fill;
		fill.setSizing(AxisSizing::fill(), AxisSizing::fill());
		root.addChild(fill);
		UiDriver driver(root, { 100, 100 });

		driver.resize({ 640, 480 });
		checkRect(makeRect(0, 0, 640, 480), fill.rect());
	}

	TEST(LayoutChangeIsVisibleImmediatelyAfterDispatch) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		EventProbe probe;
		Box other({ 10, 10 });
		root.addChild(probe);
		root.addChild(other);
		UiDriver driver(root);

		probe.setPreferred({ 50, 100 });
		driver.moveTo({ 5, 105 });

		CHECK_CLOSE(100, other.rect().pos.y, 0.01f);
		CHECK(driver.ui().hoveredWidget() == &other);
	}
}

SUITE(TreeOwnership) {
	TEST(DestroyingChildDetachesItFromParent) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		UiDriver driver(root);
		{
			Box temporary({ 10, 10 });
			root.addChild(temporary);
			driver.layout();
			CHECK_EQUAL(1u, root.children().size());
		}
		CHECK_EQUAL(0u, root.children().size());
		driver.layout();
	}

	TEST(ReparentingMovesChild) {
		Widget first;
		Widget second;
		Box child;
		first.addChild(child);
		second.addChild(child);

		CHECK(child.parent() == &second);
		CHECK(first.children().empty());
	}

	TEST(CannotCreateCycles) {
		Widget parent;
		Widget child;
		parent.addChild(child);
		child.addChild(parent);

		CHECK(parent.parent() == nullptr);
		CHECK(child.children().empty());
	}

	TEST(AttachedSubtreeKnowsItsUi) {
		Widget root;
		Widget middle;
		Box leaf;
		middle.addChild(leaf);
		UiDriver driver(root);

		CHECK(leaf.ui() == nullptr);
		root.addChild(middle);
		CHECK(leaf.ui() == &driver.ui());
		root.removeChild(middle);
		CHECK(leaf.ui() == nullptr);
	}

	TEST(WindowPositionAccumulatesAncestors) {
		Widget root;
		root.emplaceLayout<ColumnLayout>().setPadding(Insets::uniform(10));
		Widget inner;
		inner.emplaceLayout<ColumnLayout>().setPadding(Insets::uniform(7));
		Box leaf({ 5, 5 });
		inner.addChild(leaf);
		root.addChild(inner);
		UiDriver driver(root);

		CHECK_CLOSE(17, leaf.windowPosition().x, 0.01f);
		CHECK_CLOSE(17, leaf.windowPosition().y, 0.01f);
	}
}

SUITE(PaintCulling) {
	TEST(OnlyVisibleRowsOfLongScrolledListArePainted) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		ScrollView scroll;
		scroll.setSizing(AxisSizing::fill(), AxisSizing::fill());
		Widget list;
		list.emplaceLayout<ColumnLayout>(4);
		std::vector<std::unique_ptr<Box>> rows;
		for (int index = 0; index < 3000; index++) {
			rows.push_back(std::make_unique<Box>(Vec2F(200.0f, 70.0f), "row " + std::to_string(index)));
			list.addChild(*rows.back());
		}
		scroll.setContent(&list);
		root.addChild(scroll);
		UiDriver driver(root, { 600, 700 });

		driver.paint();
		const ualni painted = driver.ui().stats().paintedWidgets;
		CHECK(painted < 40);

		scroll.scrollTo(scroll.maximumOffset());
		driver.settle();
		const ualni before = driver.ui().stats().paintedWidgets;
		driver.paint();
		const ualni paintedAtEnd = driver.ui().stats().paintedWidgets - before;
		CHECK(paintedAtEnd < 40);
		CHECK(paintedAtEnd > 5);
		CHECK(overlaps(rows.back()->windowRect(), scroll.windowRect()));
	}

	TEST(OverhangingDecorationsNearTheEdgeStillPaint) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		ScrollView scroll;
		scroll.setSizing(AxisSizing::fill(), AxisSizing::fixed(100));
		Widget list;
		list.emplaceLayout<ColumnLayout>();
		Box first(Vec2F(200.0f, 100.0f), "first");
		Box second(Vec2F(200.0f, 100.0f), "second");
		list.addChild(first);
		list.addChild(second);
		scroll.setContent(&list);
		root.addChild(scroll);
		UiDriver driver(root, { 400, 300 });
		driver.paint();

		const ualni before = driver.ui().stats().paintedWidgets;
		driver.paint();
		CHECK(driver.ui().stats().paintedWidgets - before >= 4);
	}
}

SUITE(FillSizing) {
	TEST(FillGrowsPastContentMaximum) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Widget row;
		row.setSizing(AxisSizing::fill(), AxisSizing::fit());
		RowLayout& rowLayout = row.emplaceLayout<RowLayout>();
		Box fixedA(Vec2F(100.0f, 20.0f), "a");
		fixedA.setSizing(AxisSizing::fixed(100), AxisSizing::fixed(20));
		Box fixedB(Vec2F(100.0f, 20.0f), "b");
		fixedB.setSizing(AxisSizing::fixed(100), AxisSizing::fixed(20));
		row.addChild(fixedA);
		row.addChild(fixedB);
		rowLayout.setMainAlign(MainAlign::SpaceBetween);
		root.addChild(row);
		UiDriver driver(root, { 800, 200 });

		CHECK_CLOSE(800, row.size().x, 0.5f);
		CHECK_CLOSE(700, fixedB.windowRect().pos.x, 0.5f);
	}

	TEST(FillingScrollViewTakesRemainingHeightEvenWhenContentIsShort) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		ScrollView scroll;
		scroll.setSizing(AxisSizing::fill(), AxisSizing::fill());
		Box item(Vec2F(100.0f, 50.0f), "item");
		scroll.setContent(&item);
		Box footer(Vec2F(100.0f, 60.0f), "footer");
		footer.setSizing(AxisSizing::fill(), AxisSizing::fixed(60));
		root.addChild(scroll);
		root.addChild(footer);
		UiDriver driver(root, { 800, 600 });

		CHECK_CLOSE(540, scroll.size().y, 0.5f);
		CHECK_CLOSE(600, footer.windowRect().pos.y + footer.size().y, 0.5f);
	}

	TEST(ExplicitMaximumStillCapsFill) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		Box capped(Vec2F(10.0f, 10.0f), "capped");
		capped.setSizing(AxisSizing::fill().atMost(300), AxisSizing::fixed(10));
		root.addChild(capped);
		UiDriver driver(root, { 800, 100 });
		CHECK_CLOSE(300, capped.size().x, 0.5f);
	}
}

SUITE(SurfaceClipping) {
	TEST(OversizedContentDoesNotPaintOutsideItsPanel) {
		Widget root;
		root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Start);
		Panel panel;
		panel.setSizing(AxisSizing::fixed(200), AxisSizing::fixed(120));
		panel.emplaceLayout<ColumnLayout>();
		Panel big(Panel::Surface::Raised);
		big.setSizing(AxisSizing::fixed(200), AxisSizing::fixed(300));
		panel.addChild(big);
		root.addChild(panel);
		UiDriver driver(root, { 600, 600 });

		CHECK(panel.clipsChildren());
		const RectF panelRect = panel.windowRect();
		const RGBA raised = Theme::standard().palette.raisedSurface;
		const PaintCommand* oversized = nullptr;
		for (const PaintCommand& command : driver.paint().commands()) {
			if (command.kind == PaintCommand::Kind::FillRect && command.color == raised) oversized = &command;
		}
		REQUIRE CHECK(oversized != nullptr);
		CHECK_CLOSE(300, oversized->rect.size.y, 0.5f);
		CHECK(oversized->clip.pos.y + oversized->clip.size.y <= panelRect.pos.y + panelRect.size.y + 0.5f);
	}

	TEST(SurfacelessPanelsLeaveOverhangAlone) {
		Panel plain(Panel::Surface::None);
		CHECK(!plain.clipsChildren());
	}
}
