#pragma once

#include "Geometry.hpp"

namespace tp::ui {

	constexpr halnf kUnbounded = 1.0e9f;

	struct Extent {
		halnf minimum = 0;
		halnf preferred = 0;
		halnf maximum = kUnbounded;
		halnf grow = 0;

		static Extent rigid(halnf length) { return { length, length, length, 0 }; }
		static Extent natural(halnf length) { return { length, length, kUnbounded, 0 }; }
		static Extent shrinkable(halnf preferred, halnf minimum = 0) { return { minimum, preferred, kUnbounded, 0 }; }
	};

	class AxisSizing {
	public:
		enum class Mode { Fit, Fixed, Preferred, Fill };

		static AxisSizing fit() { return AxisSizing(Mode::Fit, 0, 0); }
		static AxisSizing fixed(halnf length) { return AxisSizing(Mode::Fixed, length, 0); }
		static AxisSizing preferred(halnf length) { return AxisSizing(Mode::Preferred, length, 0); }
		static AxisSizing fill(halnf weight = 1) { return AxisSizing(Mode::Fill, 0, weight); }

		[[nodiscard]] AxisSizing atLeast(halnf minimum) const;
		[[nodiscard]] AxisSizing atMost(halnf maximum) const;

		[[nodiscard]] Mode mode() const { return mMode; }
		[[nodiscard]] halnf fixedLength() const { return mLength; }
		[[nodiscard]] halnf fillWeight() const { return mWeight; }
		[[nodiscard]] halnf minimum() const { return mMinimum; }
		[[nodiscard]] halnf maximum() const { return mMaximum; }

		[[nodiscard]] Extent resolve(const Extent& content) const;

		bool operator==(const AxisSizing& other) const;

	private:
		AxisSizing(Mode mode, halnf length, halnf weight);

		Mode mMode;
		halnf mLength;
		halnf mWeight;
		halnf mMinimum = 0;
		halnf mMaximum = kUnbounded;
	};

	struct Sizing {
		AxisSizing horizontal = AxisSizing::fit();
		AxisSizing vertical = AxisSizing::fit();

		[[nodiscard]] const AxisSizing& along(Axis axis) const { return axis == Axis::Horizontal ? horizontal : vertical; }

		AxisSizing& along(Axis axis) { return axis == Axis::Horizontal ? horizontal : vertical; }
	};
}
