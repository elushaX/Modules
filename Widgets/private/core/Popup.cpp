#include "Popup.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	Placement opposite(Placement placement) {
		switch (placement) {
			case Placement::Below: return Placement::Above;
			case Placement::Above: return Placement::Below;
			case Placement::Right: return Placement::Left;
			case Placement::Left: return Placement::Right;
			case Placement::Over: return Placement::Over;
		}
		return placement;
	}

	Axis mainAxisOf(Placement placement) {
		return (placement == Placement::Left || placement == Placement::Right) ? Axis::Horizontal : Axis::Vertical;
	}

	halnf alignedStart(halnf anchorStart, halnf anchorLength, halnf popupLength, PlacementAlign align) {
		switch (align) {
			case PlacementAlign::Start: return anchorStart;
			case PlacementAlign::Center: return anchorStart + (anchorLength - popupLength) / 2;
			case PlacementAlign::End: return anchorStart + anchorLength - popupLength;
		}
		return anchorStart;
	}

	RectF placeOnSide(const RectF& anchor, Vec2F size, Placement placement, const PopupOptions& options) {
		RectF result = makeRect({ 0, 0 }, size);

		if (placement == Placement::Over) {
			result.pos = { alignedStart(anchor.pos.x, anchor.size.x, size.x, options.align), anchor.pos.y };
			return result;
		}

		const Axis main = mainAxisOf(placement);
		const Axis cross = crossAxis(main);
		const bool before = placement == Placement::Above || placement == Placement::Left;

		const halnf mainStart = before ? startAlong(anchor, main) - options.gap - along(size, main)
																	 : startAlong(anchor, main) + lengthAlong(anchor, main) + options.gap;
		const halnf crossStart =
			alignedStart(startAlong(anchor, cross), lengthAlong(anchor, cross), along(size, cross), options.align);

		along(result.pos, main) = mainStart;
		along(result.pos, cross) = crossStart;
		return result;
	}

	bool fitsAlong(const RectF& rect, const RectF& bounds, Axis axis) {
		const halnf start = startAlong(rect, axis);
		return start >= startAlong(bounds, axis) &&
					 start + lengthAlong(rect, axis) <= startAlong(bounds, axis) + lengthAlong(bounds, axis);
	}

	halnf slideInside(halnf start, halnf length, halnf boundsStart, halnf boundsLength) {
		const halnf boundsEnd = boundsStart + boundsLength;
		if (start + length > boundsEnd) start = boundsEnd - length;
		if (start < boundsStart) start = boundsStart;
		return start;
	}
}

RectF tp::ui::placePopup(const RectF& anchor, Vec2F popupSize, const RectF& bounds, const PopupOptions& options) {
	RectF placed = placeOnSide(anchor, popupSize, options.placement, options);

	if (options.placement != Placement::Over) {
		const Axis main = mainAxisOf(options.placement);
		if (!fitsAlong(placed, bounds, main)) {
			const RectF flipped = placeOnSide(anchor, popupSize, opposite(options.placement), options);
			if (fitsAlong(flipped, bounds, main)) placed = flipped;
		}
	}

	placed.pos.x = slideInside(placed.pos.x, placed.size.x, bounds.pos.x, bounds.size.x);
	placed.pos.y = slideInside(placed.pos.y, placed.size.y, bounds.pos.y, bounds.size.y);
	return placed;
}
