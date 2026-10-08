#include "Views.hpp"

#include "BoxLayout.hpp"
#include "Event.hpp"
#include "Layout.hpp"
#include "StackLayout.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <cmath>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kTabBodyGap = 12;
	constexpr halnf kCarouselBodyGap = 6;
	constexpr halnf kCarouselHeaderInset = 6;
	constexpr halnf kPositionEpsilon = 0.0005f;
	constexpr halnf kMinimumSampleMs = 16;
	constexpr halnf kTwoPi = 6.28318530f;
	constexpr halnf kTriangleInset = 6;

	halnf clampValue(halnf value, halnf minimum, halnf maximum) {
		if (value < minimum) return minimum;
		if (value > maximum) return maximum;
		return value;
	}
}

Vec2F tp::ui::saturationValueAt(Vec2F unitTrianglePoint) {
	const halnf whiteToHue = clampValue((1 - unitTrianglePoint.x / std::cos(kTwoPi / 3)) / 3, 0, 1);
	const halnf blackToWhite = (unitTrianglePoint.y / std::sin(kTwoPi / 3) + 1) / 2;
	const halnf insideTriangle = clampValue(blackToWhite, whiteToHue / 2, 1 - whiteToHue / 2);

	const halnf value = insideTriangle + whiteToHue / 2;
	const halnf saturation = value > 0 ? whiteToHue / value : 0;
	return { clampValue(saturation, 0, 1), clampValue(value, 0, 1) };
}

class SplitView::SplitLayout : public Layout {
public:
	explicit SplitLayout(SplitView& view) :
		mView(view) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override {
		Extent total = { 0, 0, kUnbounded, 1 };
		const halnf handle = mView.theme().metrics.splitterThickness;

		for (const Widget* pane : context.children()) {
			if (pane == &mView.mHandle) continue;
			const Extent extent = context.measure(*pane, axis, crossLength);
			if (axis == mView.mAxis) {
				total.minimum += extent.minimum;
				total.preferred += extent.preferred;
			} else {
				total.minimum = std::max(total.minimum, extent.minimum);
				total.preferred = std::max(total.preferred, extent.preferred);
			}
		}

		if (axis == mView.mAxis) {
			total.minimum += handle;
			total.preferred += handle;
		}
		return total;
	}

	void arrangeChildren(LayoutContext& context, const RectF& contentArea) override {
		const Axis axis = mView.mAxis;
		const Axis cross = crossAxis(axis);
		const halnf handle = mView.theme().metrics.splitterThickness;
		const halnf available = lengthAlong(contentArea, axis) - handle;
		const halnf firstLength = std::round(available * mView.mRatio);

		RectF firstRect = contentArea;
		setSpanAlong(firstRect, axis, startAlong(contentArea, axis), firstLength);

		RectF secondRect = contentArea;
		setSpanAlong(secondRect, axis, startAlong(contentArea, axis) + firstLength + handle, available - firstLength);
		setSpanAlong(secondRect, cross, startAlong(contentArea, cross), lengthAlong(contentArea, cross));

		if (mView.mFirst && mView.mFirst->isVisible()) context.place(*mView.mFirst, firstRect);
		if (mView.mSecond && mView.mSecond->isVisible()) context.place(*mView.mSecond, secondRect);

		const bool bothPanes = mView.mFirst && mView.mFirst->isVisible() && mView.mSecond && mView.mSecond->isVisible();
		RectF handleRect = contentArea;
		const halnf handleStart = startAlong(contentArea, axis) + (bothPanes ? firstLength : available + handle);
		setSpanAlong(handleRect, axis, handleStart, bothPanes ? handle : 0);
		context.place(mView.mHandle, handleRect);
	}

private:
	SplitView& mView;
};

SplitView::SplitView(Axis axis) :
	mAxis(axis),
	mHandle(axis) {
	setDebugName("split view");
	setSizing(AxisSizing::fill(), AxisSizing::fill());
	setLayout(std::make_unique<SplitLayout>(*this));

	mHandle.setOnDragged([this](halnf windowCoordinate) {
		setRatio(ratioAt(windowCoordinate - along(windowPosition(), mAxis)));
	});
	addChild(mHandle);
}

void SplitView::setFirst(Widget* first) { replaceChild(mFirst, first); }

void SplitView::setSecond(Widget* second) { replaceChild(mSecond, second); }

void SplitView::setRatio(halnf ratio) {
	const halnf next = clampValue(ratio, 0, 1);
	if (next == mRatio) return;

	mRatio = next;
	requestLayout();
}

void SplitView::setMinimumPaneLength(halnf length) {
	mMinimumPaneLength = length;
	setRatio(mRatio);
}

RectF SplitView::handleRect() const {
	const halnf handle = theme().metrics.splitterThickness;
	const halnf available = lengthAlong(localRect(), mAxis) - handle;

	RectF rect = localRect();
	setSpanAlong(rect, mAxis, std::round(available * mRatio), handle);
	return rect;
}

void SplitView::replaceChild(Widget*& slot, Widget* widget) {
	if (slot) removeChild(*slot);

	slot = widget;
	if (slot) insertChild(0, *slot);
}

halnf SplitView::ratioAt(halnf alongAxis) const {
	const halnf handle = theme().metrics.splitterThickness;
	const halnf available = lengthAlong(localRect(), mAxis) - handle;
	if (available <= 0) return mRatio;

	const halnf minimum = std::min(mMinimumPaneLength, available / 2);
	const halnf position = clampValue(alongAxis - handle / 2, minimum, available - minimum);
	return position / available;
}

class TabView::CarouselLayout : public Layout {
public:
	explicit CarouselLayout(TabView& view) :
		mView(view) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override {
		Extent largest = { 0, 0, kUnbounded, 1 };
		for (const Widget* slot : context.children()) {
			const Extent extent = context.measure(*slot, axis, crossLength);
			largest.minimum = std::max(largest.minimum, extent.minimum);
			largest.preferred = std::max(largest.preferred, extent.preferred);
		}
		return largest;
	}

	void arrangeChildren(LayoutContext& context, const RectF& contentArea) override {
		const halnf width = contentArea.size.x;
		for (const ShownPage& shown : mView.shownPages()) {
			Widget& slot = mView.holder(mView.mTabs[shown.index]);
			if (!slot.isVisible()) continue;
			const halnf x = contentArea.pos.x + std::round(shown.offset * width);
			context.place(slot, makeRect(x, contentArea.pos.y, width, contentArea.size.y));
		}
	}

private:
	TabView& mView;
};

class TabView::CarouselTabBar : public Panel {
public:
	explicit CarouselTabBar(TabView& view) :
		Panel(Surface::None),
		mView(view) {
		setDebugName("carousel tab bar");
	}

protected:
	void paint(Painter& painter) override {
		Panel::paint(painter);
		const Theme& style = theme();
		const RGBA selection = style.palette.neutralSelection;
		for (const Indicator& indicator : mView.indicators()) {
			const RGBA faded = { selection.r, selection.g, selection.b, selection.a * indicator.opacity };
			painter.fillRect(indicator.rect, faded, style.metrics.cornerRadius);
		}
	}

private:
	TabView& mView;
};

class TabView::CarouselTabButton : public Button {
public:
	using Button::Button;

protected:
	[[nodiscard]] RGBA backgroundTarget() const override {
		if (!isSelected() || isHovered() || isPressed()) return Button::backgroundTarget();
		const RGBA hovered = theme().palette.controlHovered;
		return { hovered.r, hovered.g, hovered.b, 0 };
	}
};

TabView::TabView(Style style) :
	mStyle(style) {
	setDebugName(style == Style::Carousel ? "carousel tab view" : "tab view");

	if (style == Style::Carousel) mTabBar = std::make_unique<CarouselTabBar>(*this);
	else mTabBar = std::make_unique<Panel>(Panel::Surface::None);
	mTabBar->setSizing(AxisSizing::fit(), AxisSizing::fit());
	mTabBar->emplaceLayout<RowLayout>(2).setPadding(Insets::symmetric(4, 2));
	mTabScroller.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mTabScroller.setWheelStep(40);
	mTabScroller.setShowsScrollBar(false);
	mTabScroller.setContent(mTabBar.get());

	mPageArea.setSurface(Panel::Surface::None);
	mPageArea.setSizing(AxisSizing::fill(), AxisSizing::fill());

	if (style == Style::Carousel) buildCarouselHeader();
	else buildTabsHeader();
	addChild(mPageArea);
}

void TabView::setTabAlignment(TabAlignment alignment) {
	if (alignment == mTabAlignment) return;
	mTabAlignment = alignment;
	auto* row = dynamic_cast<RowLayout*>(mTabBar->layout());
	if (row) row->setMainAlign(alignment == TabAlignment::Center ? MainAlign::Center : MainAlign::Start);
	mTabBar->setSizing(alignment == TabAlignment::Center ? AxisSizing::fill() : AxisSizing::fit(), AxisSizing::fit());
}

void TabView::buildTabsHeader() {
	emplaceLayout<ColumnLayout>(kTabBodyGap);
	mPageArea.emplaceLayout<StackLayout>();
	addChild(mTabScroller);
}

void TabView::buildCarouselHeader() {
	emplaceLayout<ColumnLayout>(kCarouselBodyGap);
	mPageArea.setLayout(std::make_unique<CarouselLayout>(*this));
	mPageArea.setClipsChildren(true);

	mPrevious.setTooltip("Previous page");
	mPrevious.setOnActivate([this] { showPrevious(); });
	mNext.setTooltip("Next page");
	mNext.setOnActivate([this] { showNext(); });

	mHeader.setSizing(AxisSizing::fill(), AxisSizing::fit());
	RowLayout& headerRow = mHeader.emplaceLayout<RowLayout>(2);
	headerRow.setCrossAlign(CrossAlign::Center);
	headerRow.setPadding(Insets::symmetric(kCarouselHeaderInset, 0));
	mHeader.addChild(mPrevious);
	mHeader.addChild(mTabScroller);
	mHeader.addChild(mNext);
	addChild(mHeader);
}

std::unique_ptr<Button> TabView::makeTabButton(std::string title, size_t index) {
	auto activate = [this, index]() { setCurrentIndex(index); };
	std::unique_ptr<Button> button;
	if (mStyle == Style::Carousel) button = std::make_unique<CarouselTabButton>(std::move(title), activate);
	else button = std::make_unique<Button>(std::move(title), activate);
	button->setStyle(Button::Style::Tab);
	return button;
}

void TabView::addTab(std::string title, Widget& page) {
	const size_t index = mTabs.size();

	std::unique_ptr<Button> button = makeTabButton(std::move(title), index);
	mTabBar->addChild(*button);

	std::unique_ptr<Widget> slot;
	if (mStyle == Style::Carousel) {
		slot = std::make_unique<Widget>();
		slot->setDebugName("carousel page");
		slot->setSizing(AxisSizing::fill(), AxisSizing::fill());
		slot->setGeometryAnimated(false);
		slot->emplaceLayout<StackLayout>();
		slot->addChild(page);
	}

	mTabs.push_back({ std::move(button), &page, std::move(slot) });
	Widget& pageHolder = holder(mTabs.back());
	pageHolder.setVisible(false);
	mPageArea.addChild(pageHolder);

	if (mCurrent < 0) setCurrentIndex(0);
	else positionChanged();
}

void TabView::removeTab(Widget& page) {
	const auto position = std::find_if(mTabs.begin(), mTabs.end(), [&page](const Tab& tab) { return tab.page == &page; });
	if (position == mTabs.end()) return;

	finishTransitions();
	const long removedIndex = (long) (position - mTabs.begin());
	mTabBar->removeChild(*position->button);
	mPageArea.removeChild(holder(*position));
	if (position->slot) position->slot->removeChild(page);
	page.setVisible(true);
	mTabs.erase(position);

	for (size_t index = 0; index < mTabs.size(); index++) {
		mTabs[index].button->setOnActivate([this, index]() { setCurrentIndex(index); });
	}

	if (mTabs.empty()) {
		mCurrent = -1;
		return;
	}

	const long next = removedIndex >= mCurrent ? std::min<long>(mCurrent, (long) mTabs.size() - 1) : mCurrent - 1;
	mCurrent = -1;
	setCurrentIndex((size_t) next);
}

void TabView::revealTab(size_t index) {
	if (index >= mTabs.size() || !mTabScroller.isScrollable()) return;
	const RectF tab = mTabs[index].button->rect();
	const halnf view = mTabScroller.viewportLength();
	const halnf offset = mTabScroller.offset();
	const halnf start = tab.pos.x;
	const halnf margin = 24;
	if (start - margin < offset) mTabScroller.scrollTo(start - margin);
	else if (start + tab.size.x + margin > offset + view) mTabScroller.scrollTo(start + tab.size.x + margin - view);
}

void TabView::setCurrentIndex(size_t index) {
	if (index >= mTabs.size() || (long) index == mCurrent) return;

	if (!slides() || mCurrent < 0) {
		showOnly(index);
		return;
	}

	const long count = (long) mTabs.size();
	long delta = (long) index - (long) cyclicIndex(mStep.committed());
	if (delta > count / 2) delta -= count;
	if (delta < -count / 2) delta += count;
	scrollBy(delta);
}

void TabView::showNext() {
	if (mTabs.size() < 2) return;
	if (!slides()) {
		showOnly(cyclicIndex(mCurrent + 1));
		return;
	}
	scrollBy(1);
}

void TabView::showPrevious() {
	if (mTabs.size() < 2) return;
	if (!slides()) {
		showOnly(cyclicIndex(mCurrent - 1));
		return;
	}
	scrollBy(-1);
}

size_t TabView::cyclicIndex(long index) const {
	const long count = (long) mTabs.size();
	return (size_t) (((index % count) + count) % count);
}

void TabView::showOnly(size_t index) {
	mStep.jumpTo((long) index);
	for (size_t tab = 0; tab < mTabs.size(); tab++)
		holder(mTabs[tab]).setVisible(tab == index);
	mPageArea.requestLayout();
	currentChanged(index);
}

void TabView::scrollBy(long pages) {
	mStep.moveBy(pages);
	startTicking();
	positionChanged();
}

void TabView::pull(halnf pages) {
	mStep.push(pages);
	startTicking();
	positionChanged();
}

void TabView::positionChanged() {
	std::vector<bool> shown(mTabs.size(), false);
	for (const ShownPage& page : shownPages())
		shown[page.index] = true;
	for (size_t tab = 0; tab < mTabs.size(); tab++)
		holder(mTabs[tab]).setVisible(shown[tab]);

	mPageArea.requestLayout();
	mTabBar->requestPaint();
	currentChanged(cyclicIndex(mStep.committed()));
}

void TabView::currentChanged(size_t index) {
	if ((long) index == mCurrent) return;

	mCurrent = (long) index;
	for (size_t tab = 0; tab < mTabs.size(); tab++)
		mTabs[tab].button->setSelected(tab == index);
	revealTab(index);

	if (mOnChanged) mOnChanged(index);
}

std::vector<TabView::ShownPage> TabView::shownPages() const {
	if (mTabs.empty()) return {};

	const halnf position = mStep.position();
	const halnf base = std::floor(position);
	const halnf fraction = position - base;
	const long baseIndex = (long) base;

	std::vector<ShownPage> pages = { { cyclicIndex(baseIndex), -fraction } };
	const bool between = fraction > kPositionEpsilon && fraction < 1 - kPositionEpsilon;
	if (between && cyclicIndex(baseIndex + 1) != pages.front().index)
		pages.push_back({ cyclicIndex(baseIndex + 1), 1 - fraction });
	if (fraction >= 1 - kPositionEpsilon) pages = { { cyclicIndex(baseIndex + 1), 0 } };
	return pages;
}

std::vector<TabView::Indicator> TabView::indicators() const {
	if (mTabs.empty()) return {};

	const halnf position = mStep.position();
	const halnf base = std::floor(position);
	const halnf fraction = position - base;
	const size_t from = cyclicIndex((long) base);
	const size_t to = cyclicIndex((long) base + 1);
	const RectF fromRect = mTabs[from].button->rect();
	const RectF toRect = mTabs[to].button->rect();

	if (fraction <= kPositionEpsilon) return { { fromRect, 1 } };
	if (fraction >= 1 - kPositionEpsilon) return { { toRect, 1 } };

	if (to > from) {
		const RectF between = makeRect(
			fromRect.pos + (toRect.pos - fromRect.pos) * fraction, fromRect.size + (toRect.size - fromRect.size) * fraction
		);
		return { { between, 1 } };
	}

	RectF leaving = fromRect;
	leaving.pos.x += fromRect.size.x * fraction;
	RectF arriving = toRect;
	arriving.pos.x -= toRect.size.x * (1 - fraction);
	return { { leaving, 1 - fraction }, { arriving, fraction } };
}

RectF TabView::selectionIndicatorRect() const {
	const std::vector<Indicator> shown = indicators();
	if (shown.empty()) return makeRect(0, 0, 0, 0);
	const Indicator& strongest =
		*std::max_element(shown.begin(), shown.end(), [](const Indicator& a, const Indicator& b) {
			return a.opacity < b.opacity;
		});
	return strongest.rect;
}

void TabView::onTick(halnf deltaMs) {
	mStep.advance(deltaMs);
	positionChanged();
	if (mStep.isActive()) return;

	stopTicking();
	settleOnCommitted();
}

void TabView::settleOnCommitted() {
	if (mTabs.empty()) return;
	mStep.jumpTo((long) cyclicIndex(mStep.committed()));
	positionChanged();
}

void TabView::finishTransitions() {
	if (mStyle != Style::Carousel) return;
	settleOnCommitted();
}

void TabView::onEvent(Event& event) {
	if (event.type() != Event::Type::Wheel || !handleWheel(event)) return;
	event.accept();
}

bool TabView::handleWheel(const Event& event) {
	if (mStyle != Style::Carousel || mTabs.size() < 2) return false;

	halnf amount = event.horizontalWheelAmount();
	if (amount == 0 && event.modifiers().shift) amount = event.wheelAmount();
	if (amount == 0) return false;

	const bool mouseNotch = amount == std::round(amount);
	if (mouseNotch) scrollBy(-std::lround(amount));
	else pull(-amount * kPagesPerTrackpadUnit);
	return true;
}

ColorWheel::ColorWheel() :
	mHsv(0, 0.8f, 0.9f) {
	setDebugName("color wheel");
}

void ColorWheel::setColor(const RGBA& color) {
	mHsv = HSV(RGB(color.r, color.g, color.b));
	requestPaint();
}

RGBA ColorWheel::color() const {
	const RGB rgb = mHsv;
	return { rgb.r, rgb.g, rgb.b, 1 };
}

Extent ColorWheel::measureContent(Axis, halnf) const { return { 80, 180, kUnbounded, 0 }; }

void ColorWheel::onEvent(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown:
			if (event.button() != PointerButton::Primary) return;
			mDragTarget = targetAt(event.position());
			if (mDragTarget == DragTarget::None) return;
			ui()->capturePointer(*this);
			pickAt(event.position());
			event.accept();
			break;

		case Event::Type::PointerMove:
			if (mDragTarget == DragTarget::None) return;
			pickAt(event.position());
			event.accept();
			break;

		case Event::Type::PointerUp:
			if (mDragTarget == DragTarget::None) return;
			mDragTarget = DragTarget::None;
			ui()->releasePointer(*this);
			event.accept();
			break;

		default: return;
	}
}

void ColorWheel::paint(Painter& painter) { painter.colorWheel(wheelRect(), mHsv, theme().metrics.sliderHandleSize); }

RectF ColorWheel::wheelRect() const {
	const halnf side = std::min(size().x, size().y);
	return makeRect((size().x - side) / 2, (size().y - side) / 2, side, side);
}

ColorWheel::DragTarget ColorWheel::targetAt(Vec2F localPoint) const {
	const RectF wheel = wheelRect();
	const Vec2F offset = localPoint - wheel.center();
	const halnf distance = (halnf) offset.length();
	const halnf outerRadius = wheel.size.x / 2;
	const halnf innerRadius = outerRadius - theme().metrics.sliderHandleSize;

	if (distance > outerRadius) return DragTarget::None;
	if (distance >= innerRadius) return DragTarget::Hue;
	return DragTarget::SaturationValue;
}

void ColorWheel::pickAt(Vec2F localPoint) {
	const RectF wheel = wheelRect();
	const Vec2F offset = localPoint - wheel.center();

	if (mDragTarget == DragTarget::Hue) {
		const halnf angle = std::atan2(offset.y, offset.x);
		mHsv.h = angle >= 0 ? angle : angle + kTwoPi;
	} else {
		const halnf triangleRadius = wheel.size.x / 2 - theme().metrics.sliderHandleSize - kTriangleInset;
		const halnf cosine = std::cos(-mHsv.h);
		const halnf sine = std::sin(-mHsv.h);
		const Vec2F unrotated = { offset.x * cosine - offset.y * sine, offset.x * sine + offset.y * cosine };
		const Vec2F saturationValue = saturationValueAt(unrotated / triangleRadius);

		mHsv.s = saturationValue.x;
		mHsv.v = saturationValue.y;
	}

	requestPaint();
	if (mOnChanged) mOnChanged(color());
}

ColorSquare::ColorSquare() :
	mHsv(0, 0.8f, 0.9f) {
	setDebugName("color square");
}

void ColorSquare::setHsv(const HSV& hsv) {
	mHsv = hsv;
	requestPaint();
}

RGBA ColorSquare::color() const {
	const RGB rgb = mHsv;
	return { rgb.r, rgb.g, rgb.b, 1 };
}

Extent ColorSquare::measureContent(Axis axis, halnf) const {
	if (axis == Axis::Horizontal) return { 80, 180, kUnbounded, 0 };
	return { 80 + kHueStripHeight + kGap, 180 + kHueStripHeight + kGap, kUnbounded, 0 };
}

RectF ColorSquare::squareRect() const {
	const halnf side = std::max(0.0f, std::min(size().x, size().y - kHueStripHeight - kGap));
	return makeRect((size().x - side) / 2, 0, side, side);
}

RectF ColorSquare::hueRect() const {
	const RectF square = squareRect();
	return makeRect(square.pos.x, square.pos.y + square.size.y + kGap, square.size.x, kHueStripHeight);
}

Vec2F ColorSquare::windowPointFor(halnf saturation, halnf value) const {
	const RectF square = squareRect();
	return windowRect().pos + square.pos + Vec2F{ square.size.x * saturation, square.size.y * (1 - value) };
}

Vec2F ColorSquare::windowPointForHue(halnf hueRadians) const {
	const RectF strip = hueRect();
	return windowRect().pos + strip.pos + Vec2F{ strip.size.x * hueRadians / kTwoPi, strip.size.y / 2 };
}

void ColorSquare::onEvent(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown:
			if (event.button() != PointerButton::Primary) return;
			if (contains(makeRect(squareRect().pos - Vec2F{ 1, 1 }, squareRect().size + Vec2F{ 2, 2 }), event.position()))
				mDragTarget = DragTarget::Square;
			else if (contains(makeRect(hueRect().pos - Vec2F{ 1, 1 }, hueRect().size + Vec2F{ 2, 2 }), event.position()))
				mDragTarget = DragTarget::Hue;
			else return;
			ui()->capturePointer(*this);
			pickAt(event.position());
			event.accept();
			break;

		case Event::Type::PointerMove:
			if (mDragTarget == DragTarget::None) return;
			pickAt(event.position());
			event.accept();
			break;

		case Event::Type::PointerUp:
			if (mDragTarget == DragTarget::None) return;
			mDragTarget = DragTarget::None;
			ui()->releasePointer(*this);
			event.accept();
			break;

		default: return;
	}
}

void ColorSquare::pickAt(Vec2F localPoint) {
	if (mDragTarget == DragTarget::Square) {
		const RectF square = squareRect();
		if (square.size.x <= 0) return;
		mHsv.s = clampValue((localPoint.x - square.pos.x) / square.size.x, 0, 1);
		mHsv.v = clampValue(1 - (localPoint.y - square.pos.y) / square.size.y, 0, 1);
	} else {
		const RectF strip = hueRect();
		if (strip.size.x <= 0) return;
		mHsv.h = clampValue((localPoint.x - strip.pos.x) / strip.size.x, 0, 0.9999f) * kTwoPi;
	}
	requestPaint();
	if (mOnChanged) mOnChanged(mHsv);
}

void ColorSquare::paint(Painter& painter) {
	const RectF square = squareRect();
	const RGB pure = HSV(mHsv.h, 1, 1);
	painter.fillGradientRect(square, { 1, 1, 1, 1 }, { pure.r, pure.g, pure.b, 1 }, true);
	painter.fillGradientRect(square, { 0, 0, 0, 0 }, { 0, 0, 0, 1 }, false);

	const RectF strip = hueRect();
	constexpr int4 kSegments = 6;
	const halnf width = strip.size.x / kSegments;
	for (int4 index = 0; index < kSegments; index++) {
		const RGB from = HSV(kTwoPi * (halnf) index / kSegments, 1, 1);
		const RGB to = HSV(kTwoPi * (halnf) (index + 1) / kSegments - 1e-4f, 1, 1);
		painter.fillGradientRect(
			makeRect(strip.pos.x + width * (halnf) index, strip.pos.y, width, strip.size.y),
			{ from.r, from.g, from.b, 1 },
			{ to.r, to.g, to.b, 1 },
			true
		);
	}

	const Vec2F marker = square.pos + Vec2F{ square.size.x * mHsv.s, square.size.y * (1 - mHsv.v) };
	painter.strokeCircle(marker, 6, { 0, 0, 0, 0.8f }, 3);
	painter.strokeCircle(marker, 6, { 1, 1, 1, 1 }, 1.5f);
	const halnf hueX = strip.pos.x + strip.size.x * mHsv.h / kTwoPi;
	painter.strokeRect(makeRect(hueX - 3, strip.pos.y - 2, 6, strip.size.y + 4), { 0, 0, 0, 0.8f }, 2, 3);
	painter.strokeRect(makeRect(hueX - 3, strip.pos.y - 2, 6, strip.size.y + 4), { 1, 1, 1, 1 }, 2, 1.5f);
}

SwatchGrid::SwatchGrid() { setDebugName("swatch grid"); }

void SwatchGrid::setColors(std::vector<RGBA> colors) {
	mColors = std::move(colors);
	if (mSelected >= (long) mColors.size()) mSelected = -1;
	requestLayout();
}

void SwatchGrid::setCaptions(std::vector<std::string> captions) {
	mCaptions = std::move(captions);
	requestPaint();
}

void SwatchGrid::setSelected(long index) {
	mSelected = index;
	requestPaint();
}

size_t SwatchGrid::columnsFor(halnf width) const {
	const halnf usable = width + kGap;
	const size_t count = usable > 0 ? (size_t) (usable / (kCell + kGap)) : 0;
	return std::max<size_t>(1, count);
}

size_t SwatchGrid::columns() const { return columnsFor(size().x); }

Extent SwatchGrid::measureContent(Axis axis, halnf crossLength) const {
	if (axis == Axis::Horizontal) return Extent::shrinkable(kCell * 6 + kGap * 5, kCell);
	const size_t perRow = columnsFor(crossLength < kUnbounded ? crossLength : kCell * 6 + kGap * 5);
	const size_t rows = std::max<size_t>(1, (mColors.size() + perRow - 1) / perRow);
	return Extent::rigid((halnf) rows * kCell + (halnf) (rows - 1) * kGap);
}

RectF SwatchGrid::cellRect(size_t index) const {
	const size_t perRow = columns();
	return makeRect((halnf) (index % perRow) * (kCell + kGap), (halnf) (index / perRow) * (kCell + kGap), kCell, kCell);
}

long SwatchGrid::indexAt(Vec2F localPoint) const {
	for (size_t index = 0; index < mColors.size(); index++)
		if (contains(cellRect(index), localPoint)) return (long) index;
	return -1;
}

void SwatchGrid::onEvent(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown:
			if (event.button() != PointerButton::Primary) return;
			mPressed = indexAt(event.position());
			if (mPressed < 0) return;
			event.accept();
			break;
		case Event::Type::Click:
			{
				const long index = indexAt(event.position());
				if (index < 0) return;
				mSelected = index;
				requestPaint();
				if (mOnPicked) mOnPicked((size_t) index);
				event.accept();
				break;
			}
		case Event::Type::LongPress:
			{
				const long index = indexAt(event.position());
				if (index < 0 || !mOnHeld) return;
				mOnHeld((size_t) index);
				event.accept();
				break;
			}
		default: Control::onEvent(event); return;
	}
}

void SwatchGrid::paint(Painter& painter) {
	const Theme& style = theme();
	for (size_t index = 0; index < mColors.size(); index++) {
		const RectF cell = cellRect(index);
		painter.fillRect(cell, mColors[index], 4);
		painter.strokeRect(cell, style.palette.controlBorder, 4, 1);
		if ((long) index == mSelected) painter.strokeRect(insetRect(cell, -3), style.palette.text, 6, 2);
		if (index < mCaptions.size() && !mCaptions[index].empty()) {
			const RGBA& color = mColors[index];
			const bool light = color.r * 0.3f + color.g * 0.59f + color.b * 0.11f > 0.55f;
			painter.text(
				mCaptions[index],
				cell,
				style.metrics.smallFontSize,
				TextAlign::Center,
				light ? RGBA(0, 0, 0, 0.8f) : RGBA(1, 1, 1, 0.9f)
			);
		}
	}
}
