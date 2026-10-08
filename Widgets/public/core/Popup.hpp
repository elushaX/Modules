#pragma once

#include "Geometry.hpp"

#include <functional>
#include <optional>

namespace tp::ui {

	class Widget;

	enum class Placement { Below, Above, Right, Left, Over };
	enum class PlacementAlign { Start, Center, End };

	struct PopupOptions {
		Placement placement = Placement::Below;
		PlacementAlign align = PlacementAlign::Start;
		halnf gap = 2;

		bool closeOnOutsidePress = true;
		bool closeOnEscape = true;
		bool modal = false;
		bool interactive = true;
		std::optional<RectF> appearFrom;

		std::function<void()> onClosed;
	};

	struct PopupRequest {
		Widget* content = nullptr;
		Widget* owner = nullptr;
		RectF anchor;
		PopupOptions options;
	};

	RectF placePopup(const RectF& anchor, Vec2F popupSize, const RectF& bounds, const PopupOptions& options);
}
