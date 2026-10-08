#include "Checkerboard.hpp"

#include <cmath>

namespace tp::sketch2d {

	std::vector<CheckerCell> checkerCells(const Document& document, Vec2F viewport) {
		std::vector<CheckerCell> cells;
		const Vec2F origin = document.worldToScreen({ 0, 0 });
		const halnf firstX = std::fmod(origin.x, kCheckerCellPixels * 2) - kCheckerCellPixels * 2;
		const halnf firstY = std::fmod(origin.y, kCheckerCellPixels * 2) - kCheckerCellPixels * 2;

		cells.push_back({ { 0, 0 }, viewport, kCheckerLight });
		for (halnf y = firstY; y < viewport.y; y += kCheckerCellPixels) {
			const int4 row = (int4) std::lround((y - firstY) / kCheckerCellPixels);
			for (halnf x = firstX + (row % 2 == 0 ? kCheckerCellPixels : 0); x < viewport.x; x += kCheckerCellPixels * 2)
				cells.push_back({ { x, y }, { kCheckerCellPixels, kCheckerCellPixels }, kCheckerDark });
		}
		return cells;
	}
}
