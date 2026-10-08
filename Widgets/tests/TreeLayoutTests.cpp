#include "UnitTest++/UnitTest++.h"

#include "TreeLayout.hpp"

#include <cmath>
#include <map>

using namespace tp;
using namespace tp::ui;

namespace {
	class MapTree : public TreeModel {
	public:
		std::vector<TreeItemId> topLevel;
		std::map<TreeItemId, std::vector<TreeItemId>> childrenOf;

		void add(TreeItemId id, std::optional<TreeItemId> parent) {
			if (parent) childrenOf[*parent].push_back(id);
			else topLevel.push_back(id);
		}

		[[nodiscard]] std::vector<TreeItemId> roots() const override { return topLevel; }
		[[nodiscard]] std::vector<TreeItemId> children(TreeItemId id) const override {
			const auto found = childrenOf.find(id);
			return found == childrenOf.end() ? std::vector<TreeItemId>{} : found->second;
		}
	};

	MapTree branchingTree(size_t levels, size_t fanOut) {
		MapTree tree;
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
		return tree;
	}

	struct Scene {
		MapTree tree;
		TreeStructure structure;
		TreeExpansion expansion;
		TreeLayoutMetrics metrics;

		explicit Scene(MapTree model) :
			tree(std::move(model)) {
			structure.rebuild(tree);
		}

		size_t index(TreeItemId id) const { return *structure.indexOf(id); }
		TreePacking pack() const { return TreePacking::pack(structure, expansion, metrics); }
	};
}

SUITE(TreeStructure) {
	TEST(BuildsDepthsChildrenAndDescendantCounts) {
		MapTree tree;
		tree.add(1, std::nullopt);
		tree.add(2, 1);
		tree.add(3, 1);
		tree.add(4, 3);
		tree.add(5, std::nullopt);
		TreeStructure structure;
		structure.rebuild(tree);

		CHECK_EQUAL(5u, structure.items().size());
		CHECK_EQUAL(2u, structure.roots().size());
		CHECK_EQUAL(2u, structure.height());
		CHECK_EQUAL(1u, structure.items()[*structure.indexOf(3)].descendants);
		CHECK_EQUAL(3u, structure.items()[*structure.indexOf(1)].descendants);
		CHECK(structure.isAncestorOrSelf(*structure.indexOf(1), *structure.indexOf(4)));
		CHECK(!structure.isAncestorOrSelf(*structure.indexOf(2), *structure.indexOf(4)));
	}

	TEST(SurvivesACycleInTheModel) {
		MapTree tree;
		tree.add(1, std::nullopt);
		tree.add(2, 1);
		tree.childrenOf[2].push_back(1);
		TreeStructure structure;
		structure.rebuild(tree);
		CHECK_EQUAL(2u, structure.items().size());
	}

	TEST(ReparentingIntoOwnSubtreeIsRefused) {
		MapTree tree;
		tree.add(1, std::nullopt);
		tree.add(2, 1);
		tree.add(3, 2);
		tree.add(4, std::nullopt);
		CHECK(!tree.canReparent(1, 3));
		CHECK(!tree.canReparent(2, 2));
		CHECK(tree.canReparent(3, 4));
		CHECK(tree.canReparent(4, 3));
	}
}

SUITE(TreeExpansion) {
	TEST(OnlyChildrenOfOpenItemsAreShown) {
		Scene scene(branchingTree(3, 2));
		CHECK(scene.expansion.isShown(scene.structure, scene.index(1)));
		CHECK(!scene.expansion.isShown(scene.structure, scene.index(2)));
		scene.expansion.setOpen(1, true);
		CHECK(scene.expansion.isShown(scene.structure, scene.index(2)));
		CHECK(!scene.expansion.isShown(scene.structure, scene.index(4)));
		CHECK_EQUAL(scene.index(2), scene.expansion.visibleAncestor(scene.structure, scene.index(4)));
	}

	TEST(LevelsAreCountedAndChangedPerSubtree) {
		Scene scene(branchingTree(4, 2));
		CHECK_EQUAL(0u, scene.expansion.openDepth(scene.structure, std::nullopt));
		scene.expansion.openTo(scene.structure, std::nullopt, 2);
		CHECK_EQUAL(2u, scene.expansion.openDepth(scene.structure, std::nullopt));
		CHECK_EQUAL(1u, scene.expansion.openDepth(scene.structure, scene.index(2)));
		CHECK_EQUAL(2u, scene.expansion.subtreeHeight(scene.structure, scene.index(2)));

		scene.expansion.setLevel(scene.structure, scene.index(2), 1, true);
		CHECK_EQUAL(2u, scene.expansion.openDepth(scene.structure, scene.index(2)));
		CHECK_EQUAL(1u, scene.expansion.openDepth(scene.structure, scene.index(3)));
		CHECK_EQUAL(1u, *scene.expansion.deepestOpenLevel(scene.structure, scene.index(2)));
		CHECK_EQUAL(2u, *scene.expansion.deepestOpenLevel(scene.structure, std::nullopt));
	}
}

SUITE(TreePacking) {
	TEST(OpenChildrenSitRightOfTheirCentredParentWithTheGaps) {
		MapTree tree;
		tree.add(1, std::nullopt);
		tree.add(2, 1);
		tree.add(3, 1);
		Scene scene(tree);
		scene.expansion.setOpen(1, true);
		const TreePacking packing = scene.pack();
		const Vec2F root = packing.centers[scene.index(1)];
		const Vec2F first = packing.centers[scene.index(2)];
		const Vec2F second = packing.centers[scene.index(3)];
		const Vec2F card = scene.metrics.card;
		CHECK_CLOSE(card.x, packing.card.x, 0.001f);
		CHECK_CLOSE(card.x + scene.metrics.columnGap, first.x - root.x, 0.001f);
		CHECK_CLOSE(card.y + scene.metrics.rowGap, second.y - first.y, 0.001f);
		CHECK_CLOSE((first.y + second.y) / 2, root.y, 0.001f);
	}

	TEST(ClosedItemsKeepTheirSubtreeAtTheirOwnPlace) {
		Scene scene(branchingTree(3, 2));
		scene.expansion.setOpen(1, true);
		const TreePacking packing = scene.pack();
		CHECK(std::abs(packing.centers[scene.index(4)].x - packing.centers[scene.index(2)].x) < 0.001f);
		CHECK(std::abs(packing.centers[scene.index(4)].y - packing.centers[scene.index(2)].y) < 0.001f);
	}

	TEST(ShownCardsNeverOverlapForAnyExpansion) {
		Scene scene(branchingTree(5, 3));
		const TreeLayoutMetrics& metrics = scene.metrics;
		for (size_t depth = 0; depth <= scene.structure.height(); depth++) {
			scene.expansion.openTo(scene.structure, std::nullopt, depth);
			scene.expansion.setOpen(scene.structure.items()[scene.index(2)].id, false);
			const TreePacking packing = scene.pack();
			std::vector<Vec2F> shown;
			for (size_t item = 0; item < scene.structure.items().size(); item++)
				if (scene.expansion.isShown(scene.structure, item)) shown.push_back(packing.centers[item]);
			for (size_t first = 0; first < shown.size(); first++)
				for (size_t second = first + 1; second < shown.size(); second++) {
					const Vec2F apart = shown[first] - shown[second];
					const bool clear = std::abs(apart.x) >= packing.card.x + metrics.columnGap - 0.5f ||
														 std::abs(apart.y) >= packing.card.y + metrics.rowGap - 0.5f;
					CHECK(clear);
				}
		}
	}
}
