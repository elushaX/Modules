
#pragma once

#include "Common.hpp"

namespace tp {
	template <typename Type>
	class Range {
	public:
		Range() = default;
		Range(Type v1, Type v2) : start(v1), end(v2) {}

		Type mid() const { return (start + end) / 2.f; }
		Type size() const { return (end - start); }

		void resizeFromCenter(Type newSize) {
			const auto middle = mid();
			const auto len = newSize / 2;
			start = middle - len;
			end = middle + len;
		}

		void clamp(const Range& inside, const Range& outside) {
			start = tp::clamp(start, outside.start, inside.start);
			end = tp::clamp(end, inside.end, outside.end);
		}

		void clamp(const Range& outside) {
			start = tp::clamp(start, outside.start, outside.end);
			end = tp::clamp(end, outside.start, outside.end);
		}

	public:
		Type start{};
		Type end{};
	};

	using RangeF = Range<halnf>;
}