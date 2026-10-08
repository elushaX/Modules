#include "TreeLayout.hpp"

#include <algorithm>
#include <functional>

using namespace tp;
using namespace tp::ui;

bool TreeModel::canReparent(TreeItemId child, TreeItemId parent) const {
	if (child == parent) return false;

	std::vector<TreeItemId> pending = children(child);
	std::unordered_set<TreeItemId> seen;
	while (!pending.empty()) {
		const TreeItemId current = pending.back();
		pending.pop_back();
		if (current == parent) return false;
		if (!seen.insert(current).second) continue;
		for (TreeItemId next : children(current))
			pending.push_back(next);
	}
	return true;
}

void TreeStructure::rebuild(const TreeModel& model) {
	mItems.clear();
	mRoots.clear();
	mIndex.clear();
	mHeight = 0;
	for (TreeItemId root : model.roots()) {
		if (mIndex.contains(root)) continue;
		mRoots.push_back(add(model, root, kNone, 0));
	}
}

size_t TreeStructure::add(const TreeModel& model, TreeItemId id, size_t parent, size_t depth) {
	const size_t index = mItems.size();
	mItems.push_back({ id, parent, depth, {}, 0 });
	mIndex[id] = index;
	mHeight = std::max(mHeight, depth);

	for (TreeItemId child : model.children(id)) {
		if (mIndex.contains(child)) continue;
		const size_t added = add(model, child, index, depth + 1);
		mItems[index].children.push_back(added);
		mItems[index].descendants += 1 + mItems[added].descendants;
	}
	return index;
}

std::optional<size_t> TreeStructure::indexOf(TreeItemId id) const {
	const auto found = mIndex.find(id);
	if (found == mIndex.end()) return std::nullopt;
	return found->second;
}

size_t TreeStructure::ancestorAtDepth(size_t item, size_t depth) const {
	while (mItems[item].depth > depth)
		item = mItems[item].parent;
	return item;
}

bool TreeStructure::isAncestorOrSelf(size_t ancestor, size_t item) const {
	for (size_t current = item; current != kNone; current = mItems[current].parent)
		if (current == ancestor) return true;
	return false;
}

void TreeExpansion::setOpen(TreeItemId id, bool open) {
	if (open) mOpen.insert(id);
	else mOpen.erase(id);
}

void TreeExpansion::retain(const TreeStructure& structure) {
	std::erase_if(mOpen, [&](TreeItemId id) { return !structure.indexOf(id); });
}

bool TreeExpansion::isShown(const TreeStructure& structure, size_t item) const {
	for (size_t parent = structure.items()[item].parent; parent != TreeStructure::kNone;
			 parent = structure.items()[parent].parent)
		if (!isOpen(structure.items()[parent].id)) return false;
	return true;
}

size_t TreeExpansion::visibleAncestor(const TreeStructure& structure, size_t item) const {
	size_t visible = item;
	for (size_t parent = structure.items()[item].parent; parent != TreeStructure::kNone;
			 parent = structure.items()[parent].parent)
		if (!isOpen(structure.items()[parent].id)) visible = parent;
	return visible;
}

size_t TreeExpansion::subtreeHeight(const TreeStructure& structure, std::optional<size_t> root) const {
	size_t height = 0;
	const auto walk = [&](const auto& self, size_t item, size_t level) -> void {
		height = std::max(height, level);
		for (size_t child : structure.items()[item].children)
			self(self, child, level + 1);
	};
	if (root) walk(walk, *root, 0);
	else
		for (size_t top : structure.roots())
			walk(walk, top, 0);
	return height;
}

size_t TreeExpansion::openDepth(const TreeStructure& structure, std::optional<size_t> root) const {
	size_t depth = std::numeric_limits<size_t>::max();
	const auto walk = [&](const auto& self, size_t item, size_t level) -> void {
		const TreeStructure::Item& entry = structure.items()[item];
		if (entry.children.empty()) return;
		if (!isOpen(entry.id)) {
			depth = std::min(depth, level);
			return;
		}
		for (size_t child : entry.children)
			self(self, child, level + 1);
	};
	if (root) walk(walk, *root, 0);
	else
		for (size_t top : structure.roots())
			walk(walk, top, 0);
	return depth == std::numeric_limits<size_t>::max() ? subtreeHeight(structure, root) : depth;
}

void TreeExpansion::openTo(const TreeStructure& structure, std::optional<size_t> root, size_t depth) {
	const auto walk = [&](const auto& self, size_t item, size_t level) -> void {
		const TreeStructure::Item& entry = structure.items()[item];
		if (entry.children.empty()) return;
		setOpen(entry.id, level < depth);
		for (size_t child : entry.children)
			self(self, child, level + 1);
	};
	if (root) walk(walk, *root, 0);
	else
		for (size_t top : structure.roots())
			walk(walk, top, 0);
}

std::optional<size_t>
TreeExpansion::deepestOpenLevel(const TreeStructure& structure, std::optional<size_t> root) const {
	std::optional<size_t> deepest;
	const auto walk = [&](const auto& self, size_t item, size_t level) -> void {
		const TreeStructure::Item& entry = structure.items()[item];
		if (entry.children.empty() || !isOpen(entry.id)) return;
		deepest = std::max(deepest.value_or(0), level);
		for (size_t child : entry.children)
			self(self, child, level + 1);
	};
	if (root) walk(walk, *root, 0);
	else
		for (size_t top : structure.roots())
			walk(walk, top, 0);
	return deepest;
}

void TreeExpansion::setLevel(const TreeStructure& structure, std::optional<size_t> root, size_t level, bool open) {
	const auto walk = [&](const auto& self, size_t item, size_t at) -> void {
		const TreeStructure::Item& entry = structure.items()[item];
		if (entry.children.empty()) return;
		if (at == level) {
			setOpen(entry.id, open);
			return;
		}
		for (size_t child : entry.children)
			self(self, child, at + 1);
	};
	if (root) walk(walk, *root, 0);
	else
		for (size_t top : structure.roots())
			walk(walk, top, 0);
}

TreePacking
TreePacking::pack(const TreeStructure& structure, const TreeExpansion& expansion, const TreeLayoutMetrics& metrics) {
	const std::vector<TreeStructure::Item>& items = structure.items();
	TreePacking packing;
	packing.card = metrics.card;
	packing.centers.resize(items.size());

	const halnf rowPitch = packing.card.y + metrics.rowGap;
	const halnf columnPitch = packing.card.x + metrics.columnGap;
	halnf cursor = 0;
	size_t deepest = 0;
	const std::function<halnf(size_t)> place = [&](size_t index) {
		const TreeStructure::Item& item = items[index];
		deepest = std::max(deepest, item.depth);
		halnf y;
		if (item.children.empty() || !expansion.isOpen(item.id)) {
			y = cursor + packing.card.y / 2;
			cursor += rowPitch;
		} else {
			const halnf first = place(item.children.front());
			halnf last = first;
			for (size_t position = 1; position < item.children.size(); position++)
				last = place(item.children[position]);
			y = (first + last) / 2;
		}
		packing.centers[index] = { halnf(item.depth) * columnPitch + packing.card.x / 2, y };
		return y;
	};
	for (size_t position = 0; position < structure.roots().size(); position++) {
		if (position > 0) cursor += metrics.rootGap - metrics.rowGap;
		place(structure.roots()[position]);
	}
	for (size_t index = 0; index < items.size(); index++) {
		const size_t visible = expansion.visibleAncestor(structure, index);
		if (visible != index) packing.centers[index] = packing.centers[visible];
	}
	packing.extent = { halnf(deepest) * columnPitch + packing.card.x, std::max(0.0f, cursor - metrics.rowGap) };
	return packing;
}
