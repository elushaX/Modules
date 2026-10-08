#include "SpaceDistribution.hpp"

#include <cmath>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kNegligibleSpace = 0.01f;

	halnf clampToExtent(halnf length, const Extent& extent) {
		if (length < extent.minimum) return extent.minimum;
		if (length > extent.maximum) return extent.maximum;
		return length;
	}

	halnf sum(const std::vector<halnf>& values) {
		halnf total = 0;
		for (halnf value : values)
			total += value;
		return total;
	}

	halnf growWeight(const Extent& extent, halnf length) {
		if (extent.grow <= 0 || length >= extent.maximum) return 0;
		return extent.grow;
	}

	halnf shrinkWeight(const Extent& extent, halnf length) {
		const halnf slack = length - extent.minimum;
		return slack > 0 ? slack : 0;
	}

	bool distributeOnce(const std::vector<Extent>& items, std::vector<halnf>& lengths, halnf freeSpace) {
		const bool growing = freeSpace > 0;

		std::vector<halnf> weights(items.size());
		halnf totalWeight = 0;

		for (size_t index = 0; index < items.size(); index++) {
			weights[index] = growing ? growWeight(items[index], lengths[index]) : shrinkWeight(items[index], lengths[index]);
			totalWeight += weights[index];
		}

		if (totalWeight <= 0) return false;

		for (size_t index = 0; index < items.size(); index++) {
			if (weights[index] <= 0) continue;
			const halnf share = freeSpace * weights[index] / totalWeight;
			lengths[index] = clampToExtent(lengths[index] + share, items[index]);
		}

		return true;
	}
}

std::vector<halnf> tp::ui::distributeSpace(const std::vector<Extent>& items, halnf available) {
	std::vector<halnf> lengths(items.size());
	for (size_t index = 0; index < items.size(); index++) {
		lengths[index] = clampToExtent(items[index].preferred, items[index]);
	}

	for (size_t round = 0; round <= items.size(); round++) {
		const halnf freeSpace = available - sum(lengths);
		if (std::fabs(freeSpace) < kNegligibleSpace) break;
		if (!distributeOnce(items, lengths, freeSpace)) break;
	}

	return lengths;
}

std::vector<Segment> tp::ui::placeSnapped(const std::vector<halnf>& lengths, halnf origin, halnf gap) {
	std::vector<Segment> segments(lengths.size());
	halnf cursor = origin;

	for (size_t index = 0; index < lengths.size(); index++) {
		const halnf start = std::round(cursor);
		const halnf end = std::round(cursor + lengths[index]);

		segments[index] = { start, end - start };
		cursor += lengths[index] + gap;
	}

	return segments;
}
