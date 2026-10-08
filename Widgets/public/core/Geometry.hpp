#pragma once

#include "Rect.hpp"

namespace tp::ui {

	enum class Axis { Horizontal = 0, Vertical = 1 };

	inline RectF makeRect(Vec2F position, Vec2F size) { return { position, size }; }
	inline RectF makeRect(halnf x, halnf y, halnf width, halnf height) { return { x, y, width, height }; }

	inline Axis crossAxis(Axis axis) { return axis == Axis::Horizontal ? Axis::Vertical : Axis::Horizontal; }

	inline halnf& along(Vec2F& vector, Axis axis) { return axis == Axis::Horizontal ? vector.x : vector.y; }
	inline halnf along(const Vec2F& vector, Axis axis) { return axis == Axis::Horizontal ? vector.x : vector.y; }

	inline halnf startAlong(const RectF& rect, Axis axis) { return along(rect.pos, axis); }
	inline halnf lengthAlong(const RectF& rect, Axis axis) { return along(rect.size, axis); }

	inline void setSpanAlong(RectF& rect, Axis axis, halnf start, halnf length) {
		along(rect.pos, axis) = start;
		along(rect.size, axis) = length;
	}

	struct Insets {
		halnf left = 0;
		halnf top = 0;
		halnf right = 0;
		halnf bottom = 0;

		static Insets uniform(halnf value) { return { value, value, value, value }; }
		static Insets symmetric(halnf horizontal, halnf vertical) { return { horizontal, vertical, horizontal, vertical }; }

		[[nodiscard]] halnf leading(Axis axis) const { return axis == Axis::Horizontal ? left : top; }
		[[nodiscard]] halnf trailing(Axis axis) const { return axis == Axis::Horizontal ? right : bottom; }
		[[nodiscard]] halnf total(Axis axis) const { return leading(axis) + trailing(axis); }

		[[nodiscard]] RectF shrink(const RectF& rect) const {
			const Vec2F size = { rect.size.x - left - right, rect.size.y - top - bottom };
			return makeRect({ rect.pos.x + left, rect.pos.y + top }, { size.x > 0 ? size.x : 0, size.y > 0 ? size.y : 0 });
		}
	};

	inline bool contains(const RectF& rect, Vec2F point) {
		return point.x >= rect.pos.x && point.y >= rect.pos.y && point.x < rect.pos.x + rect.size.x &&
					 point.y < rect.pos.y + rect.size.y;
	}

	inline bool overlaps(const RectF& first, const RectF& second) {
		return first.pos.x < second.pos.x + second.size.x && second.pos.x < first.pos.x + first.size.x &&
					 first.pos.y < second.pos.y + second.size.y && second.pos.y < first.pos.y + first.size.y;
	}

	inline RectF intersection(const RectF& first, const RectF& second) {
		const halnf left = first.pos.x > second.pos.x ? first.pos.x : second.pos.x;
		const halnf top = first.pos.y > second.pos.y ? first.pos.y : second.pos.y;
		const halnf firstRight = first.pos.x + first.size.x;
		const halnf secondRight = second.pos.x + second.size.x;
		const halnf firstBottom = first.pos.y + first.size.y;
		const halnf secondBottom = second.pos.y + second.size.y;
		const halnf right = firstRight < secondRight ? firstRight : secondRight;
		const halnf bottom = firstBottom < secondBottom ? firstBottom : secondBottom;

		if (right <= left || bottom <= top) return makeRect(left, top, 0, 0);
		return makeRect(left, top, right - left, bottom - top);
	}

	inline RectF translated(const RectF& rect, Vec2F offset) { return makeRect(rect.pos + offset, rect.size); }

	inline RectF insetRect(const RectF& rect, halnf amount) {
		return makeRect(rect.pos.x + amount, rect.pos.y + amount, rect.size.x - amount * 2, rect.size.y - amount * 2);
	}
}
