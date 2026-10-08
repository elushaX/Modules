#include "BoxLayout.hpp"
#include "Button.hpp"
#include "Containers.hpp"
#include "StackLayout.hpp"
#include "TextField.hpp"
#include "Label.hpp"
#include "TreeView.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <algorithm>
#include <cmath>
#include <map>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class EditableTree : public TreeModel {
	public:
		std::map<TreeItemId, std::optional<TreeItemId>> parentOf;

		void add(TreeItemId id, std::optional<TreeItemId> parent) { parentOf[id] = parent; }
		void setParent(TreeItemId child, TreeItemId parent) { parentOf[child] = parent; }

		[[nodiscard]] std::vector<TreeItemId> roots() const override {
			std::vector<TreeItemId> result;
			for (const auto& [id, parent] : parentOf)
				if (!parent) result.push_back(id);
			return result;
		}
		[[nodiscard]] std::vector<TreeItemId> children(TreeItemId id) const override {
			std::vector<TreeItemId> result;
			for (const auto& [child, parent] : parentOf)
				if (parent == id) result.push_back(child);
			return result;
		}
	};

	struct TreeScene {
		EditableTree tree;
		Widget root;
		TreeView view{ tree };
		std::vector<std::pair<TreeItemId, TreeItemId>> reparented;
		std::vector<TreeItemId> created;
		std::vector<TreeItemId> selected;
		std::vector<std::vector<TreeItemId>> deletions;

		explicit TreeScene(size_t levels = 4, size_t fanOut = 2) {
			TreeItemId next = 1;
			tree.add(next++, std::nullopt);
			std::vector<TreeItemId> level = { 1 };
			for (size_t depth = 1; depth < levels; depth++) {
				std::vector<TreeItemId> deeper;
				for (TreeItemId parent : level)
					for (size_t child = 0; child < fanOut; child++) {
						tree.add(next, parent);
						deeper.push_back(next++);
					}
				level = deeper;
			}

			root.emplaceLayout<StackLayout>();
			root.addChild(view);
			view.setCardFactory([](TreeItemId id) {
				auto card = std::make_unique<TreeCard>(id);
				card->setTitle("Item " + std::to_string(id));
				card->setLabel("#" + std::to_string(id));
				return card;
			});
			view.modelChanged();
			view.setOnReparent([this](TreeItemId child, TreeItemId parent) {
				reparented.emplace_back(child, parent);
				tree.setParent(child, parent);
				view.modelChanged();
			});
			view.setOnCreateChild([this](TreeItemId parent) {
				const TreeItemId id = tree.parentOf.rbegin()->first + 1;
				tree.add(id, parent);
				created.push_back(id);
				view.modelChanged();
				view.select(id);
			});
			view.setOnSelected([this](TreeItemId id) { selected.push_back(id); });
			view.setOnDeleteRequested([this](const std::vector<TreeItemId>& ids) { deletions.push_back(ids); });
		}

		Vec2F windowPoint(Vec2F local) { return view.windowPosition() + local; }
		Vec2F centerOf(TreeItemId id) {
			const RectF rect = *view.cardRect(id);
			return windowPoint(rect.pos + rect.size / 2.0f);
		}
		Vec2F middle() { return windowPoint(view.size() / 2.0f); }
		Vec2F portOf(TreeItemId id, TreeView::Port port) { return windowPoint(*view.portPosition(id, port)); }
		bool isShown(TreeItemId id) { return view.stateOf(id) == TreeItemState::Shown; }

		std::map<TreeItemId, Vec2F> shownCenters() {
			std::map<TreeItemId, Vec2F> centers;
			for (const TreeStructure::Item& item : view.structure().items())
				if (isShown(item.id)) centers[item.id] = centerOf(item.id);
			return centers;
		}

		void showLevels(UiDriver& driver, size_t depth, halnf scale = 0.6f) {
			view.select(std::nullopt);
			view.openTo(std::nullopt, depth);
			view.setView(view.pan(), scale, false);
			view.frame(false);
			driver.settle();
		}
	};

	halnf length(Vec2F vector) { return std::sqrt(vector.x * vector.x + vector.y * vector.y); }

	int pushOneStep(UiDriver& driver, TreeView& view, halnf direction) {
		std::vector<bool> open;
		for (const TreeStructure::Item& item : view.structure().items())
			open.push_back(view.isOpen(item.id));
		const auto changed = [&] {
			for (size_t index = 0; index < open.size(); index++)
				if (view.isOpen(view.structure().items()[index].id) != open[index]) return true;
			return false;
		};
		int notches = 0;
		while (!changed() && notches < 40) {
			driver.horizontalWheel(direction);
			notches++;
		}
		driver.settle();
		return notches;
	}
}

SUITE(TreeView) {
	TEST(OpensAsMuchOfTheTreeAsFitsAndCentresIt) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		CHECK_EQUAL(2u, scene.view.openDepth(std::nullopt));
		CHECK(scene.view.detailAmount() > 0.99f);
		for (const auto& [id, center] : scene.shownCenters())
			CHECK(contains(scene.view.windowRect(), center));

		TreeScene deep(8, 2);
		UiDriver deepDriver(deep.root, { 900, 600 });
		deepDriver.settle();
		CHECK(deep.view.openDepth(std::nullopt) < 7);
		const RectF area = deep.view.windowRect();
		for (const auto& [id, center] : deep.shownCenters())
			CHECK(contains(area, center));
	}

	TEST(TappingTheSelectedItemOpensAndClosesIt) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 0);
		REQUIRE CHECK(!scene.isShown(2));

		driver.click(scene.centerOf(1));
		driver.settle();
		CHECK_EQUAL(1u, *scene.view.selection());
		CHECK(!scene.view.isOpen(1));

		driver.click(scene.centerOf(1));
		driver.settle();
		CHECK(scene.view.isOpen(1));
		CHECK(scene.isShown(2));
		CHECK(scene.isShown(3));
		CHECK(!scene.isShown(4));

		driver.click(scene.centerOf(1));
		driver.settle();
		CHECK(!scene.view.isOpen(1));
		CHECK(!scene.isShown(2));
	}

	TEST(SecondTapDoesNotOpenAfterADoubleClickOnAnotherCard) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 1);
		driver.click(scene.centerOf(2));
		driver.click(scene.centerOf(2));
		driver.settle();
		CHECK(scene.view.isOpen(2));
		CHECK_EQUAL(2u, *scene.view.selection());
	}

	TEST(ChildrenSpringOutOfTheirParentAndFadeIn) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 0);
		driver.click(scene.centerOf(1));
		driver.settle();

		scene.view.setOpen(1, true);
		driver.layout();
		CHECK(length(scene.centerOf(2) - scene.centerOf(1)) < 1);
		CHECK(scene.view.emergence(2) < 0.05f);

		halnf previous = scene.view.emergence(2);
		for (int frame = 0; frame < 40; frame++) {
			driver.advance(16);
			CHECK(scene.view.emergence(2) >= previous - 0.0001f);
			CHECK(scene.view.emergence(2) <= 1.0f);
			previous = scene.view.emergence(2);
		}
		driver.settle();
		CHECK_CLOSE(1.0f, scene.view.emergence(2), 0.0001f);
		CHECK(scene.centerOf(2).x > scene.centerOf(1).x + 60);

		scene.view.setOpen(1, false);
		driver.advance(64);
		CHECK(scene.view.emergence(2) > 0.05f);
		CHECK(scene.view.emergence(2) < 0.95f);
		driver.settle();
		CHECK(!scene.view.card(2)->isVisible());
	}

	TEST(DiagonalWheelZoomsOneAxisUntilTheGestureRests) {
		TreeScene scene(4, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 1);
		driver.click(scene.centerOf(2));
		driver.settle();
		driver.moveTo(scene.middle());
		const halnf scale = scene.view.scale();

		for (int notch = 0; notch < 6; notch++)
			driver.ui().dispatch(Event::wheel(scene.middle(), 0.4f, 1.0f));
		driver.settle();
		CHECK(scene.view.isOpen(2));
		CHECK_CLOSE(scale, scene.view.scale(), 0.001f);

		for (int notch = 0; notch < 6; notch++)
			driver.ui().dispatch(Event::wheel(scene.middle(), 1.0f, 0.4f));
		driver.settle();
		CHECK(scene.view.isOpen(2));
		CHECK(scene.view.scale() != scale);
	}

	TEST(HorizontalWheelOnASelectedItemOnlyOpensItsSubtree) {
		TreeScene scene(4, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 1);
		driver.click(scene.centerOf(2));
		driver.settle();
		driver.moveTo(scene.middle());

		CHECK(pushOneStep(driver, scene.view, 1) > 1);
		CHECK(scene.view.isOpen(2));
		CHECK(!scene.view.isOpen(3));
		CHECK(scene.isShown(4));
		CHECK(!scene.isShown(6));

		pushOneStep(driver, scene.view, 1);
		CHECK(scene.view.isOpen(4));
		CHECK(scene.view.isOpen(5));
		CHECK(!scene.view.isOpen(3));

		pushOneStep(driver, scene.view, -1);
		CHECK(!scene.view.isOpen(4));
		CHECK(scene.view.isOpen(2));
		pushOneStep(driver, scene.view, -1);
		CHECK(!scene.view.isOpen(2));
		CHECK(scene.view.isOpen(1));
	}

	TEST(HorizontalWheelWithoutSelectionOpensTheWholeTreeLevelByLevel) {
		TreeScene scene(4, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 0);
		driver.moveTo(scene.middle());
		const halnf scale = scene.view.targetScale();
		std::vector<size_t> depths = { 0 };
		for (int step = 0; step < 4; step++) {
			pushOneStep(driver, scene.view, 1);
			depths.push_back(scene.view.openDepth(std::nullopt));
		}
		CHECK((depths == std::vector<size_t>{ 0, 1, 2, 3, 3 }));
		CHECK_CLOSE(scale, scene.view.targetScale(), 0.0001f);
		for (int step = 0; step < 3; step++)
			pushOneStep(driver, scene.view, -1);
		CHECK_EQUAL(0u, scene.view.openDepth(std::nullopt));
	}

	TEST(VerticalZoomOnlyScalesTheCameraAndNeverChangesTopology) {
		TreeScene scene(4, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 1);
		driver.moveTo(scene.middle());
		const halnf start = scene.view.targetScale();
		std::vector<bool> open;
		for (const TreeStructure::Item& item : scene.view.structure().items())
			open.push_back(scene.view.isOpen(item.id));
		const auto unchanged = [&] {
			for (size_t index = 0; index < open.size(); index++)
				if (scene.view.isOpen(scene.view.structure().items()[index].id) != open[index]) return false;
			return true;
		};

		for (int notch = 0; notch < 12; notch++)
			driver.wheel(1);
		driver.settle();
		CHECK(scene.view.targetScale() > start * 2);
		CHECK(unchanged());
		CHECK(!scene.view.isPullingLevel());

		for (int notch = 0; notch < 30; notch++)
			driver.wheel(-1);
		driver.settle();
		CHECK(scene.view.targetScale() < start);
		CHECK(unchanged());

		const Vec2F center = scene.middle();
		driver.gesture({ GesturePhase::Begin, center, { 0, 0 }, 1 });
		for (int step = 0; step < 10; step++)
			driver.gesture({ GesturePhase::Update, center, { 0, 0 }, 1.1f });
		driver.gesture({ GesturePhase::End, center, { 0, 0 }, 1 });
		driver.settle();
		CHECK(unchanged());
	}

	TEST(CameraScaleDecidesBetweenCompactAndDetailedCards) {
		TreeScene scene(2, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		Button control("Run");
		scene.view.card(1)->content().addChild(control);

		scene.view.setView(scene.view.pan(), 1, false);
		scene.view.centerOn(1, false);
		driver.settle();
		CHECK_CLOSE(1.0f, scene.view.detailAmount(), 0.001f);
		CHECK(scene.view.card(1)->content().isVisible());
		CHECK_CLOSE(scene.view.style().layout.card.x, scene.view.cardRect(1)->size.x, 0.01f);

		scene.view.setView(scene.view.pan(), 0.2f, false);
		scene.view.centerOn(1, false);
		driver.settle();
		CHECK_CLOSE(0.0f, scene.view.detailAmount(), 0.001f);
		CHECK(!scene.view.card(1)->content().isVisible());
		CHECK(driver.paint().hasText("#1"));
		CHECK(scene.view.isOpen(1));
		scene.view.card(1)->content().removeChild(control);
	}

	TEST(AtMinimumZoomDecorationsStayInsideTinyCards) {
		TreeScene scene(4, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.view.openTo(std::nullopt, 2);
		scene.view.select(2);
		scene.view.setView(scene.view.pan(), scene.view.style().minimumScale, false);
		scene.view.centerOn(2, false);
		driver.settle();

		const RectF card = *scene.view.cardRect(2);
		REQUIRE CHECK(card.size.y < 12);
		const Vec2F origin = scene.view.windowPosition();
		const RectF allowed = translated(insetRect(card, -2), origin);
		size_t circles = 0;
		for (const PaintCommand& command : driver.paint().commands()) {
			if (command.kind == PaintCommand::Kind::Circle || command.kind == PaintCommand::Kind::StrokeCircle) circles++;
			if (command.kind != PaintCommand::Kind::StrokeRect) continue;
			if (!overlaps(command.rect, allowed)) continue;
			CHECK(command.rect.size.x <= allowed.size.x + 0.5f);
			CHECK(command.rect.size.y <= allowed.size.y + 0.5f);
		}
		CHECK_EQUAL(0u, circles);
		CHECK(!scene.view.portAt(*scene.view.portPosition(2, TreeView::Port::Children)));

		scene.view.setView(scene.view.pan(), 1, false);
		scene.view.centerOn(2, false);
		driver.settle();
		CHECK(scene.view.portAt(*scene.view.portPosition(2, TreeView::Port::Children)).has_value());
	}

	TEST(CameraZoomNeverRestsBetweenCompactAndDetailedCards) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		TreeView& view = scene.view;
		const halnf low = view.style().compactBelowScale;
		const halnf high = view.style().detailedAboveScale;
		view.setView(view.pan(), low, false);
		driver.settle();
		driver.moveTo(scene.middle());

		driver.wheel(1);
		driver.advance(40);
		CHECK(view.isPullingCamera());
		CHECK(view.targetScale() > low);
		CHECK(view.targetScale() < high);
		driver.settle();
		CHECK(!view.isPullingCamera());
		CHECK_CLOSE(low, view.scale(), 0.001f);
		CHECK_CLOSE(0.0f, view.detailAmount(), 0.001f);

		int notches = 0;
		while (view.targetScale() < high - 0.001f && notches < 40) {
			driver.wheel(1);
			notches++;
		}
		CHECK(notches > 1);
		driver.settle();
		CHECK_CLOSE(high, view.scale(), 0.001f);
		CHECK_CLOSE(1.0f, view.detailAmount(), 0.001f);

		for (int notch = 0; notch < 3; notch++)
			driver.wheel(1);
		driver.settle();
		CHECK(view.scale() > high);

		for (int notch = 0; notch < 40; notch++) {
			driver.wheel(-1);
			driver.advance(8);
			const halnf resting = view.targetScale();
			if (!view.isPullingCamera()) CHECK(resting <= low + 0.001f || resting >= high - 0.001f);
		}
		driver.settle();
		CHECK(view.scale() < low);
		CHECK_CLOSE(0.0f, view.detailAmount(), 0.001f);
	}

	TEST(PinchIntoTheTransitionSpringsBackOnRelease) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		TreeView& view = scene.view;
		const halnf high = view.style().detailedAboveScale;
		view.setView(view.pan(), high, false);
		driver.settle();
		const Vec2F center = scene.middle();

		driver.gesture({ GesturePhase::Begin, center, { 0, 0 }, 1 });
		driver.gesture({ GesturePhase::Update, center, { 0, 0 }, 0.95f });
		driver.advance(400);
		CHECK(view.isPullingCamera());
		CHECK(view.targetScale() < high);
		driver.gesture({ GesturePhase::End, center, { 0, 0 }, 1 });
		driver.settle();
		CHECK_CLOSE(high, view.scale(), 0.001f);
	}

	TEST(ShiftWheelActsAsHorizontalWheel) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 0);
		driver.moveTo(scene.middle());
		const halnf scale = scene.view.targetScale();
		for (int notch = 0; notch < 40 && !scene.view.isOpen(1); notch++) {
			driver.ui().dispatch(Event::wheel(scene.middle(), 1).withModifiers({ .shift = true }));
		}
		driver.settle();
		CHECK(scene.view.isOpen(1));
		CHECK_CLOSE(scale, scene.view.targetScale(), 0.0001f);
	}

	TEST(ALightPushResistsAndSpringsBack) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 0);
		driver.click(scene.centerOf(1));
		driver.settle();
		driver.moveTo(scene.middle());

		driver.horizontalWheel(0.5f);
		driver.advance(80);
		CHECK(scene.view.isPullingLevel());
		CHECK(!scene.view.isOpen(1));
		CHECK(scene.view.emergence(2) > 0.01f);
		CHECK(scene.view.emergence(2) < 0.35f);
		driver.settle();
		CHECK(!scene.view.isPullingLevel());
		CHECK(!scene.view.isOpen(1));
		CHECK(scene.view.emergence(2) < 0.001f);
	}

	TEST(SelectingCentresTheItem) {
		TreeScene scene(4, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 2);

		driver.click(scene.centerOf(3));
		driver.settle();
		CHECK(length(scene.centerOf(3) - scene.middle()) < 1);
		driver.key(InputID::DOWN);
		driver.settle();
		CHECK(length(scene.centerOf(*scene.view.selection()) - scene.middle()) < 1);
	}

	TEST(OpeningTheSelectedItemZoomsOutUntilItsSubtreeFits) {
		TreeScene scene(6, 3);
		UiDriver driver(scene.root, { 1200, 800 });
		driver.settle();
		TreeView& view = scene.view;
		view.openTo(std::nullopt, 1);
		view.setView(view.pan(), 1, false);
		driver.settle();
		driver.click(scene.centerOf(2));
		driver.settle();
		const halnf before = view.targetScale();

		const auto subtreeFits = [&] {
			const RectF area = view.windowRect();
			for (const TreeStructure::Item& item : view.structure().items()) {
				const TreeStructure& structure = view.structure();
				if (!scene.isShown(item.id) || !structure.isAncestorOrSelf(*structure.indexOf(2), *structure.indexOf(item.id)))
					continue;
				const RectF card = translated(*view.cardRect(item.id), view.windowPosition());
				if (!contains(area, card.pos) || !contains(area, card.pos + card.size)) return false;
			}
			return true;
		};

		driver.click(scene.centerOf(2));
		driver.settle();
		REQUIRE CHECK(view.isOpen(2));
		CHECK(subtreeFits());
		for (int step = 0; step < 2; step++) {
			driver.moveTo(scene.middle());
			pushOneStep(driver, view, 1);
			CHECK(subtreeFits());
		}
		CHECK(view.targetScale() < before);
		const halnf low = view.style().compactBelowScale;
		const halnf high = view.style().detailedAboveScale;
		CHECK(view.targetScale() <= low + 0.001f || view.targetScale() >= high - 0.001f);

		driver.moveTo(scene.middle());
		pushOneStep(driver, view, 1);
		CHECK_CLOSE(view.style().minimumScale, view.targetScale(), 0.0001f);
	}

	TEST(OpeningNeverJumpsTheViewAndMovesItSmoothly) {
		TreeScene scene(5, 3);
		UiDriver driver(scene.root, { 1200, 800 });
		driver.settle();
		TreeView& view = scene.view;
		view.openTo(std::nullopt, 1);
		view.setView(view.pan(), 1, false);
		driver.settle();
		driver.click(scene.centerOf(3));
		driver.settle();

		for (const halnf startScale : { 1.0f, 0.3f }) {
			view.setOpen(3, false);
			view.setView(view.pan(), startScale, false);
			view.centerOn(3, false);
			driver.settle();
			Vec2F previous = scene.centerOf(3);
			driver.click(previous);
			driver.layout();
			CHECK(length(scene.centerOf(3) - previous) < 1);
			for (int frame = 0; frame < 60; frame++) {
				driver.advance(16);
				const Vec2F now = scene.centerOf(3);
				CHECK(length(now - previous) < 40);
				previous = now;
			}
			driver.settle();
			REQUIRE CHECK(view.isOpen(3));
		}
	}

	TEST(CameraZoomAnimatesAroundTheSelectedItem) {
		TreeScene scene(4, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 2, 1.0f);
		driver.click(scene.centerOf(2));
		driver.settle();
		const Vec2F selected = scene.centerOf(2);
		driver.moveTo(scene.centerOf(7));
		const halnf before = scene.view.scale();

		driver.wheel(1);
		driver.advance(16);
		const halnf early = scene.view.scale();
		CHECK(early > before);
		CHECK(early < scene.view.targetScale());
		CHECK(length(scene.centerOf(2) - selected) < 1);
		driver.settle();
		CHECK(length(scene.centerOf(2) - selected) < 1);

		for (int notch = 0; notch < 4; notch++)
			driver.wheel(-1);
		driver.settle();
		CHECK(scene.view.scale() < before);
		CHECK(length(scene.centerOf(2) - selected) < 1);
	}

	TEST(WithoutSelectionTheCardUnderThePointerStaysInPlace) {
		TreeScene scene(4, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 1);
		const Vec2F start = scene.centerOf(3);
		driver.moveTo(start);
		pushOneStep(driver, scene.view, 1);
		CHECK(length(scene.centerOf(3) - start) < 2);
		REQUIRE CHECK(!scene.view.selection());
		for (int notch = 0; notch < 5; notch++)
			driver.wheel(-1);
		driver.settle();
		CHECK(length(scene.centerOf(3) - start) < 2);
	}

	TEST(DraggingAChildPortOntoItsOwnDescendantIsRefused) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 2);
		const TreeItemId grandchild = scene.tree.children(2).front();

		driver.drag(scene.portOf(2, TreeView::Port::Parent), scene.centerOf(grandchild), 6);
		driver.settle();
		CHECK(scene.reparented.empty());
		driver.drag(scene.portOf(grandchild, TreeView::Port::Children), scene.centerOf(1), 6);
		driver.settle();
		CHECK(scene.reparented.empty());
		CHECK_EQUAL(1u, *scene.tree.parentOf[2]);
	}

	TEST(ReparentingMovesTheSubtree) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 2);
		const TreeItemId grandchild = scene.tree.children(2).front();
		driver.drag(scene.portOf(grandchild, TreeView::Port::Parent), scene.centerOf(3), 6);
		driver.settle();
		REQUIRE CHECK_EQUAL(1u, scene.reparented.size());
		CHECK_EQUAL(3u, *scene.tree.parentOf[grandchild]);
		const TreeStructure& structure = scene.view.structure();
		CHECK_EQUAL(*structure.indexOf(3), structure.items()[*structure.indexOf(grandchild)].parent);
		CHECK(scene.isShown(grandchild));
	}

	TEST(AddingChildrenOpensTheParentAndSelectsTheChild) {
		TreeScene scene(2, 1);
		UiDriver driver(scene.root, { 1200, 800 });
		driver.settle();
		for (int added = 0; added < 3; added++) {
			const TreeItemId leaf = scene.tree.parentOf.rbegin()->first;
			const Vec2F port = scene.portOf(leaf, TreeView::Port::Children);
			driver.drag(port, port + Vec2F(120.0f, 40.0f), 6);
			driver.settle();
		}
		CHECK_EQUAL(3u, scene.created.size());
		CHECK_EQUAL(scene.created.back(), *scene.view.selection());
		for (const TreeStructure::Item& item : scene.view.structure().items())
			CHECK(scene.isShown(item.id));
		CHECK(length(scene.centerOf(scene.created.back()) - scene.middle()) < 1);
	}

	TEST(TugSpringsBackAndDoesNotMoveTheTree) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1200, 800 });
		driver.settle();
		driver.click(scene.centerOf(2));
		driver.settle();
		const Vec2F centred = scene.centerOf(2);
		driver.moveTo(centred);
		driver.press();
		driver.moveTo(centred + Vec2F(400.0f, 0.0f));
		const halnf pulled = scene.centerOf(2).x - centred.x;
		CHECK(pulled > 20);
		CHECK(pulled < scene.view.style().tugReach + 1);
		driver.release();
		driver.settle();
		CHECK(length(scene.centerOf(2) - centred) < 0.5f);
		CHECK(scene.view.isOpen(2));
	}

	TEST(ArrowKeysWalkTheHierarchyAndRightOpens) {
		TreeScene scene(3, 3);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 0);
		driver.click(scene.centerOf(1));
		driver.key(InputID::RIGHT);
		driver.settle();
		const std::vector<TreeItemId> children = scene.tree.children(1);
		CHECK(scene.view.isOpen(1));
		CHECK_EQUAL(children[1], *scene.view.selection());
		driver.key(InputID::DOWN);
		CHECK_EQUAL(children[2], *scene.view.selection());
		driver.key(InputID::UP);
		driver.key(InputID::UP);
		CHECK_EQUAL(children[0], *scene.view.selection());
		driver.key(InputID::LEFT);
		CHECK_EQUAL(1u, *scene.view.selection());
	}

	TEST(CtrlClickBoxSelectAndDelete) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 2);

		driver.click(scene.centerOf(2));
		driver.settle();
		driver.click(scene.centerOf(4), { .control = true });
		driver.settle();
		driver.click(scene.centerOf(5), { .shift = true });
		driver.settle();
		CHECK((scene.view.selectedItems() == std::vector<TreeItemId>{ 2, 4, 5 }));
		driver.click(scene.centerOf(4), { .control = true });
		driver.settle();
		CHECK((scene.view.selectedItems() == std::vector<TreeItemId>{ 2, 5 }));

		driver.key(InputID::DELETE_KEY);
		REQUIRE CHECK_EQUAL(1u, scene.deletions.size());
		CHECK((scene.deletions.front() == std::vector<TreeItemId>{ 2, 5 }));

		driver.key(InputID::ESCAPE);
		CHECK(scene.view.selectedItems().empty());
		driver.key(InputID::DELETE_KEY);
		CHECK_EQUAL(1u, scene.deletions.size());
		driver.key(InputID::A, { .control = true });
		CHECK_EQUAL(scene.tree.parentOf.size(), scene.view.selectedItems().size());

		driver.key(InputID::ESCAPE);
		const RectF first = *scene.view.cardRect(4);
		const RectF second = *scene.view.cardRect(5);
		const Vec2F from = scene.windowPoint({ first.pos.x - 10, first.pos.y - 6 });
		const Vec2F to = scene.windowPoint({ second.pos.x + second.size.x + 10, second.pos.y + second.size.y + 6 });
		REQUIRE CHECK(!scene.view.itemAt(from - scene.view.windowPosition()));
		driver.drag(from, to, 6);
		driver.settle();
		CHECK((scene.view.selectedItems() == std::vector<TreeItemId>{ 4, 5 }));
		driver.click(scene.windowPoint({ 4, 4 }));
		CHECK(scene.view.selectedItems().empty());
	}

	TEST(DetailedCardsHoldWorkingWidgets) {
		TreeScene scene(2, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		scene.showLevels(driver, 1, 1.0f);
		int clicks = 0;
		Button control("Run");
		scene.view.card(2)->content().addChild(control);
		control.setOnClicked([&](const Event&) { clicks++; });
		driver.settle();
		CHECK_CLOSE(1.0f, scene.view.detailAmount(), 0.001f);
		REQUIRE CHECK(control.isVisible());
		driver.click(control);
		CHECK_EQUAL(1, clicks);
		scene.view.card(2)->content().removeChild(control);
	}

	TEST(ZoomingPastTheDetailedCardSnapsMagneticallyIntoTheFullView) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		TreeView& view = scene.view;
		scene.showLevels(driver, 1, 1.0f);
		driver.click(scene.centerOf(2));
		driver.settle();
		driver.moveTo(scene.middle());
		std::vector<std::optional<TreeItemId>> changes;
		view.setOnFullViewChanged([&](std::optional<TreeItemId> item) { changes.push_back(item); });

		int notches = 0;
		while (view.targetScale() < view.fullViewScale() - 0.001f && notches < 40) {
			driver.wheel(1);
			notches++;
		}
		driver.settle();
		CHECK(!view.fullViewItem());
		CHECK_CLOSE(view.fullViewScale(), view.scale(), 0.001f);

		driver.wheel(1);
		driver.advance(40);
		CHECK(view.isPullingFullView());
		CHECK(view.fullViewAmount() > 0.01f);
		CHECK(view.fullViewAmount() < 0.5f);
		driver.settle();
		CHECK(!view.isPullingFullView());
		CHECK(!view.fullViewItem());
		CHECK_CLOSE(0.0f, view.fullViewAmount(), 0.001f);

		for (notches = 0; !view.fullViewItem() && notches < 40; notches++)
			driver.wheel(1);
		CHECK(notches > 1);
		driver.settle();
		REQUIRE CHECK(view.fullViewItem() == std::optional<TreeItemId>(2));
		CHECK(changes == std::vector<std::optional<TreeItemId>>{ 2 });
		const RectF full = view.fullViewFrame().rect();
		CHECK_CLOSE(view.size().x - 2 * FullViewFrame::kMargin, full.size.x, 0.5f);
		CHECK_CLOSE(view.size().y - 2 * FullViewFrame::kMargin, full.size.y, 0.5f);
		CHECK(view.fullViewFrame().isVisible());
		CHECK(view.emergence(1) < 0.001f);
		CHECK(view.emergence(2) < 0.001f);
		CHECK(!view.itemAt(scene.windowPoint(view.size() / 2.0f) - view.windowPosition()));

		const RecordingPainter& painted = driver.paint();
		for (const PaintCommand& command : painted.commands()) {
			CHECK(command.kind != PaintCommand::Kind::Polyline);
			CHECK(command.kind != PaintCommand::Kind::Circle);
			CHECK(command.kind != PaintCommand::Kind::StrokeCircle);
		}
		CHECK(!painted.hasText("\u2190 Back"));
	}

	TEST(TheFullViewGrowsWithoutOvershootAndIgnoresZoomWhileMaximized) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		TreeView& view = scene.view;
		scene.showLevels(driver, 1, 1.0f);
		view.select(2);
		driver.settle();
		const RectF before = *view.cardRect(2);

		view.enterFullView(2);
		halnf previous = 0;
		halnf peak = 0;
		bool monotonic = true;
		for (int frame = 0; frame < 120; frame++) {
			driver.advance(8);
			monotonic = monotonic && view.fullViewAmount() >= previous - 0.0001f;
			previous = view.fullViewAmount();
			peak = std::max(peak, previous);
		}
		CHECK(monotonic);
		CHECK(peak <= 1.0001f);
		driver.settle();
		const RectF opened = view.fullViewFrame().rect();
		CHECK(opened.size.x > before.size.x * 2);
		CHECK_CLOSE(1.0f, view.fullViewAmount(), 0.001f);

		driver.moveTo(scene.middle());
		const halnf scale = view.targetScale();
		for (int notch = 0; notch < 40; notch++) {
			driver.wheel(notch % 2 == 0 ? -1.0f : 1.0f);
			driver.horizontalWheel(-1);
		}
		driver.settle();
		CHECK(!view.isPullingFullView());
		CHECK(view.fullViewItem() == std::optional<TreeItemId>(2));
		CHECK_CLOSE(1.0f, view.fullViewAmount(), 0.001f);
		CHECK_CLOSE(scale, view.targetScale(), 0.0001f);

		view.leaveFullView();
		driver.settle();
		CHECK_CLOSE(0.0f, view.fullViewAmount(), 0.001f);
		CHECK(!view.fullViewFrame().isVisible());
		CHECK(length(view.cardRect(2)->size - before.size) < 1);
		CHECK(view.emergence(1) > 0.999f);
		CHECK(view.emergence(2) > 0.999f);
	}

	TEST(ScrollingInsideTheMaximizedFrameScrollsItsContent) {
		TreeScene scene(2, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		TreeView& view = scene.view;
		scene.showLevels(driver, 1, 1.0f);
		ScrollView scroller(Axis::Vertical);
		Widget tall;
		tall.setSizing(AxisSizing::fill(), AxisSizing::fixed(4000));
		scroller.setContent(&tall);
		view.fullViewFrame().setContent(&scroller);
		view.enterFullView(2);
		driver.settle();
		REQUIRE CHECK(scroller.isVisible());
		const halnf scale = view.targetScale();

		driver.moveTo(scroller.windowRect().center());
		for (int notch = 0; notch < 5; notch++)
			driver.wheel(-1);
		driver.settle();
		CHECK(scroller.offset() > 0);
		const halnf scrolled = scroller.offset();
		for (int notch = 0; notch < 3; notch++)
			driver.wheel(1);
		driver.settle();
		CHECK(scroller.offset() < scrolled);
		CHECK(view.fullViewItem() == std::optional<TreeItemId>(2));
		CHECK_CLOSE(1.0f, view.fullViewAmount(), 0.001f);
		CHECK_CLOSE(scale, view.targetScale(), 0.0001f);

		for (Widget* over : { static_cast<Widget*>(&view.fullViewFrame()), static_cast<Widget*>(&scroller) }) {
			if (over == &scroller) tall.setSizing(AxisSizing::fill(), AxisSizing::fixed(10));
			driver.settle();
			driver.moveTo(over->windowRect().center());
			for (int notch = 0; notch < 6; notch++) {
				driver.wheel(-1);
				driver.advance(16);
				CHECK(!view.isPullingFullView());
				CHECK_CLOSE(1.0f, view.fullViewAmount(), 0.001f);
			}
			driver.settle();
			CHECK(view.fullViewItem() == std::optional<TreeItemId>(2));
		}
		view.fullViewFrame().setContent(nullptr);
	}

	bool shownOnScreen(const Widget& widget) {
		for (const Widget* current = &widget; current; current = current->parent())
			if (!current->isVisible()) return false;
		return widget.ui() != nullptr;
	}

	TEST(TheFullViewShowsThePreviewUntilHalfwayThenTheContentBothWays) {
		TreeScene scene(2, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		TreeView& view = scene.view;
		scene.showLevels(driver, 1, 1.0f);
		Label preview("Card face");
		Label detailed("Detailed");
		FullViewFrame& frame = view.fullViewFrame();
		frame.setPreview(&preview);
		frame.setContent(&detailed);

		view.enterFullView(2);
		bool sawPreviewGrowing = false;
		for (int step = 0; step < 200 && frame.progress() < 0.999f; step++) {
			driver.advance(4);
			if (frame.progress() > 0.05f && frame.progress() < FullViewFrame::kContentShownProgress) {
				sawPreviewGrowing |= shownOnScreen(preview);
				CHECK(!shownOnScreen(detailed));
			}
			if (frame.progress() >= FullViewFrame::kContentShownProgress) {
				CHECK(shownOnScreen(detailed));
				CHECK(!shownOnScreen(preview));
			}
		}
		CHECK(sawPreviewGrowing);

		frame.setContent(nullptr);
		view.leaveFullView();
		bool sawPreviewShrinking = false;
		for (int step = 0; step < 200 && frame.isShown(); step++) {
			driver.advance(4);
			if (frame.progress() > 0.05f) sawPreviewShrinking |= shownOnScreen(preview);
		}
		CHECK(sawPreviewShrinking);
		frame.setPreview(nullptr);
	}

	TEST(TheFullViewFrameIsTheGrownCardHoldingTheHostWidgetThatAsksToGoBack) {
		TreeScene scene(2, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		TreeView& view = scene.view;
		scene.showLevels(driver, 1, 1.0f);
		Button detailed("Detailed");
		int clicks = 0;
		detailed.setOnClicked([&](const Event&) {
			clicks++;
			view.fullViewFrame().requestBack();
		});
		view.fullViewFrame().setContent(&detailed);
		view.enterFullView(2);
		driver.settle();
		REQUIRE CHECK(detailed.isVisible());
		REQUIRE CHECK(detailed.parent() != nullptr);
		CHECK(detailed.parent()->parent() == &view.fullViewFrame());
		const RectF frame = view.fullViewFrame().windowRect();
		CHECK(detailed.windowRect().pos.y - frame.pos.y >= FullViewFrame::kContentPadding - 0.5f);

		driver.click(detailed);
		CHECK_EQUAL(1, clicks);
		driver.settle();
		CHECK(!view.fullViewItem());
		CHECK(!view.fullViewFrame().isVisible());
		CHECK(view.selection() == std::optional<TreeItemId>(2));

		view.enterFullView(2);
		driver.settle();
		CHECK(driver.ui().keyboardFocus() == &view.fullViewFrame());
		driver.key(InputID::ESCAPE);
		driver.settle();
		CHECK(!view.fullViewItem());
		CHECK(driver.ui().keyboardFocus() == &view);
		view.fullViewFrame().setContent(nullptr);
	}

	TEST(EscapeLeavesTheFullViewOnlyWhileTheFrameItselfHasFocus) {
		TreeScene scene(2, 2);
		UiDriver driver(scene.root, { 1400, 900 });
		driver.settle();
		TreeView& view = scene.view;
		scene.showLevels(driver, 1, 1.0f);
		TextField field;
		view.fullViewFrame().setContent(&field);
		view.enterFullView(2);
		driver.settle();

		driver.click(field);
		REQUIRE CHECK(driver.ui().keyboardFocus() == &field);
		driver.key(InputID::ESCAPE);
		driver.settle();
		CHECK(view.fullViewItem() == std::optional<TreeItemId>(2));
		CHECK_CLOSE(1.0f, view.fullViewAmount(), 0.001f);

		driver.ui().setKeyboardFocus(&view.fullViewFrame());
		CHECK(driver.ui().keyboardFocus() == &view.fullViewFrame());
		driver.key(InputID::ESCAPE);
		driver.settle();
		CHECK(!view.fullViewItem());
		CHECK_CLOSE(0.0f, view.fullViewAmount(), 0.001f);
		view.fullViewFrame().setContent(nullptr);
	}

	TEST(LinksAndPortsAreClippedToTheView) {
		EditableTree tree;
		tree.add(1, std::nullopt);
		for (TreeItemId id = 2; id < 40; id++)
			tree.add(id, 1);
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Widget above;
		above.setSizing(AxisSizing::fill(), AxisSizing::fixed(200));
		TreeView view{ tree };
		view.setSizing(AxisSizing::fill(), AxisSizing::fill());
		root.addChild(above);
		root.addChild(view);
		view.setCardFactory([](TreeItemId id) { return std::make_unique<TreeCard>(id); });
		UiDriver driver(root, { 900, 700 });
		driver.settle();
		view.openTo(std::nullopt, 1);
		view.setView(view.pan(), 1, false);
		view.centerOn(1, false);
		driver.settle();

		const RectF area = view.windowRect();
		size_t outside = 0;
		for (const PaintCommand& command : driver.paint().commands()) {
			if (
				command.kind != PaintCommand::Kind::Polyline && command.kind != PaintCommand::Kind::Circle &&
				command.kind != PaintCommand::Kind::StrokeCircle
			)
				continue;
			const RectF clip = command.clip;
			if (clip.pos.y < area.pos.y - 0.5f || clip.pos.y + clip.size.y > area.pos.y + area.size.y + 0.5f) outside++;
		}
		CHECK_EQUAL(0u, outside);
	}

	TEST(RemovingTheSelectedItemClearsSelectionAndKeepsWorking) {
		TreeScene scene(3, 2);
		UiDriver driver(scene.root, { 1200, 800 });
		driver.settle();
		driver.click(scene.centerOf(2));
		for (TreeItemId child : scene.tree.children(2))
			scene.tree.parentOf.erase(child);
		scene.tree.parentOf.erase(2);
		scene.view.modelChanged();
		driver.settle();
		CHECK(!scene.view.selection());
		CHECK(!scene.view.card(2));
		driver.moveTo(scene.middle());
		driver.wheel(-3);
		driver.settle();
		CHECK(scene.view.scale() > 0);
	}
}
