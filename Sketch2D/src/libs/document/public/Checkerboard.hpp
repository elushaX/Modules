#pragma once

#include "Document.hpp"

#include <vector>

namespace tp::sketch2d {

	struct CheckerCell {
		Vec2F position;
		Vec2F size;
		RGBA color;
	};

	constexpr halnf kCheckerCellPixels = 8;
	inline const RGBA kCheckerLight = { 0.80f, 0.80f, 0.80f, 1.0f };
	inline const RGBA kCheckerDark = { 0.62f, 0.62f, 0.62f, 1.0f };

	[[nodiscard]] std::vector<CheckerCell> checkerCells(const Document& document, Vec2F viewport);
}
