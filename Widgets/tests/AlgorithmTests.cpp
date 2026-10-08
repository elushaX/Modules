#include "Animation.hpp"
#include "Popup.hpp"
#include "SpaceDistribution.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr float kTolerance = 0.05f;

	Extent grower(halnf preferred, halnf weight = 1, halnf minimum = 0, halnf maximum = kUnbounded) {
		return { minimum, preferred, maximum, weight };
	}
}

SUITE(AxisSizingResolution) {
	TEST(FixedIgnoresContent) {
		const Extent resolved = AxisSizing::fixed(120).resolve(Extent::shrinkable(40));
		CHECK_CLOSE(120, resolved.minimum, kTolerance);
		CHECK_CLOSE(120, resolved.preferred, kTolerance);
		CHECK_CLOSE(120, resolved.maximum, kTolerance);
		CHECK_CLOSE(0, resolved.grow, kTolerance);
	}

	TEST(FitKeepsContentPreferenceWithoutGrowing) {
		const Extent resolved = AxisSizing::fit().resolve(Extent::shrinkable(40));
		CHECK_CLOSE(40, resolved.preferred, kTolerance);
		CHECK_CLOSE(0, resolved.grow, kTolerance);
	}

	TEST(PreferredNeverDropsBelowContentMinimum) {
		const Extent roomy = AxisSizing::preferred(200).resolve(Extent::shrinkable(40, 30));
		CHECK_CLOSE(30, roomy.minimum, kTolerance);
		CHECK_CLOSE(200, roomy.preferred, kTolerance);
		CHECK_CLOSE(200, roomy.maximum, kTolerance);

		const Extent tight = AxisSizing::preferred(10).resolve(Extent::shrinkable(40, 30));
		CHECK_CLOSE(30, tight.preferred, kTolerance);
		CHECK_CLOSE(30, tight.maximum, kTolerance);
	}

	TEST(FillGrowsWithWeight) {
		const Extent resolved = AxisSizing::fill(3).resolve(Extent::shrinkable(40));
		CHECK_CLOSE(40, resolved.preferred, kTolerance);
		CHECK_CLOSE(3, resolved.grow, kTolerance);
	}

	TEST(BoundsClampPreferenceAndRange) {
		const Extent resolved = AxisSizing::fill().atLeast(50).atMost(80).resolve(Extent::shrinkable(10));
		CHECK_CLOSE(50, resolved.minimum, kTolerance);
		CHECK_CLOSE(50, resolved.preferred, kTolerance);
		CHECK_CLOSE(80, resolved.maximum, kTolerance);
	}

	TEST(MinimumAboveMaximumWins) {
		const Extent resolved = AxisSizing::fit().atLeast(90).atMost(40).resolve(Extent::shrinkable(10));
		CHECK_CLOSE(40, resolved.minimum, kTolerance);
		CHECK(resolved.maximum >= resolved.minimum);
	}
}

SUITE(SpaceDistribution) {
	TEST(RigidItemsKeepPreferredSizes) {
		const auto lengths = distributeSpace({ Extent::rigid(10), Extent::rigid(20) }, 100);
		CHECK_CLOSE(10, lengths[0], kTolerance);
		CHECK_CLOSE(20, lengths[1], kTolerance);
	}

	TEST(EqualWeightsShareFreeSpaceEqually) {
		const auto lengths = distributeSpace({ grower(0), grower(0) }, 100);
		CHECK_CLOSE(50, lengths[0], kTolerance);
		CHECK_CLOSE(50, lengths[1], kTolerance);
	}

	TEST(WeightsSplitProportionally) {
		const auto lengths = distributeSpace({ grower(0, 1), grower(0, 3) }, 100);
		CHECK_CLOSE(25, lengths[0], kTolerance);
		CHECK_CLOSE(75, lengths[1], kTolerance);
	}

	TEST(GrowingStartsFromPreferredSizes) {
		const auto lengths = distributeSpace({ grower(20), grower(40) }, 100);
		CHECK_CLOSE(40, lengths[0], kTolerance);
		CHECK_CLOSE(60, lengths[1], kTolerance);
	}

	TEST(FrozenItemGivesSpaceToOthers) {
		const auto lengths = distributeSpace({ grower(0, 1, 0, 10), grower(0) }, 100);
		CHECK_CLOSE(10, lengths[0], kTolerance);
		CHECK_CLOSE(90, lengths[1], kTolerance);
	}

	TEST(RigidPlusGrowerFillsRemainder) {
		const auto lengths = distributeSpace({ Extent::rigid(30), grower(0) }, 100);
		CHECK_CLOSE(30, lengths[0], kTolerance);
		CHECK_CLOSE(70, lengths[1], kTolerance);
	}

	TEST(ShrinkingIsProportionalToSlackAboveMinimum) {
		const auto lengths = distributeSpace({ grower(60, 0, 50), grower(60, 0, 0) }, 80);
		const halnf deficit = 40;
		const halnf totalSlack = 10 + 60;
		CHECK_CLOSE(60 - deficit * 10 / totalSlack, lengths[0], kTolerance);
		CHECK_CLOSE(60 - deficit * 60 / totalSlack, lengths[1], kTolerance);
		CHECK(lengths[0] >= 50);
	}

	TEST(OverflowStopsAtMinimums) {
		const auto lengths = distributeSpace({ grower(40, 0, 40), grower(40, 0, 40) }, 50);
		CHECK_CLOSE(40, lengths[0], kTolerance);
		CHECK_CLOSE(40, lengths[1], kTolerance);
	}

	TEST(FractionalSpaceTerminates) {
		std::vector<Extent> items(7, grower(0, 1, 0, 13.37f));
		const auto lengths = distributeSpace(items, 333.333f);
		for (halnf length : lengths)
			CHECK_CLOSE(13.37f, length, kTolerance);
	}

	TEST(EmptyInputReturnsEmpty) { CHECK(distributeSpace({}, 100).empty()); }
}

SUITE(PixelSnapping) {
	TEST(EdgesAreRoundedSoSegmentsTile) {
		const auto segments = placeSnapped({ 33.3f, 33.3f, 33.4f }, 0, 0);
		CHECK_EQUAL(0, segments[0].start);
		CHECK_EQUAL(segments[0].start + segments[0].length, segments[1].start);
		CHECK_EQUAL(segments[1].start + segments[1].length, segments[2].start);
		CHECK_EQUAL(100, segments[2].start + segments[2].length);
	}

	TEST(GapsSeparateSegments) {
		const auto segments = placeSnapped({ 10, 10 }, 5, 4);
		CHECK_EQUAL(5, segments[0].start);
		CHECK_EQUAL(19, segments[1].start);
	}
}

SUITE(PopupPlacement) {
	const RectF kScreen = makeRect(0, 0, 400, 300);
	const RectF kAnchor = makeRect(100, 100, 80, 20);

	TEST(BelowAlignedToStart) {
		PopupOptions options;
		options.gap = 2;
		const RectF placed = placePopup(kAnchor, { 120, 50 }, kScreen, options);
		CHECK_CLOSE(100, placed.pos.x, kTolerance);
		CHECK_CLOSE(122, placed.pos.y, kTolerance);
	}

	TEST(RightOfAnchor) {
		PopupOptions options;
		options.placement = Placement::Right;
		options.gap = 0;
		const RectF placed = placePopup(kAnchor, { 50, 50 }, kScreen, options);
		CHECK_CLOSE(180, placed.pos.x, kTolerance);
		CHECK_CLOSE(100, placed.pos.y, kTolerance);
	}

	TEST(CenterAlignment) {
		PopupOptions options;
		options.align = PlacementAlign::Center;
		options.gap = 0;
		const RectF placed = placePopup(kAnchor, { 40, 10 }, kScreen, options);
		CHECK_CLOSE(120, placed.pos.x, kTolerance);
	}

	TEST(FlipsAboveWhenNoRoomBelow) {
		PopupOptions options;
		options.gap = 0;
		const RectF lowAnchor = makeRect(100, 270, 80, 20);
		const RectF placed = placePopup(lowAnchor, { 100, 100 }, kScreen, options);
		CHECK_CLOSE(170, placed.pos.y, kTolerance);
	}

	TEST(FlipsLeftWhenNoRoomRight) {
		PopupOptions options;
		options.placement = Placement::Right;
		options.gap = 0;
		const RectF edgeAnchor = makeRect(350, 50, 40, 20);
		const RectF placed = placePopup(edgeAnchor, { 100, 50 }, kScreen, options);
		CHECK_CLOSE(250, placed.pos.x, kTolerance);
	}

	TEST(SlidesInsideScreenOnCrossAxis) {
		PopupOptions options;
		options.gap = 0;
		const RectF edgeAnchor = makeRect(380, 10, 20, 20);
		const RectF placed = placePopup(edgeAnchor, { 100, 50 }, kScreen, options);
		CHECK_CLOSE(300, placed.pos.x, kTolerance);
	}

	TEST(OversizedPopupPinnedToOrigin) {
		PopupOptions options;
		const RectF placed = placePopup(kAnchor, { 1000, 1000 }, kScreen, options);
		CHECK_CLOSE(0, placed.pos.x, kTolerance);
		CHECK_CLOSE(0, placed.pos.y, kTolerance);
	}
}

SUITE(SpringAnimation) {
	TEST(ConvergesToTarget) {
		AnimatedFloat value(0, Motion::quick());
		value.animateTo(100);

		int frames = 0;
		while (value.advance(16) && frames < 1000)
			frames++;

		CHECK_CLOSE(100, value.value(), 0.01f);
		CHECK(frames < 120);
		CHECK(!value.isAnimating());
	}

	TEST(MovesTowardTargetMonotonicallyAtFirst) {
		AnimatedFloat value(0, Motion::smooth());
		value.animateTo(10);
		value.advance(16);
		const halnf first = value.value();
		value.advance(16);
		CHECK(first > 0);
		CHECK(value.value() > first);
	}

	TEST(JumpAndFinishAreImmediate) {
		AnimatedRect rect(makeRect(0, 0, 10, 10));
		rect.animateTo(makeRect(50, 50, 20, 20));
		CHECK(rect.isAnimating());

		rect.finish();
		CHECK(!rect.isAnimating());
		CHECK(rect.value() == makeRect(50, 50, 20, 20));

		rect.jumpTo(makeRect(1, 2, 3, 4));
		CHECK(rect.value() == makeRect(1, 2, 3, 4));
	}

	TEST(ColorChannelsAnimateTogether) {
		AnimatedColor color(RGBA(0, 0, 0, 1));
		color.animateTo(RGBA(1, 0.5f, 0, 1));
		while (color.advance(16)) {
		}
		CHECK_CLOSE(1, color.value().r, 0.01f);
		CHECK_CLOSE(0.5f, color.value().g, 0.01f);
	}

	TEST(LargeTimeStepIsStable) {
		AnimatedFloat value(0, Motion::quick());
		value.animateTo(1);
		value.advance(5000);
		CHECK_CLOSE(1, value.value(), 0.01f);
	}
}
