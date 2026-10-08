#pragma once

#include "Sizing.hpp"

#include <vector>

namespace tp::ui {

	std::vector<halnf> distributeSpace(const std::vector<Extent>& items, halnf available);

	struct Segment {
		halnf start = 0;
		halnf length = 0;
	};

	std::vector<Segment> placeSnapped(const std::vector<halnf>& lengths, halnf origin, halnf gap);
}
