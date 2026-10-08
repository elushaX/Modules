#include "BoxLayout.hpp"

#include "SpaceDistribution.hpp"

using namespace tp;
using namespace tp::ui;

BoxLayout::BoxLayout(Axis axis, halnf gap) :
	mAxis(axis),
	mGap(gap) {}

void BoxLayout::setGap(halnf gap) {
	mGap = gap;
	invalidate();
}

void BoxLayout::setCrossAlign(CrossAlign align) {
	mCrossAlign = align;
	invalidate();
}

void BoxLayout::setMainAlign(MainAlign align) {
	mMainAlign = align;
	invalidate();
}

Extent BoxLayout::measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const {
	return axis == mAxis ? measureMainAxis(context, crossLength) : measureCrossAxis(context, crossLength);
}

Extent BoxLayout::measureMainAxis(const LayoutContext& context, halnf crossLength) const {
	Extent total = { 0, 0, 0, 0 };

	for (const Widget* child : context.children()) {
		const Extent childExtent = context.measure(*child, mAxis, crossLength);
		total.minimum += childExtent.minimum;
		total.preferred += childExtent.preferred;
		total.maximum += childExtent.maximum;
		total.grow += childExtent.grow;
	}

	const halnf gaps = totalGap(context);
	total.minimum += gaps;
	total.preferred += gaps;
	total.maximum = total.maximum + gaps < kUnbounded ? total.maximum + gaps : kUnbounded;
	return total;
}

Extent BoxLayout::measureCrossAxis(const LayoutContext& context, halnf mainLength) const {
	const Axis cross = crossAxis(mAxis);
	Extent total = { 0, 0, kUnbounded, 0 };

	std::vector<Extent> mainExtents;
	for (const Widget* child : context.children())
		mainExtents.push_back(context.measure(*child, mAxis, kUnbounded));

	const bool mainLengthKnown = mainLength < kUnbounded;
	const std::vector<halnf> mainLengths =
		mainLengthKnown ? distributeSpace(mainExtents, mainLength - totalGap(context)) : std::vector<halnf>();

	for (size_t index = 0; index < context.children().size(); index++) {
		const halnf childMain = mainLengthKnown ? mainLengths[index] : mainExtents[index].preferred;
		const Extent childCross = context.measure(*context.children()[index], cross, childMain);

		if (childCross.minimum > total.minimum) total.minimum = childCross.minimum;
		if (childCross.preferred > total.preferred) total.preferred = childCross.preferred;
		if (childCross.grow > total.grow) total.grow = childCross.grow;
	}

	return total;
}

void BoxLayout::arrangeChildren(LayoutContext& context, const RectF& contentArea) {
	const auto& children = context.children();
	if (children.empty()) return;

	const Axis cross = crossAxis(mAxis);
	const halnf available = lengthAlong(contentArea, mAxis) - totalGap(context);
	const halnf crossAvailable = lengthAlong(contentArea, cross);

	std::vector<Extent> mainExtents;
	mainExtents.reserve(children.size());
	for (const Widget* child : children)
		mainExtents.push_back(context.measure(*child, mAxis, crossAvailable));

	const std::vector<halnf> lengths = distributeSpace(mainExtents, available);

	halnf used = 0;
	for (halnf length : lengths)
		used += length;
	const halnf leftover = available - used > 0 ? available - used : 0;

	halnf start = startAlong(contentArea, mAxis);
	halnf gap = mGap;
	switch (mMainAlign) {
		case MainAlign::Start: break;
		case MainAlign::Center: start += leftover / 2; break;
		case MainAlign::End: start += leftover; break;
		case MainAlign::SpaceBetween:
			if (children.size() > 1) gap += leftover / (halnf) (children.size() - 1);
			break;
	}

	const std::vector<Segment> segments = placeSnapped(lengths, start, gap);

	for (size_t index = 0; index < children.size(); index++) {
		const Extent crossExtent = context.measure(*children[index], cross, segments[index].length);
		const halnf crossLength = crossLengthFor(crossExtent, crossAvailable);

		RectF rect;
		setSpanAlong(rect, mAxis, segments[index].start, segments[index].length);
		setSpanAlong(rect, cross, startAlong(contentArea, cross) + crossStartFor(crossLength, crossAvailable), crossLength);
		context.place(*children[index], rect);
	}
}

halnf BoxLayout::totalGap(const LayoutContext& context) const {
	const size_t count = context.children().size();
	return count > 1 ? mGap * (halnf) (count - 1) : 0;
}

halnf BoxLayout::crossLengthFor(const Extent& extent, halnf available) const {
	const bool stretches = mCrossAlign == CrossAlign::Stretch || extent.grow > 0;
	halnf length = stretches ? available : extent.preferred;

	if (length > extent.maximum) length = extent.maximum;
	if (length < extent.minimum) length = extent.minimum;
	if (length > available) length = available;
	return length;
}

halnf BoxLayout::crossStartFor(halnf length, halnf available) const {
	const halnf leftover = available - length;
	switch (mCrossAlign) {
		case CrossAlign::Stretch:
		case CrossAlign::Start: return 0;
		case CrossAlign::Center: return leftover / 2;
		case CrossAlign::End: return leftover;
	}
	return 0;
}
