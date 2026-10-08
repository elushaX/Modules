#include "KanbanBoard.hpp"
#include "Label.hpp"
#include "TextField.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class ListModel : public KanbanModel {
	public:
		std::vector<std::vector<KanbanItemId>> columns;
		std::vector<KanbanMove> refused;

		[[nodiscard]] std::vector<KanbanItemId> items(size_t column) const override {
			return column < columns.size() ? columns[column] : std::vector<KanbanItemId>{};
		}

		[[nodiscard]] bool canMove(const KanbanMove& move) const override {
			return std::find(refused.begin(), refused.end(), move) == refused.end() && move.item != kLockedItem;
		}

		void apply(const KanbanMove& move) {
			auto& from = columns[move.fromColumn];
			from.erase(std::find(from.begin(), from.end(), move.item));
			auto& to = columns[move.toColumn];
			to.insert(to.begin() + std::ptrdiff_t(std::min(move.toIndex, to.size())), move.item);
		}

		static constexpr KanbanItemId kLockedItem = 99;
	};

	struct BoardScene {
		ListModel model;
		KanbanBoard board{ model };
		std::vector<KanbanMove> moves;
		std::vector<KanbanItemId> selected;
		std::vector<KanbanItemId> activated;
		std::vector<std::pair<size_t, size_t>> columnMoves;

		explicit BoardScene(std::vector<KanbanColumn> columns = defaultColumns()) {
			model.columns = { { 1, 2, 3 }, { 4 }, {} };
			model.columns.resize(columns.size());
			board.setCardUpdater([](KanbanCard& card) { card.setTitle("Card " + std::to_string(card.id())); });
			board.setOnMoved([this](const KanbanMove& move) {
				moves.push_back(move);
				model.apply(move);
			});
			board.setOnSelected([this](KanbanItemId id) { selected.push_back(id); });
			board.setOnActivated([this](KanbanItemId id) { activated.push_back(id); });
			board.setOnColumnMoved([this](size_t from, size_t to) {
				columnMoves.emplace_back(from, to);
				std::vector<KanbanItemId> items = std::move(model.columns[from]);
				model.columns.erase(model.columns.begin() + std::ptrdiff_t(from));
				model.columns.insert(model.columns.begin() + std::ptrdiff_t(to), std::move(items));
			});
			board.setColumns(std::move(columns));
		}

		static std::vector<KanbanColumn> defaultColumns() { return { { "To do" }, { "Doing" }, { "Done" } }; }

		KanbanCard& card(KanbanItemId id) { return *board.card(id); }

		Vec2F centerOfColumn(size_t column) { return board.columnView(column).windowRect().center(); }

		Vec2F headerOf(size_t column) { return board.columnView(column).title().windowRect().center(); }

		Vec2F leftEdgeOf(size_t column) {
			const RectF rect = board.columnView(column).windowRect();
			return { rect.pos.x + rect.size.x * 0.25f, rect.center().y };
		}

		Vec2F rightEdgeOf(size_t column) {
			const RectF rect = board.columnView(column).windowRect();
			return { rect.pos.x + rect.size.x * 0.75f, rect.center().y };
		}

		std::vector<std::string> columnTitles() {
			std::vector<std::string> titles;
			for (size_t column = 0; column < board.columnCount(); column++)
				titles.push_back(board.columnView(column).title().text());
			return titles;
		}

		std::vector<std::string> menuTitles(size_t column) {
			std::vector<std::string> titles;
			for (const std::unique_ptr<MenuItem>& item : board.columnView(column).optionsMenu().items())
				titles.push_back(item->text());
			return titles;
		}

		Vec2F belowCard(KanbanItemId id) {
			const RectF rect = card(id).windowRect();
			return { rect.center().x, rect.pos.y + rect.size.y * 0.8f };
		}

		Vec2F aboveCard(KanbanItemId id) {
			const RectF rect = card(id).windowRect();
			return { rect.center().x, rect.pos.y + rect.size.y * 0.2f };
		}

		std::vector<KanbanItemId> shownIn(size_t column) {
			std::vector<KanbanItemId> ids;
			for (const KanbanCard* shown : board.columnView(column).stack().cards())
				ids.push_back(shown->id());
			return ids;
		}
	};

	using Ids = std::vector<KanbanItemId>;
	using Titles = std::vector<std::string>;
	using ColumnMove = std::pair<size_t, size_t>;
}

SUITE(KanbanBoard) {
	TEST(ColumnsShowModelItemsInOrder) {
		BoardScene scene;
		UiDriver driver(scene.board);

		REQUIRE CHECK_EQUAL(3u, scene.board.columnCount());
		CHECK(scene.shownIn(0) == Ids({ 1, 2, 3 }));
		CHECK(scene.shownIn(1) == Ids({ 4 }));
		CHECK(scene.shownIn(2).empty());
		CHECK_EQUAL("Card 2", scene.card(2).title());
		CHECK(scene.card(1).windowRect().pos.y < scene.card(2).windowRect().pos.y);
		CHECK(scene.board.columnView(0).windowRect().pos.x < scene.board.columnView(1).windowRect().pos.x);
	}

	TEST(ColumnHeaderShowsTitleCountAndLimit) {
		std::vector<KanbanColumn> columns = BoardScene::defaultColumns();
		columns[0].cardLimit = 2;
		BoardScene scene(columns);
		UiDriver driver(scene.board);

		CHECK_EQUAL("To do", scene.board.columnView(0).title().text());
		CHECK_EQUAL("3 / 2", scene.board.columnView(0).count().text());
		CHECK_EQUAL("1", scene.board.columnView(1).count().text());
	}

	TEST(DraggingCardToAnotherColumnReportsMove) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.drag(driver.centerOf(scene.card(2)), scene.centerOfColumn(2), 8);

		REQUIRE CHECK_EQUAL(1u, scene.moves.size());
		CHECK(scene.moves[0] == (KanbanMove{ 2, 0, 1, 2, 0 }));
		CHECK(scene.shownIn(0) == Ids({ 1, 3 }));
		CHECK(scene.shownIn(2) == Ids({ 2 }));
		CHECK_EQUAL(2u, *scene.board.columnOf(2));
		CHECK(!scene.board.isDragging());
	}

	TEST(DroppingBelowACardInsertsAfterIt) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.drag(driver.centerOf(scene.card(1)), scene.belowCard(4), 8);

		REQUIRE CHECK_EQUAL(1u, scene.moves.size());
		CHECK_EQUAL(1u, scene.moves[0].toColumn);
		CHECK_EQUAL(1u, scene.moves[0].toIndex);
		CHECK(scene.shownIn(1) == Ids({ 4, 1 }));
	}

	TEST(DraggingWithinAColumnReorders) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.drag(driver.centerOf(scene.card(3)), scene.aboveCard(1), 8);

		REQUIRE CHECK_EQUAL(1u, scene.moves.size());
		CHECK(scene.moves[0] == (KanbanMove{ 3, 0, 2, 0, 0 }));
		CHECK(scene.shownIn(0) == Ids({ 3, 1, 2 }));
	}

	TEST(PlaceholderOpensWhereTheCardWillLand) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.moveTo(driver.centerOf(scene.card(1)));
		driver.press();
		driver.moveTo(scene.belowCard(4));

		REQUIRE CHECK(scene.board.isDragging());
		CHECK_EQUAL(1u, *scene.board.draggedItem());
		REQUIRE CHECK(scene.board.dropTarget().has_value());
		CHECK_EQUAL(1u, scene.board.dropTarget()->column);
		CHECK(scene.board.dropTarget()->accepted);
		CHECK_EQUAL(1u, *scene.board.columnView(1).stack().placeholderIndex());
		CHECK(!scene.board.columnView(0).stack().placeholderIndex().has_value());
		driver.release();
	}

	TEST(SmallPointerMovementIsAClickNotADrag) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.drag(driver.centerOf(scene.card(2)), driver.centerOf(scene.card(2)) + Vec2F(2, 1), 2);

		CHECK(scene.moves.empty());
		CHECK(scene.selected == Ids({ 2 }));
		CHECK_EQUAL(2u, *scene.board.selection());
		CHECK(scene.card(2).isSelected());
	}

	TEST(DoubleClickActivatesCard) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.click(scene.card(4));
		driver.click(scene.card(4));

		CHECK(scene.activated == Ids({ 4 }));
	}

	TEST(DoubleClickGrowsTheCardIntoTheFullView) {
		BoardScene scene;
		UiDriver driver(scene.board, { 1000, 700 });
		driver.settle();
		Label detailed("Most detailed");
		scene.board.fullViewFrame().setContent(&detailed);
		std::vector<std::optional<KanbanItemId>> changes;
		scene.board.setOnFullViewChanged([&](std::optional<KanbanItemId> item) { changes.push_back(item); });
		const RectF cardRect = scene.card(4).windowRect();

		driver.click(scene.card(4));
		driver.click(scene.card(4));
		REQUIRE CHECK(scene.board.fullViewItem() == std::optional<KanbanItemId>(4));
		driver.advance(16);
		const RectF growing = scene.board.fullViewFrame().windowRect();
		CHECK(growing.size.x < scene.board.size().x - 2 * FullViewFrame::kMargin);
		CHECK(growing.size.x > cardRect.size.x - 1);

		halnf peak = 0;
		for (int frame = 0; frame < 120; frame++) {
			driver.advance(8);
			peak = std::max(peak, scene.board.fullViewAmount());
		}
		CHECK(peak <= 1.0001f);
		driver.settle();
		const RectF full = scene.board.fullViewFrame().windowRect();
		CHECK_CLOSE(scene.board.size().x - 2 * FullViewFrame::kMargin, full.size.x, 0.5f);
		CHECK(detailed.isVisible());
		CHECK(!driver.paint().hasText("Card 4"));
		CHECK(!scene.board.columnScroller().isVisible());
		CHECK(changes == std::vector<std::optional<KanbanItemId>>{ 4 });

		scene.board.fullViewFrame().requestBack();
		driver.advance(16);
		CHECK(scene.board.columnScroller().isVisible());
		driver.settle();
		CHECK(!scene.board.fullViewItem());
		CHECK(!scene.board.fullViewFrame().isVisible());
		CHECK(changes.back() == std::nullopt);

		driver.click(scene.card(1));
		driver.click(scene.card(1));
		driver.settle();
		CHECK(scene.board.fullViewItem() == std::optional<KanbanItemId>(1));
		CHECK(driver.ui().keyboardFocus() == &scene.board.fullViewFrame());
		driver.key(InputID::ESCAPE);
		driver.settle();
		CHECK(!scene.board.fullViewItem());
		scene.board.fullViewFrame().setContent(nullptr);
	}

	TEST(EscapeInAFocusedChildOfTheFullViewDoesNotLeaveIt) {
		BoardScene scene;
		UiDriver driver(scene.board, { 1000, 700 });
		driver.settle();
		TextField field;
		scene.board.fullViewFrame().setContent(&field);
		scene.board.enterFullView(4);
		driver.settle();

		driver.click(field);
		REQUIRE CHECK(driver.ui().keyboardFocus() == &field);
		driver.key(InputID::ESCAPE);
		driver.settle();
		CHECK(scene.board.fullViewItem() == std::optional<KanbanItemId>(4));

		driver.ui().setKeyboardFocus(&scene.board.fullViewFrame());
		driver.key(InputID::ESCAPE);
		driver.settle();
		CHECK(!scene.board.fullViewItem());
		scene.board.fullViewFrame().setContent(nullptr);
	}

	TEST(DragDoesNotSelectOnRelease) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.drag(driver.centerOf(scene.card(2)), scene.centerOfColumn(2), 8);

		CHECK(scene.selected.empty());
	}

	TEST(EscapeCancelsDragAndRestoresCard) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.moveTo(driver.centerOf(scene.card(2)));
		driver.press();
		driver.moveTo(scene.centerOfColumn(2));
		REQUIRE CHECK(scene.board.isDragging());
		driver.key(InputID::ESCAPE);
		driver.release();

		CHECK(!scene.board.isDragging());
		CHECK(scene.moves.empty());
		CHECK(scene.shownIn(0) == Ids({ 1, 2, 3 }));
		CHECK(!scene.board.columnView(2).stack().placeholderIndex().has_value());
	}

	TEST(ColumnThatRefusesDropsKeepsCardHome) {
		std::vector<KanbanColumn> columns = BoardScene::defaultColumns();
		columns[2].acceptsDrops = false;
		BoardScene scene(columns);
		UiDriver driver(scene.board);

		driver.moveTo(driver.centerOf(scene.card(1)));
		driver.press();
		driver.moveTo(scene.centerOfColumn(2));
		REQUIRE CHECK(scene.board.dropTarget().has_value());
		CHECK(!scene.board.dropTarget()->accepted);
		driver.release();

		CHECK(scene.moves.empty());
		CHECK(scene.shownIn(0) == Ids({ 1, 2, 3 }));
	}

	TEST(FullColumnRefusesDropsFromOtherColumns) {
		std::vector<KanbanColumn> columns = BoardScene::defaultColumns();
		columns[1].cardLimit = 1;
		BoardScene scene(columns);
		UiDriver driver(scene.board);

		driver.drag(driver.centerOf(scene.card(1)), scene.centerOfColumn(1), 8);
		CHECK(scene.moves.empty());

		driver.drag(driver.centerOf(scene.card(4)), scene.centerOfColumn(2), 8);
		CHECK_EQUAL(1u, scene.moves.size());
	}

	TEST(ModelCanVetoMoves) {
		BoardScene scene;
		scene.model.refused.push_back({ 1, 0, 0, 2, 0 });
		UiDriver driver(scene.board);

		driver.drag(driver.centerOf(scene.card(1)), scene.centerOfColumn(2), 8);
		CHECK(scene.moves.empty());

		driver.drag(driver.centerOf(scene.card(1)), scene.belowCard(4), 8);
		CHECK_EQUAL(1u, scene.moves.size());
	}

	TEST(ModelDefinedOrderingAppendsAndIgnoresReorder) {
		std::vector<KanbanColumn> columns = BoardScene::defaultColumns();
		for (KanbanColumn& column : columns)
			column.ordering = KanbanOrdering::ModelDefined;
		BoardScene scene(columns);
		UiDriver driver(scene.board);

		driver.drag(driver.centerOf(scene.card(3)), scene.aboveCard(1), 8);
		CHECK(scene.moves.empty());

		driver.moveTo(driver.centerOf(scene.card(1)));
		driver.press();
		driver.moveTo(scene.aboveCard(4));
		CHECK(!scene.board.columnView(1).stack().placeholderIndex().has_value());
		CHECK_EQUAL(1u, scene.board.dropTarget()->index);
		driver.release();
		REQUIRE CHECK_EQUAL(1u, scene.moves.size());
		CHECK_EQUAL(1u, scene.moves[0].toIndex);
	}

	TEST(ModelChangesAddMoveAndRemoveCards) {
		BoardScene scene;
		UiDriver driver(scene.board);
		KanbanCard* keptCard = &scene.card(3);
		driver.click(scene.card(2));

		scene.model.columns = { { 1 }, { 4, 3 }, { 5 } };
		scene.board.modelChanged();
		driver.settle();

		CHECK(scene.shownIn(0) == Ids({ 1 }));
		CHECK(scene.shownIn(1) == Ids({ 4, 3 }));
		CHECK(scene.shownIn(2) == Ids({ 5 }));
		CHECK(scene.board.card(2) == nullptr);
		CHECK(!scene.board.selection().has_value());
		CHECK(&scene.card(3) == keptCard);
		CHECK(contains(scene.board.columnView(1).windowRect(), scene.card(3).windowRect().center()));
	}

	TEST(CardFactoryAndUpdaterCustomizeCards) {
		BoardScene scene;
		int created = 0;
		scene.board.setCardFactory([&](KanbanItemId id) {
			created++;
			auto card = std::make_unique<KanbanCard>(id);
			card->setDetail("detail " + std::to_string(id));
			return card;
		});
		scene.model.columns = { { 7 }, {}, {} };
		scene.board.modelChanged();
		UiDriver driver(scene.board);

		CHECK_EQUAL(1, created);
		CHECK_EQUAL("detail 7", scene.card(7).detail());
		CHECK_EQUAL("Card 7", scene.card(7).title());
	}

	TEST(DraggedCardFollowsPointer) {
		BoardScene scene;
		UiDriver driver(scene.board);
		const Vec2F start = driver.centerOf(scene.card(1));

		driver.moveTo(start);
		driver.press();
		driver.moveTo(start + Vec2F(120, 60));

		const Vec2F center = scene.card(1).windowRect().center();
		CHECK_CLOSE(start.x + 120, center.x, 1.0f);
		CHECK_CLOSE(start.y + 60, center.y, 1.0f);
		driver.release();
	}

	TEST(TouchNeedsLongPressBeforeDragging) {
		BoardScene scene;
		UiDriver driver(scene.board);
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);

		driver.drag(driver.centerOf(scene.card(1)), scene.centerOfColumn(2), 8);
		CHECK(scene.moves.empty());

		driver.moveTo(driver.centerOf(scene.card(1)));
		driver.press();
		driver.advance(Ui::kLongPressMs + 50);
		driver.moveTo(scene.centerOfColumn(2));
		driver.release();
		CHECK_EQUAL(1u, scene.moves.size());
	}

	TEST(ColumnsShareTheWholeWidthWithoutAnUpperLimit) {
		BoardScene scene;
		scene.board.setColumnWidthRange(100, kUnbounded);
		UiDriver driver(scene.board, { 1800, 500 });

		const RectF last = scene.board.columnView(2).windowRect();
		CHECK(last.pos.x + last.size.x > 1750);
		CHECK(!scene.board.columnScroller().isScrollable());
	}

	TEST(ColumnsScrollHorizontallyWhenNarrow) {
		std::vector<KanbanColumn> columns;
		for (int index = 0; index < 8; index++)
			columns.push_back({ "Column " + std::to_string(index) });
		BoardScene scene(columns);
		UiDriver driver(scene.board, { 600, 400 });

		CHECK(scene.board.columnScroller().isScrollable());
		CHECK(scene.board.columnView(0).size().x >= 219.0f);
	}

	TEST(NarrowColumnsKeepTheMinimumWidthAndTrackpadSwipesScrollThem) {
		std::vector<KanbanColumn> columns;
		for (int index = 0; index < 6; index++)
			columns.push_back({ "Column " + std::to_string(index) });
		BoardScene scene(columns);
		scene.board.setColumnWidthRange(200, kUnbounded);
		UiDriver driver(scene.board, { 700, 400 });

		for (size_t column = 0; column < scene.board.columnCount(); column++)
			CHECK_CLOSE(200.0f, scene.board.columnView(column).size().x, 0.5f);
		driver.moveTo(scene.centerOfColumn(1));
		driver.horizontalWheel(-3);
		driver.settle();
		CHECK(scene.board.columnScroller().offset() > 0);
	}
}

SUITE(KanbanColumnMove) {
	TEST(DroppingAHeaderPastTheLastColumnMovesItToTheEnd) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.drag(scene.headerOf(0), scene.rightEdgeOf(2), 8);

		REQUIRE CHECK_EQUAL(1u, scene.columnMoves.size());
		CHECK(scene.columnMoves[0] == ColumnMove(0, 2));
		CHECK(scene.columnTitles() == Titles({ "Doing", "Done", "To do" }));
		CHECK(scene.board.columnView(0).windowRect().pos.x < scene.board.columnView(2).windowRect().pos.x);
		CHECK(!scene.board.isMovingColumn());
	}

	TEST(DroppingAHeaderOnTheLeftHalfOfAColumnPlacesItBefore) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.drag(scene.headerOf(2), scene.leftEdgeOf(0), 8);

		REQUIRE CHECK_EQUAL(1u, scene.columnMoves.size());
		CHECK(scene.columnMoves[0] == ColumnMove(2, 0));
		CHECK(scene.columnTitles() == Titles({ "Done", "To do", "Doing" }));
	}

	TEST(DroppingAHeaderOnTheRightHalfOfANeighbourSwapsThem) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.drag(scene.headerOf(0), scene.rightEdgeOf(1), 8);

		REQUIRE CHECK_EQUAL(1u, scene.columnMoves.size());
		CHECK(scene.columnMoves[0] == ColumnMove(0, 1));
		CHECK(scene.columnTitles() == Titles({ "Doing", "To do", "Done" }));
	}

	TEST(DroppingAHeaderBackOnItsOwnColumnChangesNothing) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.drag(scene.headerOf(1), scene.rightEdgeOf(1), 8);

		CHECK(scene.columnMoves.empty());
		CHECK(scene.columnTitles() == Titles({ "To do", "Doing", "Done" }));
	}

	TEST(AMovedColumnKeepsItsCards) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.drag(scene.headerOf(0), scene.rightEdgeOf(2), 8);
		driver.settle();

		CHECK(scene.shownIn(2) == Ids({ 1, 2, 3 }));
		CHECK(scene.shownIn(0) == Ids({ 4 }));
		CHECK_EQUAL("3", scene.board.columnView(2).count().text());
		CHECK_EQUAL(2u, *scene.board.columnOf(1));
	}

	TEST(OptionsMenusListColumnsInTheirShownOrder) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.drag(scene.headerOf(0), scene.rightEdgeOf(2), 8);

		for (size_t column = 0; column < scene.board.columnCount(); column++)
			CHECK(scene.menuTitles(column) == Titles({ "Doing", "Done", "To do" }));
	}

	TEST(AHiddenColumnIsSkippedWhenChoosingTheDropSlot) {
		BoardScene scene;
		UiDriver driver(scene.board);
		scene.board.setColumnVisible(1, false);

		driver.drag(scene.headerOf(0), scene.rightEdgeOf(2), 8);

		REQUIRE CHECK_EQUAL(1u, scene.columnMoves.size());
		CHECK(scene.columnMoves[0] == ColumnMove(0, 2));
		CHECK(scene.columnTitles() == Titles({ "Doing", "Done", "To do" }));
		CHECK(!scene.board.isColumnVisible(0));
	}

	TEST(TheWholeColumnFollowsThePointerAndTheOthersOpenAGap) {
		BoardScene scene;
		UiDriver driver(scene.board);
		const RectF doing = scene.board.columnView(1).windowRect();
		const RectF done = scene.board.columnView(2).windowRect();

		driver.moveTo(scene.headerOf(0));
		driver.press();
		driver.moveTo(scene.rightEdgeOf(1));
		driver.settle();

		REQUIRE CHECK(scene.board.isMovingColumn());
		CHECK_EQUAL(1u, *scene.board.columnDropIndex());
		const RectF lifted = scene.board.columnView(0).windowRect();
		CHECK(lifted.pos.x > doing.pos.x);
		CHECK(scene.card(1).windowRect().pos.x >= lifted.pos.x);
		CHECK(scene.board.columnView(1).windowRect().pos.x < doing.pos.x);
		CHECK_CLOSE(done.pos.x, scene.board.columnView(2).windowRect().pos.x, 0.5f);
		CHECK(scene.columnMoves.empty());

		driver.release();
		CHECK(!scene.board.isMovingColumn());
		CHECK_EQUAL(1u, scene.columnMoves.size());
	}

	TEST(TheSlotIsOutlinedWhereTheColumnWillLand) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.moveTo(scene.headerOf(0));
		driver.press();
		driver.moveTo(scene.rightEdgeOf(2));
		driver.settle();

		REQUIRE CHECK_EQUAL(2u, *scene.board.columnDropIndex());
		const RectF slot = scene.board.columnView(2).windowRect();
		const RectF expected = makeRect(slot.pos.x + slot.size.x + 10, slot.pos.y, slot.size.x, slot.size.y);
		bool outlined = false;
		for (const PaintCommand& command : driver.paint().commands()) {
			const bool sameRect = std::fabs(command.rect.pos.x - expected.pos.x) < 0.5f &&
														std::fabs(command.rect.size.x - expected.size.x) < 0.5f;
			outlined |= command.kind == PaintCommand::Kind::StrokeRect && sameRect;
		}
		CHECK(outlined);
		driver.release();
	}

	TEST(TheNeighbourMovesOnceTheLiftedColumnEdgeCrossesItsCenter) {
		BoardScene scene;
		UiDriver driver(scene.board);
		const RectF todo = scene.board.columnView(0).windowRect();
		const RectF doing = scene.board.columnView(1).windowRect();
		const Vec2F grab = scene.headerOf(0);
		const halnf toCenter = doing.center().x - (todo.pos.x + todo.size.x);

		driver.moveTo(grab);
		driver.press();
		driver.moveTo({ grab.x + toCenter - 4, grab.y });
		REQUIRE CHECK(scene.board.isMovingColumn());
		CHECK_EQUAL(0u, *scene.board.columnDropIndex());

		driver.moveTo({ grab.x + toCenter + 4, grab.y });
		CHECK_EQUAL(1u, *scene.board.columnDropIndex());
		driver.release();
	}

	TEST(MovingBackUsesTheEdgeFacingTheNeighbour) {
		BoardScene scene;
		UiDriver driver(scene.board);
		const RectF todo = scene.board.columnView(0).windowRect();
		const RectF doing = scene.board.columnView(1).windowRect();
		const Vec2F grab = scene.headerOf(0);
		const halnf grabOffset = grab.x - todo.pos.x;

		driver.moveTo(grab);
		driver.press();
		driver.moveTo({ doing.center().x - todo.size.x + grabOffset + 4, grab.y });
		driver.settle();
		REQUIRE CHECK_EQUAL(1u, *scene.board.columnDropIndex());

		const halnf doingCenterNow = todo.center().x;
		driver.moveTo({ doingCenterNow + grabOffset + 4, grab.y });
		CHECK_EQUAL(1u, *scene.board.columnDropIndex());

		driver.moveTo({ doingCenterNow + grabOffset - 4, grab.y });
		CHECK_EQUAL(0u, *scene.board.columnDropIndex());
		driver.release();
	}

	TEST(EscapeCancelsAColumnMove) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.moveTo(scene.headerOf(0));
		driver.press();
		driver.moveTo(scene.rightEdgeOf(2));
		REQUIRE CHECK(scene.board.isMovingColumn());
		driver.key(InputID::ESCAPE);
		driver.release();

		CHECK(!scene.board.isMovingColumn());
		CHECK(scene.columnMoves.empty());
		CHECK(scene.columnTitles() == Titles({ "To do", "Doing", "Done" }));
	}

	TEST(AClickOnAHeaderIsNotAMove) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.click(scene.headerOf(1));

		CHECK(scene.columnMoves.empty());
		CHECK(!scene.board.isMovingColumn());
	}

	TEST(TheOptionsButtonStillOpensItsMenu) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.click(scene.board.columnView(0).optionsButton());

		CHECK(scene.columnMoves.empty());
		CHECK(scene.board.columnView(0).optionsMenu().isOpen());
	}

	TEST(CardDraggingStillWorksAfterAColumnMove) {
		BoardScene scene;
		UiDriver driver(scene.board);

		driver.drag(scene.headerOf(0), scene.rightEdgeOf(2), 8);
		driver.drag(driver.centerOf(scene.card(4)), scene.centerOfColumn(1), 8);

		REQUIRE CHECK_EQUAL(1u, scene.moves.size());
		CHECK(scene.moves[0] == (KanbanMove{ 4, 0, 0, 1, 0 }));
		CHECK(scene.shownIn(1) == Ids({ 4 }));
	}

	TEST(MovingAColumnProgrammaticallyReportsIt) {
		BoardScene scene;
		UiDriver driver(scene.board);

		scene.board.moveColumn(2, 0);
		scene.board.moveColumn(1, 1);
		scene.board.moveColumn(5, 0);

		REQUIRE CHECK_EQUAL(1u, scene.columnMoves.size());
		CHECK(scene.columnMoves[0] == ColumnMove(2, 0));
		CHECK(scene.columnTitles() == Titles({ "Done", "To do", "Doing" }));
	}
}

SUITE(KanbanColumnVisibility) {
	bool shownOnScreen(const Widget& widget) {
		for (const Widget* current = &widget; current; current = current->parent())
			if (!current->isVisible()) return false;
		return widget.ui() != nullptr;
	}

	std::vector<std::string> shownTitles(KanbanBoard & board) {
		std::vector<std::string> titles;
		for (size_t column = 0; column < board.columnCount(); column++)
			if (board.columnView(column).isVisible()) titles.push_back(board.columnView(column).title().text());
		return titles;
	}

	TEST(HiddenColumnsTakeNoSpaceAndTheRestShareTheWidth) {
		std::vector<KanbanColumn> columns = BoardScene::defaultColumns();
		columns[1].visible = false;
		BoardScene scene(columns);
		scene.board.setColumnWidthRange(100, kUnbounded);
		UiDriver driver(scene.board, { 1200, 500 });

		CHECK((shownTitles(scene.board) == std::vector<std::string>{ "To do", "Done" }));
		CHECK(!shownOnScreen(scene.card(4)));
		CHECK(scene.board.columnView(2).windowRect().pos.x < 700);
		CHECK(scene.board.columnView(0).size().x > 500);
	}

	TEST(EveryColumnMenuTogglesAnyColumn) {
		BoardScene scene;
		std::vector<std::pair<size_t, bool>> reported;
		scene.board.setOnColumnVisibilityChanged([&](size_t column, bool visible) {
			reported.emplace_back(column, visible);
		});
		UiDriver driver(scene.board, { 1200, 500 });
		KanbanColumnView& done = scene.board.columnView(2);

		driver.click(done.optionsButton());
		driver.settle();
		REQUIRE CHECK(done.optionsMenu().isOpen());
		REQUIRE CHECK_EQUAL(3u, done.optionsMenu().items().size());
		CHECK_EQUAL("Doing", done.optionsMenu().items()[1]->text());
		driver.click(*done.optionsMenu().items()[1]);
		driver.settle();

		CHECK(!done.optionsMenu().isOpen());
		CHECK((shownTitles(scene.board) == std::vector<std::string>{ "To do", "Done" }));
		CHECK((reported == std::vector<std::pair<size_t, bool>>{ { 1, false } }));

		scene.board.setColumnVisible(1, true);
		driver.settle();
		CHECK((shownTitles(scene.board) == std::vector<std::string>{ "To do", "Doing", "Done" }));
		CHECK(shownOnScreen(scene.card(4)));
	}

	TEST(TheLastVisibleColumnCannotBeHidden) {
		BoardScene scene;
		UiDriver driver(scene.board, { 1200, 500 });
		scene.board.setColumnVisible(0, false);
		scene.board.setColumnVisible(1, false);

		scene.board.setColumnVisible(2, false);

		CHECK(scene.board.isColumnVisible(2));
		CHECK_EQUAL(1u, scene.board.visibleColumnCount());
		KanbanColumnView& done = scene.board.columnView(2);
		driver.click(done.optionsButton());
		driver.settle();
		CHECK(!done.optionsMenu().items()[2]->isEffectivelyEnabled());
	}

	TEST(DroppingNearAHiddenColumnTargetsAVisibleOne) {
		std::vector<KanbanColumn> columns = BoardScene::defaultColumns();
		columns[1].visible = false;
		BoardScene scene(columns);
		UiDriver driver(scene.board, { 1200, 500 });

		driver.drag(driver.centerOf(scene.card(1)), scene.centerOfColumn(2), 10);
		driver.settle();

		REQUIRE CHECK_EQUAL(1u, scene.moves.size());
		CHECK_EQUAL(2u, scene.moves[0].toColumn);
	}
}

SUITE(KanbanBoardLayout) {
	TEST(ColumnTitleAndCountSitAboveTheAccentLine) {
		std::vector<KanbanColumn> columns = BoardScene::defaultColumns();
		columns[0].accent = RGBA{ 1, 0, 0, 1 };
		BoardScene scene(columns);
		UiDriver driver(scene.board);

		const RectF title = scene.board.columnView(0).title().windowRect();
		const RectF count = scene.board.columnView(0).count().windowRect();
		const RectF firstCard = scene.card(1).windowRect();
		const PaintCommand* line = nullptr;
		for (const PaintCommand& command : driver.paint().commands())
			if (command.kind == PaintCommand::Kind::FillRect && command.color.r == 1 && command.color.g == 0) line = &command;

		REQUIRE CHECK(line != nullptr);
		CHECK(title.pos.y + title.size.y <= line->rect.pos.y + 0.5f);
		CHECK(count.pos.y + count.size.y <= line->rect.pos.y + 0.5f);
		CHECK(line->rect.pos.y + line->rect.size.y <= firstCard.pos.y);
	}

	TEST(FixedCardHeightClampsLongTextWithAnEllipsis) {
		BoardScene scene;
		scene.board.setCardHeight(90);
		scene.board.setCardUpdater([](KanbanCard& card) {
			card.setTitle(std::string(300, 'T'));
			card.setDetail(card.id() == 2 ? std::string(3000, 'd') : "short");
		});
		scene.board.refreshCards();
		UiDriver driver(scene.board);

		CHECK_CLOSE(90.0f, scene.card(1).size().y, 0.01f);
		CHECK_CLOSE(90.0f, scene.card(2).size().y, 0.01f);
		CHECK_CLOSE(90.0f, scene.card(4).size().y, 0.01f);

		bool ellipsized = false;
		for (const PaintCommand& command : driver.paint().commands())
			if (command.kind == PaintCommand::Kind::Text && command.text.find("\u2026") != std::string::npos)
				ellipsized = true;
		CHECK(ellipsized);
	}

	TEST(CardsGrowWithContentWithoutAFixedHeight) {
		BoardScene scene;
		scene.board.setCardUpdater([](KanbanCard& card) {
			card.setTitle("Card");
			card.setDetail(card.id() == 2 ? std::string(600, 'd') : "");
		});
		scene.board.refreshCards();
		UiDriver driver(scene.board);

		CHECK(scene.card(2).size().y > scene.card(1).size().y + 20);
	}
}

SUITE(LabelMaximumLines) {
	TEST(WrappedTextStopsAtTheLimitWithAnEllipsis) {
		Label label(std::string(400, 'a'));
		label.setWrap(true);
		label.setMaximumLines(2);
		label.setSizing(AxisSizing::fixed(120), AxisSizing::fit());
		UiDriver driver(label);

		const std::vector<std::string> lines = label.linesFor(120);
		REQUIRE CHECK_EQUAL(2u, lines.size());
		CHECK(lines[1].size() >= 3 && lines[1].substr(lines[1].size() - 3) == "\u2026");
		CHECK(label.textWidth(lines[1], label.effectiveFontSize()) <= 120.5f);
	}

	TEST(EllipsisNeverSitsAloneOnABlankLine) {
		Label label("first paragraph\n\nsecond paragraph\nthird");
		label.setWrap(true);
		label.setMaximumLines(2);

		CHECK((label.linesFor(500) == std::vector<std::string>{ "first paragraph\u2026" }));
	}

	TEST(BlankLinesInsideTheLimitAreKept) {
		Label label("one\n\nthree\nfour");
		label.setWrap(true);
		label.setMaximumLines(3);

		CHECK((label.linesFor(500) == std::vector<std::string>{ "one", "", "three\u2026" }));
	}

	TEST(ShortTextIsUntouched) {
		Label label("one line");
		label.setWrap(true);
		label.setMaximumLines(2);

		CHECK((label.linesFor(500) == std::vector<std::string>{ "one line" }));
	}

	TEST(TrimmingNeverSplitsAMultibyteCharacter) {
		std::string text;
		for (int index = 0; index < 200; index++)
			text += "\u00e9";
		Label label(text);
		label.setWrap(true);
		label.setMaximumLines(1);

		const std::string line = label.linesFor(60).front();
		const std::string body = line.substr(0, line.size() - 3);
		CHECK_EQUAL(0u, body.size() % 2);
		for (size_t index = 0; index < body.size(); index += 2)
			CHECK_EQUAL(std::string("\u00e9"), body.substr(index, 2));
	}
}
