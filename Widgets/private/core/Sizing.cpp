#include "Sizing.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	halnf clampLength(halnf value, halnf minimum, halnf maximum) {
		if (value < minimum) return minimum;
		if (value > maximum) return maximum;
		return value;
	}
}

AxisSizing::AxisSizing(Mode mode, halnf length, halnf weight) :
	mMode(mode),
	mLength(length),
	mWeight(weight) {}

AxisSizing AxisSizing::atLeast(halnf minimum) const {
	AxisSizing copy = *this;
	copy.mMinimum = minimum;
	return copy;
}

AxisSizing AxisSizing::atMost(halnf maximum) const {
	AxisSizing copy = *this;
	copy.mMaximum = maximum;
	return copy;
}

Extent AxisSizing::resolve(const Extent& content) const {
	Extent result = content;

	switch (mMode) {
		case Mode::Fixed: result = Extent::rigid(mLength); break;

		case Mode::Preferred:
			result.preferred = mLength;
			result.maximum = mLength;
			result.grow = 0;
			break;

		case Mode::Fit: result.grow = 0; break;

		case Mode::Fill:
			result.grow = mWeight;
			result.maximum = kUnbounded;
			break;
	}

	const halnf lowerBound = mMinimum > result.minimum ? mMinimum : result.minimum;
	const halnf upperBound = mMaximum < result.maximum ? mMaximum : result.maximum;

	result.minimum = clampLength(lowerBound, 0, mMaximum);
	result.maximum = upperBound < result.minimum ? result.minimum : upperBound;
	result.preferred = clampLength(result.preferred, result.minimum, result.maximum);

	return result;
}

bool AxisSizing::operator==(const AxisSizing& other) const {
	return mMode == other.mMode && mLength == other.mLength && mWeight == other.mWeight && mMinimum == other.mMinimum &&
				 mMaximum == other.mMaximum;
}
