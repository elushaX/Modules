#pragma once

#include "Geometry.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tp::ui {

	using TreeItemId = uint64_t;

	class TreeModel {
	public:
		virtual ~TreeModel() = default;

		[[nodiscard]] virtual std::vector<TreeItemId> roots() const = 0;
		[[nodiscard]] virtual std::vector<TreeItemId> children(TreeItemId id) const = 0;
		[[nodiscard]] virtual std::vector<std::pair<TreeItemId, TreeItemId>> crossLinks() const { return {}; }
		[[nodiscard]] virtual bool canReparent(TreeItemId child, TreeItemId parent) const;
	};

	class TreeStructure {
	public:
		static constexpr size_t kNone = std::numeric_limits<size_t>::max();

		struct Item {
			TreeItemId id = 0;
			size_t parent = kNone;
			size_t depth = 0;
			std::vector<size_t> children;
			size_t descendants = 0;
		};

		void rebuild(const TreeModel& model);

		[[nodiscard]] const std::vector<Item>& items() const { return mItems; }
		[[nodiscard]] const std::vector<size_t>& roots() const { return mRoots; }
		[[nodiscard]] std::optional<size_t> indexOf(TreeItemId id) const;
		[[nodiscard]] size_t height() const { return mHeight; }
		[[nodiscard]] bool empty() const { return mItems.empty(); }
		[[nodiscard]] size_t ancestorAtDepth(size_t item, size_t depth) const;
		[[nodiscard]] bool isAncestorOrSelf(size_t ancestor, size_t item) const;

	private:
		size_t add(const TreeModel& model, TreeItemId id, size_t parent, size_t depth);

		std::vector<Item> mItems;
		std::vector<size_t> mRoots;
		std::unordered_map<TreeItemId, size_t> mIndex;
		size_t mHeight = 0;
	};

	enum class TreeItemState { Shown, Folded };

	struct TreeLayoutMetrics {
		Vec2F card = { 220, 120 };
		halnf columnGap = 44;
		halnf rowGap = 16;
		halnf rootGap = 32;
	};

	// Which items are open: an open item shows its children. An item is shown when all of its ancestors are open.
	class TreeExpansion {
	public:
		[[nodiscard]] bool isOpen(TreeItemId id) const { return mOpen.contains(id); }
		void setOpen(TreeItemId id, bool open);
		void clear() { mOpen.clear(); }
		void retain(const TreeStructure& structure);
		[[nodiscard]] bool isShown(const TreeStructure& structure, size_t item) const;
		[[nodiscard]] size_t visibleAncestor(const TreeStructure& structure, size_t item) const;
		[[nodiscard]] bool operator==(const TreeExpansion& other) const = default;

		// Levels below `root` (or the whole forest) that are open everywhere, counting from root's own depth.
		[[nodiscard]] size_t openDepth(const TreeStructure& structure, std::optional<size_t> root) const;
		void openTo(const TreeStructure& structure, std::optional<size_t> root, size_t depth);
		[[nodiscard]] std::optional<size_t>
		deepestOpenLevel(const TreeStructure& structure, std::optional<size_t> root) const;
		void setLevel(const TreeStructure& structure, std::optional<size_t> root, size_t level, bool open);
		[[nodiscard]] size_t subtreeHeight(const TreeStructure& structure, std::optional<size_t> root) const;

	private:
		std::unordered_set<TreeItemId> mOpen;
	};

	// Packs the shown items of a structure: shown children sit in a column right of their parent, centred on it, with
	// the metrics' gaps. Hidden items sit at their nearest shown ancestor.
	struct TreePacking {
		Vec2F card = { 0, 0 };
		Vec2F extent = { 0, 0 };
		std::vector<Vec2F> centers;

		static TreePacking
		pack(const TreeStructure& structure, const TreeExpansion& expansion, const TreeLayoutMetrics& metrics);
	};
}
