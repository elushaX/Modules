#include "BoxLayout.hpp"
#include "Containers.hpp"
#include "StackLayout.hpp"
#include "Button.hpp"
#include "ReorderableList.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <memory>
#include <string>
#include <vector>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	struct ListScene {
		Widget root;
		ReorderableList list;
		std::vector<std::unique_ptr<Button>> rows;
		std::vector<std::pair<size_t, size_t>> moves;

		explicit ListScene(size_t count = 4) {
			root.emplaceLayout<ColumnLayout>();
			list.setSizing(AxisSizing::fixed(200), AxisSizing::fit());
			for (size_t index = 0; index < count; index++) {
				rows.push_back(std::make_unique<Button>("Row " + std::to_string(index)));
				rows.back()->setSizing(AxisSizing::fill(), AxisSizing::fixed(30));
				list.addItem(*rows.back());
			}
			list.setOnMoved([this](size_t from, size_t to) { moves.push_back({ from, to }); });
			root.addChild(list);
		}

		Vec2F gripCenter(size_t index) { return list.windowPosition() + list.gripRect(index).center(); }

		std::string orderText() const {
			std::string text;
			for (size_t index = 0; index < list.itemCount(); index++)
				text += static_cast<Button&>(list.item(index)).text().back();
			return text;
		}
	};
}

SUITE(ReorderableListDrag) {
	TEST(DraggingGripDownMovesItem) {
		ListScene scene;
		UiDriver driver(scene.root);

		driver.drag(scene.gripCenter(0), scene.gripCenter(2) + Vec2F(0, 8), 6);
		CHECK_EQUAL("1203", scene.orderText());
		REQUIRE CHECK_EQUAL(1u, scene.moves.size());
		CHECK_EQUAL(0u, scene.moves[0].first);
		CHECK_EQUAL(2u, scene.moves[0].second);
	}

	TEST(DraggingItemBodyUpMovesItem) {
		ListScene scene;
		UiDriver driver(scene.root);

		driver.drag(driver.centerOf(*scene.rows[3]), driver.centerOf(*scene.rows[0]) - Vec2F(0, 8), 6);
		CHECK_EQUAL("3012", scene.orderText());
	}

	TEST(OtherItemsMakeRoomWhileDragging) {
		ListScene scene;
		UiDriver driver(scene.root);
		const halnf firstTop = scene.rows[1]->rect().pos.y;
		const halnf secondTop = scene.rows[2]->rect().pos.y;

		driver.moveTo(scene.gripCenter(0));
		driver.press();
		driver.moveTo(scene.gripCenter(1) + Vec2F(0, 10));
		driver.settle();

		CHECK(scene.list.isDragging());
		CHECK_EQUAL(0, scene.list.draggedIndex());
		CHECK_EQUAL(1, scene.list.dropIndex());
		CHECK_CLOSE(scene.rows[0]->targetRect().pos.y, firstTop, 30.0f);
		CHECK_CLOSE(firstTop - 32, scene.rows[1]->rect().pos.y, 0.5f);
		CHECK_CLOSE(secondTop, scene.rows[2]->rect().pos.y, 0.5f);
		CHECK_EQUAL("0123", scene.orderText());

		driver.release();
		CHECK_EQUAL("1023", scene.orderText());
	}

	TEST(DraggedItemFollowsPointerAndPaintsLast) {
		ListScene scene;
		UiDriver driver(scene.root);

		driver.moveTo(scene.gripCenter(0));
		driver.press();
		driver.moveTo(scene.gripCenter(0) + Vec2F(0, 45));
		CHECK_CLOSE(45, scene.rows[0]->rect().pos.y, 1.0f);

		RecordingPainter& painter = driver.paint();
		const PaintCommand* label = painter.findText("Row 0");
		const PaintCommand* other = painter.findText("Row 3");
		REQUIRE CHECK(label != nullptr && other != nullptr);
		CHECK(label > other);
		driver.release();
	}

	TEST(EscapeCancelsDrag) {
		ListScene scene;
		UiDriver driver(scene.root);

		driver.moveTo(scene.gripCenter(0));
		driver.press();
		driver.moveTo(scene.gripCenter(3));
		driver.key(InputID::ESCAPE);
		CHECK(!scene.list.isDragging());
		driver.release();

		CHECK_EQUAL("0123", scene.orderText());
		CHECK(scene.moves.empty());
	}

	TEST(ShortPressStillClicksItem) {
		ListScene scene;
		UiDriver driver(scene.root);
		int clicks = 0;
		scene.rows[1]->setOnActivate([&clicks]() { clicks++; });

		const Vec2F center = driver.centerOf(*scene.rows[1]);
		driver.drag(center, center + Vec2F(2, 2), 2);
		CHECK_EQUAL(1, clicks);
		CHECK(scene.moves.empty());
	}

	TEST(HorizontalDragOnItemDoesNotReorder) {
		ListScene scene;
		UiDriver driver(scene.root);
		const Vec2F center = driver.centerOf(*scene.rows[1]);
		driver.drag(center, center + Vec2F(40, 3), 4);
		CHECK(scene.moves.empty());
	}

	TEST(DropOnSameSlotReportsNothing) {
		ListScene scene;
		UiDriver driver(scene.root);
		driver.drag(scene.gripCenter(1), scene.gripCenter(1) + Vec2F(0, 8), 4);
		CHECK(scene.moves.empty());
		CHECK_EQUAL("0123", scene.orderText());
	}

	TEST(GripShowsGrabCursorOnHover) {
		ListScene scene;
		UiDriver driver(scene.root);
		driver.moveTo(scene.gripCenter(2));
		CHECK(scene.list.cursor() == CursorShape::Grab);
		driver.moveTo(driver.centerOf(*scene.rows[2]));
		CHECK(scene.list.cursor() == CursorShape::Default);
	}
}

SUITE(ReorderableListEnds) {
	TEST(UnevenRowsCanReachBothEnds) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		ReorderableList list;
		list.setSizing(AxisSizing::fixed(200), AxisSizing::fit());
		Button tall("tall");
		Button small("small");
		Button medium("medium");
		tall.setSizing(AxisSizing::fill(), AxisSizing::fixed(60));
		small.setSizing(AxisSizing::fill(), AxisSizing::fixed(20));
		medium.setSizing(AxisSizing::fill(), AxisSizing::fixed(40));
		list.addItem(tall);
		list.addItem(small);
		list.addItem(medium);
		root.addChild(list);
		UiDriver driver(root);

		const Vec2F grip = list.windowPosition() + list.gripRect(0).center();
		driver.drag(grip, grip + Vec2F(0, 300), 6);
		CHECK(&list.item(2) == &tall);

		const Vec2F back = list.windowPosition() + list.gripRect(2).center();
		driver.drag(back, back - Vec2F(0, 300), 6);
		CHECK(&list.item(0) == &tall);
	}
}

#include "Toggles.hpp"

SUITE(ReorderableListClicks) {
	TEST(DraggingACheckboxBodyDoesNotToggleIt) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		ReorderableList list;
		list.setSizing(AxisSizing::fixed(200), AxisSizing::fit());
		Checkbox first{ "First" };
		Checkbox second{ "Second" };
		Checkbox third{ "Third" };
		for (Checkbox* box : { &first, &second, &third }) {
			box->setSizing(AxisSizing::fill(), AxisSizing::fixed(30));
			list.addItem(*box);
		}
		root.addChild(list);
		UiDriver driver(root);

		const Vec2F start = driver.centerOf(first);
		driver.drag(start, start + Vec2F(0, 40), 6);
		CHECK(&list.item(1) == &first);
		CHECK(!first.isChecked());

		const Vec2F back = driver.centerOf(first);
		driver.drag(back, back + Vec2F(0, 12), 4);
		driver.drag(driver.centerOf(first), driver.centerOf(first) - Vec2F(0, 6), 3);
		CHECK(!first.isChecked());

		driver.click(first);
		CHECK(first.isChecked());
	}
}

SUITE(ReorderableListGrip) {
	TEST(GripSitsOnTheRightOnTheFirstLine) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		ReorderableList list;
		list.setSizing(AxisSizing::fixed(200), AxisSizing::fit());
		Button tall{ "Tall" };
		tall.setSizing(AxisSizing::fill(), AxisSizing::fixed(120));
		Button short_{ "Short" };
		short_.setSizing(AxisSizing::fill(), AxisSizing::fixed(20));
		list.addItem(tall);
		list.addItem(short_);
		root.addChild(list);
		UiDriver driver(root);

		const RectF grip = list.gripRect(0);
		const RectF item = tall.rect();
		CHECK_CLOSE(list.size().x, grip.pos.x + grip.size.x, 0.01f);
		CHECK_CLOSE(item.pos.y, grip.pos.y, 0.01f);
		CHECK_CLOSE(driver.ui().theme().metrics.controlHeight, grip.size.y, 0.01f);
		CHECK(item.pos.x + item.size.x <= grip.pos.x + 0.01f);
		CHECK_CLOSE(0, item.pos.x, 0.01f);
		CHECK_CLOSE(20, list.gripRect(1).size.y, 0.01f);
	}

	TEST(PressingBelowTheGripOnATallItemIsNotAGripPress) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		ReorderableList list;
		list.setSizing(AxisSizing::fixed(200), AxisSizing::fit());
		Button tall{ "Tall" };
		tall.setSizing(AxisSizing::fill(), AxisSizing::fixed(120));
		list.addItem(tall);
		root.addChild(list);
		UiDriver driver(root);
		int clicks = 0;
		tall.setOnActivate([&clicks]() { clicks++; });

		driver.click(list.windowPosition() + Vec2F(list.size().x - 30, 100.0f));
		CHECK_EQUAL(1, clicks);
	}
}

SUITE(ReorderableListSwapThresholds) {
	void dragBy(UiDriver & driver, ListScene & scene, size_t index, halnf distance) {
		const Vec2F from = scene.gripCenter(index);
		driver.moveTo(from);
		driver.press();
		driver.moveTo(from + Vec2F(0.0f, distance > 0 ? 6.0f : -6.0f));
		driver.moveTo(from + Vec2F(0.0f, distance));
	}

	TEST(MovingDownSwapsOnlyWhenTheBottomPassesTheNextCenter) {
		ListScene scene;
		UiDriver driver(scene.root);
		const Vec2F start = scene.gripCenter(0);
		dragBy(driver, scene, 0, 15);
		CHECK_EQUAL(0, scene.list.dropIndex());
		driver.moveTo(start + Vec2F(0.0f, 19.0f));
		CHECK_EQUAL(1, scene.list.dropIndex());
		driver.moveTo(start + Vec2F(0.0f, 49.0f));
		CHECK_EQUAL(1, scene.list.dropIndex());
		driver.moveTo(start + Vec2F(0.0f, 51.0f));
		CHECK_EQUAL(2, scene.list.dropIndex());
		driver.release();
		CHECK_EQUAL("1203", scene.orderText());
	}

	TEST(MovingUpSwapsOnlyWhenTheTopPassesThePreviousCenter) {
		ListScene scene;
		UiDriver driver(scene.root);
		const Vec2F start = scene.gripCenter(2);
		dragBy(driver, scene, 2, -15);
		CHECK_EQUAL(2, scene.list.dropIndex());
		driver.moveTo(start - Vec2F(0.0f, 19.0f));
		CHECK_EQUAL(1, scene.list.dropIndex());
		driver.moveTo(start - Vec2F(0.0f, 51.0f));
		CHECK_EQUAL(0, scene.list.dropIndex());
		driver.release();
		CHECK_EQUAL("2013", scene.orderText());
	}

	TEST(TallItemReachesTheTopOnceItsTopPassesTheFirstCenter) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		ReorderableList list;
		list.setSizing(AxisSizing::fixed(200), AxisSizing::fit());
		Button first{ "First" };
		first.setSizing(AxisSizing::fill(), AxisSizing::fixed(30));
		Button tall{ "Tall" };
		tall.setSizing(AxisSizing::fill(), AxisSizing::fixed(200));
		list.addItem(first);
		list.addItem(tall);
		root.addChild(list);
		UiDriver driver(root, { 400, 400 });

		const Vec2F grip = list.windowPosition() + list.gripRect(1).center();
		driver.moveTo(grip);
		driver.press();
		driver.moveTo(grip - Vec2F(0.0f, 6.0f));
		driver.moveTo(grip - Vec2F(0.0f, 16.0f));
		CHECK_EQUAL(1, list.dropIndex());
		driver.moveTo(grip - Vec2F(0.0f, 20.0f));
		CHECK_EQUAL(0, list.dropIndex());
		driver.release();
		CHECK(&list.item(0) == &tall);
	}
}

SUITE(ReorderableListLongPress) {
	Vec2F rowCenter(ListScene & scene, size_t index) { return scene.rows[index]->windowRect().center(); }

	TEST(QuickDragDoesNotReorder) {
		ListScene scene;
		scene.list.setDragMode(ReorderableList::DragMode::LongPress);
		UiDriver driver(scene.root, { 300, 300 });

		driver.drag(rowCenter(scene, 0), rowCenter(scene, 0) + Vec2F(0.0f, 70.0f));

		CHECK_EQUAL("0123", scene.orderText());
		CHECK(!scene.list.isDragging());
	}

	TEST(HoldThenDragReorders) {
		ListScene scene;
		scene.list.setDragMode(ReorderableList::DragMode::LongPress);
		UiDriver driver(scene.root, { 300, 300 });

		const Vec2F start = rowCenter(scene, 0);
		driver.moveTo(start);
		driver.press();
		driver.advance(Ui::kLongPressMs + 20);
		for (int step = 1; step <= 4; step++)
			driver.moveTo(start + Vec2F(0.0f, 13.0f * (halnf) step));
		driver.release();

		CHECK_EQUAL("1203", scene.orderText());
		REQUIRE CHECK_EQUAL(1u, scene.moves.size());
		CHECK_EQUAL(0u, scene.moves[0].first);
		CHECK_EQUAL(2u, scene.moves[0].second);
	}

	TEST(HeldItemInAScrollViewMovesInsteadOfScrolling) {
		Widget root;
		root.emplaceLayout<StackLayout>();
		ScrollView scroll;
		ReorderableList list;
		list.setDragMode(ReorderableList::DragMode::LongPress);
		std::vector<std::unique_ptr<Button>> rows;
		for (int index = 0; index < 12; index++) {
			rows.push_back(std::make_unique<Button>("Row " + std::to_string(index)));
			rows.back()->setSizing(AxisSizing::fill(), AxisSizing::fixed(40));
			list.addItem(*rows.back());
		}
		std::vector<std::pair<size_t, size_t>> moves;
		list.setOnMoved([&moves](size_t from, size_t to) { moves.push_back({ from, to }); });
		scroll.setContent(&list);
		root.addChild(scroll);
		UiDriver driver(root, { 200, 200 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);

		const Vec2F start = rows[0]->windowRect().center();
		driver.moveTo(start);
		driver.press();
		driver.advance(Ui::kLongPressMs + 20);
		for (int step = 1; step <= 5; step++)
			driver.moveTo(start + Vec2F(0.0f, 20.0f * (halnf) step));
		driver.release();

		CHECK_CLOSE(0.0f, scroll.currentOffset(), 0.5f);
		REQUIRE CHECK_EQUAL(1u, moves.size());
		CHECK_EQUAL(0u, moves[0].first);
	}

	TEST(HoldingWithoutMovingReportsTheHeldItem) {
		ListScene scene;
		scene.list.setDragMode(ReorderableList::DragMode::LongPress);
		std::vector<size_t> held;
		scene.list.setOnHeld([&held](size_t index) { held.push_back(index); });
		int clicked = 0;
		scene.rows[1]->setOnActivate([&clicked]() { clicked++; });
		UiDriver driver(scene.root, { 300, 300 });

		driver.moveTo(rowCenter(scene, 1));
		driver.press();
		driver.advance(Ui::kLongPressMs + 20);
		driver.release();

		REQUIRE CHECK_EQUAL(1u, held.size());
		CHECK_EQUAL(1u, held[0]);
		CHECK_EQUAL(0, clicked);
		CHECK(scene.moves.empty());
	}

	TEST(ClearingItemsWhileHeldIsSafe) {
		ListScene scene;
		scene.list.setDragMode(ReorderableList::DragMode::LongPress);
		int held = 0;
		scene.list.setOnHeld([&held](size_t) { held++; });
		UiDriver driver(scene.root, { 300, 300 });

		const Vec2F start = rowCenter(scene, 3);
		driver.moveTo(start);
		driver.press();
		driver.advance(Ui::kLongPressMs + 20);
		scene.list.clearItems();
		driver.moveTo(start + Vec2F(0.0f, 40.0f));
		driver.release();

		CHECK_EQUAL(0, held);
		CHECK(!scene.list.isDragging());
		CHECK(driver.ui().pointerCapture() == nullptr);
	}

	TEST(DisabledDraggingKeepsTheOrder) {
		ListScene scene;
		scene.list.setDragMode(ReorderableList::DragMode::None);
		UiDriver driver(scene.root, { 300, 300 });

		const Vec2F start = rowCenter(scene, 0);
		driver.moveTo(start);
		driver.press();
		driver.advance(Ui::kLongPressMs + 20);
		driver.moveTo(start + Vec2F(0.0f, 70.0f));
		driver.release();

		CHECK(!scene.list.isDragging());
		CHECK_EQUAL("0123", scene.orderText());
		CHECK_CLOSE(scene.list.windowRect().size.x, scene.rows[0]->windowRect().size.x, 0.5f);
	}

	TEST(DroppingOntoAnAcceptingItemReportsItInsteadOfMoving) {
		ListScene scene;
		scene.list.setDragMode(ReorderableList::DragMode::LongPress);
		std::vector<std::pair<size_t, size_t>> drops;
		scene.list.setCanAcceptDrop([](size_t, size_t target) { return target == 2; });
		scene.list.setOnDroppedOnto([&drops](size_t dragged, size_t target) { drops.push_back({ dragged, target }); });
		UiDriver driver(scene.root, { 300, 300 });

		const Vec2F target = rowCenter(scene, 2);
		driver.moveTo(rowCenter(scene, 0));
		driver.press();
		driver.advance(Ui::kLongPressMs + 20);
		driver.moveTo(target);
		CHECK_EQUAL(2, scene.list.dropTarget());
		driver.release();

		REQUIRE CHECK_EQUAL(1u, drops.size());
		CHECK_EQUAL(0u, drops[0].first);
		CHECK_EQUAL(2u, drops[0].second);
		CHECK(scene.moves.empty());
		CHECK_EQUAL("0123", scene.orderText());
	}
}
