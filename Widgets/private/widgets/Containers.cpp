#include "Containers.hpp"
#include "ScrollPhysics.hpp"

#include "Event.hpp"
#include "Layout.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <cmath>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kShowAfterScrollMs = 700;
	halnf clampLength(halnf value, halnf minimum, halnf maximum) {
		if (value < minimum) return minimum;
		if (value > maximum) return maximum;
		return value;
	}
}

Panel::Panel(Surface surface) :
	mSurface(surface) {
	setDebugName("panel");
	setClipsChildren(surface != Surface::None);
}

void Panel::setSurface(Surface surface) {
	mSurface = surface;
	setClipsChildren(surface != Surface::None);
	requestPaint();
}

void Panel::setCornerRadius(halnf radius) {
	mCornerRadius = radius;
	requestPaint();
}

void Panel::setBorder(bool border) {
	mBorder = border;
	requestPaint();
}

void Panel::paint(Painter& painter) {
	const Theme& style = theme();
	const halnf radius = mCornerRadius >= 0 ? mCornerRadius : style.metrics.cornerRadius;

	switch (mSurface) {
		case Surface::None: break;
		case Surface::Window:
			painter.fillRect(localRect(), style.palette.window, mCornerRadius >= 0 ? mCornerRadius : 0);
			break;
		case Surface::Base: painter.fillRect(localRect(), style.palette.surface, radius); break;
		case Surface::Raised: painter.fillRect(localRect(), style.palette.raisedSurface, radius); break;
	}

	if (mBorder) painter.strokeRect(localRect(), style.palette.border, radius);
}

halnf Panel::effectiveCornerRadius() const { return mCornerRadius >= 0 ? mCornerRadius : theme().metrics.cornerRadius; }

Separator::Separator(Axis lineAxis) :
	mLineAxis(lineAxis) {
	setDebugName("separator");
	setSizing(mLineAxis, AxisSizing::fill());
}

Extent Separator::measureContent(Axis axis, halnf) const {
	const halnf thickness = theme().metrics.separatorThickness + theme().metrics.spacing;
	if (axis == mLineAxis) return Extent::shrinkable(thickness);
	return Extent::rigid(thickness);
}

void Separator::paint(Painter& painter) {
	const Theme& style = theme();
	const Vec2F middle = size() / 2.0f;

	if (mLineAxis == Axis::Horizontal) {
		painter.line(
			Vec2F(0.0f, middle.y), Vec2F(size().x, middle.y), style.palette.separator, style.metrics.separatorThickness
		);
	} else {
		painter.line(
			Vec2F(middle.x, 0.0f), Vec2F(middle.x, size().y), style.palette.separator, style.metrics.separatorThickness
		);
	}
}

Spacer::Spacer() {
	setDebugName("spacer");
	setSizing(AxisSizing::fill(), AxisSizing::fill());
}

Spacer::Spacer(halnf length) {
	setDebugName("spacer");
	setSizing(AxisSizing::fixed(length), AxisSizing::fixed(length));
}

class ScrollView::ScrollLayout : public Layout {
public:
	explicit ScrollLayout(ScrollView& view) :
		mView(view) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override {
		if (!mView.mContent || !mView.mContent->isVisible()) return {};

		Extent extent = context.measure(*mView.mContent, axis, crossLength);
		if (axis == mView.mScrollAxis) extent.minimum = 0;
		return extent;
	}

	void arrangeChildren(LayoutContext& context, const RectF& contentArea) override {
		const Axis scroll = mView.mScrollAxis;
		const Axis cross = crossAxis(scroll);

		if (mView.mContent && mView.mContent->isVisible()) {
			const halnf crossLength = lengthAlong(contentArea, cross);
			const halnf contentLength = context.measure(*mView.mContent, scroll, crossLength).preferred;

			mView.mContentLength = std::max(contentLength, lengthAlong(contentArea, scroll));
			mView.keepOffsetInRange();

			RectF rect;
			setSpanAlong(
				rect,
				scroll,
				startAlong(contentArea, scroll) - mView.mOffset.value() + mView.stretchOffset(),
				mView.mContentLength
			);
			setSpanAlong(rect, cross, startAlong(contentArea, cross), crossLength);
			context.place(*mView.mContent, rect);
		}

		const halnf thickness = mView.theme().metrics.scrollBarThickness;
		RectF bar = contentArea;
		setSpanAlong(bar, cross, startAlong(contentArea, cross) + lengthAlong(contentArea, cross) - thickness, thickness);
		mView.mScrollBar.setVisible(mView.mShowsScrollBar && mView.isScrollable());
		if (mView.mScrollBar.isVisible()) context.place(mView.mScrollBar, bar);
	}

private:
	ScrollView& mView;
};

ScrollBar::ScrollBar(ScrollView& view) :
	mView(view) {
	setDebugName("scroll bar");
	setGeometryAnimated(false);
}

RectF ScrollBar::trackRect() const {
	const Metrics& metrics = theme().metrics;
	const Axis axis = mView.scrollAxis();
	const Axis cross = crossAxis(axis);
	const halnf thickness = metrics.scrollTrackThickness;
	const halnf outerEdge = lengthAlong(localRect(), cross) - metrics.scrollTrackEdgeGap;

	RectF track;
	setSpanAlong(
		track, axis, metrics.scrollTrackEdgeGap, lengthAlong(localRect(), axis) - metrics.scrollTrackEdgeGap * 2
	);
	setSpanAlong(track, cross, outerEdge - thickness, thickness);
	return track;
}

RectF ScrollBar::thumbRect() const {
	const Axis axis = mView.scrollAxis();
	const RectF track = trackRect();
	const halnf trackLength = lengthAlong(track, axis);
	const halnf visibleFraction = mView.contentLength() > 0 ? mView.viewportLength() / mView.contentLength() : 1;
	const halnf thumbLength =
		clampLength(trackLength * visibleFraction, lengthAlong(track, crossAxis(axis)) * 3, trackLength);
	const halnf travel = trackLength - thumbLength;
	const halnf progress = mView.maximumOffset() > 0 ? mView.currentOffset() / mView.maximumOffset() : 0;

	RectF thumb = track;
	setSpanAlong(thumb, axis, startAlong(track, axis) + travel * progress, thumbLength);
	return thumb;
}

halnf ScrollBar::offsetForThumbAt(halnf pointerAlong) const {
	const Axis axis = mView.scrollAxis();
	const RectF track = trackRect();
	const halnf travel = lengthAlong(track, axis) - lengthAlong(thumbRect(), axis);
	if (travel <= 0) return 0;

	const halnf thumbStart = pointerAlong - mGrabOffset - startAlong(track, axis);
	return clampLength(thumbStart / travel, 0, 1) * mView.maximumOffset();
}

void ScrollBar::flash(halnf durationMs) {
	mFlashMs = std::max(mFlashMs, durationMs);
	refreshReveal();
}

void ScrollBar::onEvent(Event& event) {
	const Axis axis = mView.scrollAxis();

	switch (event.type()) {
		case Event::Type::PointerDown:
			{
				if (event.button() != PointerButton::Primary) return;
				const RectF thumb = thumbRect();
				const halnf pointer = along(event.position(), axis);
				const bool onThumb =
					pointer >= startAlong(thumb, axis) && pointer <= startAlong(thumb, axis) + lengthAlong(thumb, axis);

				mGrabOffset = onThumb ? pointer - startAlong(thumb, axis) : lengthAlong(thumb, axis) / 2;
				mDragging = true;
				ui()->capturePointer(*this);
				if (!onThumb) mView.scrollTo(offsetForThumbAt(pointer));
				break;
			}

		case Event::Type::PointerMove:
			if (!mDragging) return;
			mView.jumpTo(offsetForThumbAt(along(event.position(), axis)));
			break;

		case Event::Type::PointerUp:
			if (!mDragging || event.button() != PointerButton::Primary) return;
			mDragging = false;
			ui()->releasePointer(*this);
			break;

		default: return;
	}

	event.accept();
	refreshReveal();
}

void ScrollBar::onHoverChanged() { refreshReveal(); }

void ScrollBar::refreshReveal() {
	mReveal.animateTo(isHovered() || mDragging || mFlashMs > 0 ? 1.0f : 0.0f);
	startTicking();
}

void ScrollBar::onTick(halnf deltaMs) {
	if (mFlashMs > 0) {
		mFlashMs -= deltaMs;
		if (mFlashMs <= 0) refreshReveal();
	}

	if (!mReveal.advance(deltaMs) && mFlashMs <= 0) stopTicking();
	requestPaint();
}

void ScrollBar::paint(Painter& painter) {
	const halnf reveal = clampLength(mReveal.value(), 0, 1);
	if (reveal <= 0.01f) return;

	const Axis cross = crossAxis(mView.scrollAxis());
	const auto growFromOuterEdge = [cross, reveal](RectF rect) {
		const halnf full = lengthAlong(rect, cross);
		const halnf outerEdge = startAlong(rect, cross) + full;
		setSpanAlong(rect, cross, outerEdge - full * reveal, full * reveal);
		return rect;
	};

	const RGBA& muted = theme().palette.mutedText;
	const bool active = mDragging || isHovered();

	const RectF track = growFromOuterEdge(trackRect());
	painter.fillRect(track, { muted.r, muted.g, muted.b, 0.12f }, lengthAlong(track, cross) / 2);

	const RectF thumb = growFromOuterEdge(thumbRect());
	painter.fillRect(thumb, { muted.r, muted.g, muted.b, active ? 0.65f : 0.45f }, lengthAlong(thumb, cross) / 2);
}

ScrollView::ScrollView(Axis scrollAxis) :
	mScrollAxis(scrollAxis),
	mOffset(0, Motion::quick()) {
	setDebugName("scroll view");
	setClipsChildren(true);
	setLayout(std::make_unique<ScrollLayout>(*this));
	addChild(mScrollBar);
}

void ScrollView::setContent(Widget* content) {
	if (mContent) removeChild(*mContent);

	mContent = content;
	if (mContent) {
		insertChild(0, *mContent);
		mContent->setGeometryAnimated(false);
	}
	scrollTo(0);
}

void ScrollView::scrollTo(halnf offset) {
	mOffset.animateTo(clampLength(offset, 0, maximumOffset()));
	if (isScrollable() && mShowsScrollBar) mScrollBar.flash(kShowAfterScrollMs);
	startTicking();
}

void ScrollView::jumpTo(halnf offset) {
	mOffset.jumpTo(clampLength(offset, 0, maximumOffset()));
	applyOffset();
}

void ScrollView::scrollBy(halnf delta) { scrollTo(mOffset.target() + delta); }

halnf ScrollView::maximumOffset() const {
	const halnf overflow = mContentLength - viewportLength();
	return overflow > 0 ? overflow : 0;
}

Extent ScrollView::measureContent(Axis, halnf) const { return {}; }

void ScrollView::onEvent(Event& event) {
	if (event.type() == Event::Type::Wheel) {
		if (!isScrollable() || event.wheelAmount() == 0) return;
		scrollBy(-event.wheelAmount() * mWheelStep);
		event.accept();
		return;
	}
	trackDrag(event);
}

void ScrollView::filterDescendantEvent(Event& event) { trackDrag(event); }

bool ScrollView::startsDrag(const Event& event) const {
	return event.type() == Event::Type::PointerDown && event.button() == PointerButton::Primary && ui() &&
				 ui()->isTouchMode() && isScrollable() && !mScrollBar.isDragging();
}

void ScrollView::trackDrag(Event& event) {
	if (startsDrag(event)) {
		const bool catching = mFling.isActive() || mStartStretch.isActive() || mEndStretch.isActive();
		mFling.stop();
		mFlingVelocity = 0;
		mStartStretch.pull(0);
		mEndStretch.pull(0);
		mDragPending = true;
		mDragging = catching;
		mDragStart = event.windowPosition();
		mDragStartOffset = mOffset.value();
		mLastDragAlong = 0;
		mDragSamples.clear();
		rememberDragSample(event.sample().timeMs);
		if (catching) {
			ui()->capturePointer(*this);
			event.accept();
		}
		return;
	}
	if (!mDragPending) return;

	if (event.type() == Event::Type::PointerMove) {
		const Widget* holder = ui()->pointerCapture();
		if (!mDragging && holder && holder != this) {
			mDragPending = false;
			return;
		}
		const halnf moved = along(event.windowPosition() - mDragStart, mScrollAxis);
		if (!mDragging && std::abs(moved) < kDragThreshold) return;
		if (!mDragging) {
			mDragging = true;
			ui()->capturePointer(*this);
		}
		dragBy(mLastDragAlong - moved);
		mLastDragAlong = moved;
		rememberDragSample(event.sample().timeMs);
		event.accept();
	} else if (event.type() == Event::Type::PointerUp && event.button() == PointerButton::Primary) {
		mDragPending = false;
		if (!mDragging) return;
		mDragging = false;
		ui()->releasePointer(*this);
		event.accept();
		const halnf velocity = averageDragVelocity(event.sample().timeMs);
		mStartStretch.release();
		mEndStretch.release();
		if (std::abs(velocity) >= kFlingMinimumSpeed) {
			mFling.start(mOffset.value(), velocity * 1000.0f, 0, maximumOffset());
			mFlingVelocity = velocity;
		}
		startTicking();
	}
}

void ScrollView::rememberDragSample(uint4 timeMs) {
	if (!mDragSamples.empty() && mDragSamples.back().timeMs == timeMs) {
		mDragSamples.back().offset = mOffset.value();
		return;
	}
	mDragSamples.push_back({ timeMs, mOffset.value() });
	while (mDragSamples.size() > 2 && timeMs - mDragSamples[1].timeMs > kFlingRecentMs)
		mDragSamples.erase(mDragSamples.begin());
}

void ScrollView::dragBy(halnf delta) {
	halnf remaining = delta;
	if (remaining < 0 && mEndStretch.distance() > 0) remaining -= mEndStretch.pull(remaining);
	if (remaining > 0 && mStartStretch.distance() > 0) remaining += mStartStretch.pull(-remaining);
	const halnf target = mOffset.value() + remaining;
	const halnf limit = maximumOffset();
	if (target < 0) {
		mOffset.jumpTo(0);
		mStartStretch.setLength(viewportLength());
		mStartStretch.pull(-target);
	} else if (target > limit) {
		mOffset.jumpTo(limit);
		mEndStretch.setLength(viewportLength());
		mEndStretch.pull(target - limit);
	} else {
		mOffset.jumpTo(target);
	}
	applyOffset();
}

halnf ScrollView::stretchOffset() const { return mStartStretch.visualOffset() - mEndStretch.visualOffset(); }

halnf ScrollView::averageDragVelocity(uint4 nowMs) const { return averageVelocity(mDragSamples, nowMs); }

halnf ScrollView::averageVelocity(const std::vector<DragSample>& samples, uint4 nowMs) {
	if (samples.size() < 2) return 0;
	const DragSample& newest = samples.back();
	if (nowMs - newest.timeMs >= kFlingRecentMs) return 0;
	size_t first = samples.size() - 1;
	while (first > 0 && newest.timeMs - samples[first - 1].timeMs <= kFlingRecentMs)
		first--;
	if (first == samples.size() - 1) first--;
	const DragSample& oldest = samples[first];
	if (newest.timeMs <= oldest.timeMs) return 0;
	return (newest.offset - oldest.offset) / (halnf) (newest.timeMs - oldest.timeMs);
}

void ScrollView::onTick(halnf deltaMs) {
	bool moving = false;
	if (mFling.isActive()) {
		moving = mFling.advance(deltaMs);
		mOffset.jumpTo(mFling.position());
		mFlingVelocity = mFling.velocity() / 1000.0f;
		if (!moving) {
			const halnf impact = mFling.velocity();
			mFlingVelocity = 0;
			if (mFling.hitEdge() && impact != 0) {
				StretchEffect& edge = impact < 0 ? mStartStretch : mEndStretch;
				edge.setLength(viewportLength());
				edge.absorb(impact);
			}
		}
		moving = true;
	} else if (mStartStretch.isActive() || mEndStretch.isActive()) {
		const bool start = mStartStretch.advance(deltaMs);
		const bool end = mEndStretch.advance(deltaMs);
		moving = start || end;
	} else {
		moving = mOffset.advance(deltaMs);
	}
	applyOffset();
	if (!moving) stopTicking();
}

halnf ScrollView::viewportLength() const { return lengthAlong(localRect(), mScrollAxis); }

void ScrollView::keepOffsetInRange() {
	const halnf limit = maximumOffset();
	if (mOffset.target() > limit) mOffset.animateTo(limit);
	if (mOffset.value() > limit) mOffset.jumpTo(limit);
}

void ScrollView::applyOffset() {
	const halnf current = mOffset.value();
	const bool moved = current != mReportedOffset;
	mReportedOffset = current;
	if (moved && mOnScrolled) mOnScrolled(current);
	requestLayout();
	requestPaint();
}
