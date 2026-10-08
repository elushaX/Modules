#include "UiDriver.hpp"

#include "Slider.hpp"

#include <vector>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

UiDriver::UiDriver(Widget& root, Vec2F viewportSize) {
	mUi.setTextMetrics(&mTextMetrics);
	mUi.setViewport(makeRect({ 0, 0 }, viewportSize));
	mUi.setRoot(&root);
	mUi.updateLayout();
}

void UiDriver::finishLayoutAnimations() {
	if (!mAutoSettle) return;

	for (int pass = 0; pass < 8; pass++) {
		mUi.updateLayout();
		std::vector<Widget*> moving;
		for (Widget* widget : mUi.animatingWidgets()) {
			if (widget->isGeometryMoving() || widget->isTransitioning()) moving.push_back(widget);
		}
		if (moving.empty()) return;
		for (Widget* widget : moving)
			widget->finishGeometryAnimation();
	}
	mUi.updateLayout();
}

UiDriver::~UiDriver() {
	mUi.closeAllPopups();
	mUi.setRoot(nullptr);
}

void UiDriver::layout() {
	mUi.updateLayout();
	finishLayoutAnimations();
}

void UiDriver::resize(Vec2F viewportSize) {
	mUi.setViewport(makeRect({ 0, 0 }, viewportSize));
	finishLayoutAnimations();
}

void UiDriver::moveTo(Vec2F windowPoint) {
	const Vec2F delta = windowPoint - mPointer;
	mPointer = windowPoint;
	mUi.dispatch(
		Event::pointer(Event::Type::PointerMove, windowPoint, delta, PointerButton::Primary).withSample(mSample)
	);
	finishLayoutAnimations();
}

void UiDriver::moveTo(const Widget& widget) { moveTo(centerOf(widget)); }

void UiDriver::press(PointerButton button, Modifiers modifiers) {
	mUi.dispatch(
		Event::pointer(Event::Type::PointerDown, mPointer, { 0, 0 }, button).withModifiers(modifiers).withSample(mSample)
	);
	finishLayoutAnimations();
}

void UiDriver::release(PointerButton button, Modifiers modifiers) {
	mUi.dispatch(
		Event::pointer(Event::Type::PointerUp, mPointer, { 0, 0 }, button).withModifiers(modifiers).withSample(mSample)
	);
	finishLayoutAnimations();
}

void UiDriver::click(Vec2F windowPoint, Modifiers modifiers) {
	moveTo(windowPoint);
	press(PointerButton::Primary, modifiers);
	release(PointerButton::Primary, modifiers);
}

void UiDriver::click(const Widget& widget, Modifiers modifiers) { click(centerOf(widget), modifiers); }

void UiDriver::setSlider(Slider& slider, halnf value) {
	settle();
	click(slider.windowPointForValue(value));
}

void UiDriver::drag(Vec2F from, Vec2F to, int steps) {
	moveTo(from);
	press();
	for (int step = 1; step <= steps; step++) {
		const halnf amount = (halnf) step / (halnf) steps;
		moveTo(from + (to - from) * amount);
	}
	release();
}

void UiDriver::swipe(Vec2F from, Vec2F to, halnf durationMs, int steps) {
	moveTo(from);
	press();
	for (int step = 1; step <= steps; step++) {
		advance(durationMs / (halnf) steps);
		moveTo(from + (to - from) * ((halnf) step / (halnf) steps));
	}
	release();
}

void UiDriver::wheel(halnf amount) {
	mUi.dispatch(Event::wheel(mPointer, amount));
	finishLayoutAnimations();
}

void UiDriver::horizontalWheel(halnf amount) {
	mUi.dispatch(Event::wheel(mPointer, 0, amount));
	finishLayoutAnimations();
}

void UiDriver::gesture(const TouchGesture& gesture) {
	mUi.dispatch(Event::gesture(gesture));
	finishLayoutAnimations();
}

void UiDriver::key(InputID key, Modifiers modifiers) { holdKey(key, 0, modifiers); }

void UiDriver::holdKey(InputID key, int repeats, Modifiers modifiers) {
	mUi.dispatch(Event::key(Event::Type::KeyDown, key).withModifiers(modifiers));
	for (int repeat = 0; repeat < repeats; repeat++)
		mUi.dispatch(Event::keyRepeat(key).withModifiers(modifiers));
	mUi.dispatch(Event::key(Event::Type::KeyUp, key).withModifiers(modifiers));
	finishLayoutAnimations();
}

void UiDriver::pressKey(InputID key, Modifiers modifiers) {
	mUi.dispatch(Event::key(Event::Type::KeyDown, key).withModifiers(modifiers));
	finishLayoutAnimations();
}

void UiDriver::releaseKey(InputID key, Modifiers modifiers) {
	mUi.dispatch(Event::key(Event::Type::KeyUp, key).withModifiers(modifiers));
	finishLayoutAnimations();
}

void UiDriver::type(const std::string& text) {
	mUi.dispatch(Event::text(text));
	finishLayoutAnimations();
}

void UiDriver::advance(halnf deltaMs) {
	mClockMs += deltaMs;
	mSample.timeMs = (uint32_t) mClockMs;
	while (deltaMs > 0) {
		const halnf step = deltaMs < kFrameMs ? deltaMs : kFrameMs;
		mUi.advance(step);
		deltaMs -= step;
	}
}

void UiDriver::settle(halnf maximumMs) {
	const int maximumFrames = (int) (maximumMs / kFrameMs);

	for (int frame = 0; frame < maximumFrames && mUi.needsFrame(); frame++) {
		mClockMs += kFrameMs;
		mSample.timeMs = (uint32_t) mClockMs;
		mUi.advance(kFrameMs);
		mUi.paint(mPainter);
	}
	mPainter.clear();
}

void UiDriver::hoverFor(const Widget& widget, halnf durationMs) {
	moveTo(widget);
	advance(durationMs);
}

RecordingPainter& UiDriver::paint() {
	mPainter.clear();
	mUi.paint(mPainter);
	return mPainter;
}

Vec2F UiDriver::centerOf(const Widget& widget) {
	finishLayoutAnimations();
	mUi.updateLayout();
	return widget.windowRect().center();
}
