#pragma once

#include "Vec.hpp"

#include <cstdint>

namespace tp {

	enum class PointerTool { Mouse, Pen, Eraser, Brush, Pencil, Airbrush, Finger, Lens };

	struct PointerSample {
		PointerTool tool = PointerTool::Mouse;
		halnf pressure = 1;
		Vec2F tilt = { 0, 0 };
		halnf rotation = 0;
		halnf distance = 0;
		uint32_t timeMs = 0;

		[[nodiscard]] bool isStylus() const { return tool != PointerTool::Mouse && tool != PointerTool::Finger; }
		[[nodiscard]] bool operator==(const PointerSample& other) const = default;
	};
}
