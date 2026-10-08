#include "AppDriver.hpp"

#include "Widget.hpp"

using namespace tp;
using namespace tp::ui;

AppDriver::AppDriver(WidgetApplication& application) :
	mApplication(application),
	mInput(application.window().injectInput()) {
	mApplication.window().setPlatformInputEnabled(false);
	mApplication.start();
	mInput.onKeyboardFocus(true);
	frames(2);
}

AppDriver::~AppDriver() {
	mApplication.finish();
	mApplication.window().setPlatformInputEnabled(true);
}

void AppDriver::frame(halnf deltaMs) { mApplication.step(deltaMs); }

void AppDriver::frames(int count) {
	for (int index = 0; index < count; index++)
		frame();
}

void AppDriver::advance(halnf milliseconds) {
	while (milliseconds > 0) {
		const halnf delta = milliseconds < kFrameMs ? milliseconds : kFrameMs;
		frame(delta);
		milliseconds -= delta;
	}
}

void AppDriver::moveTo(Vec2F windowPoint) {
	mInput.onPointerMoved(windowPoint);
	frame();
}

void AppDriver::press(InputID button) {
	mInput.onPointerButton(button, true);
	frame();
}

void AppDriver::release(InputID button) {
	mInput.onPointerButton(button, false);
	frame();
}

void AppDriver::click(Vec2F windowPoint, InputID button) {
	moveTo(windowPoint);
	press(button);
	release(button);
	advance(kClickGapMs);
}

void AppDriver::click(const Widget& widget) {
	settle(widget);
	click(centerOf(widget));
}

void AppDriver::settle(const Widget& widget) {
	for (int index = 0; index < kSettleFrameLimit; index++) {
		frame();
		if (!isMoving(widget)) return;
	}
}

bool AppDriver::isMoving(const Widget& widget) {
	for (const Widget* current = &widget; current; current = current->parent()) {
		if (current->isGeometryMoving() || current->isTransitioning()) return true;
	}
	return false;
}

void AppDriver::penStroke(const std::vector<Vec2F>& points, const std::vector<PointerSample>& samples) {
	if (points.empty() || points.size() != samples.size()) return;

	mInput.onPointerSample(samples.front());
	mInput.onPointerMoved(points.front());
	mInput.onPointerButton(InputID::MOUSE1, true);
	frame();
	for (size_t index = 1; index < points.size(); index++) {
		mInput.onPointerSample(samples[index]);
		mInput.onPointerMoved(points[index]);
		frame();
	}
	mInput.onPointerButton(InputID::MOUSE1, false);
	mInput.onPointerSample(PointerSample{});
	frame();
}

void AppDriver::drag(Vec2F from, Vec2F to, int steps) {
	moveTo(from);
	press();
	for (int step = 1; step <= steps; step++) {
		const halnf amount = (halnf) step / (halnf) steps;
		moveTo(from + (to - from) * amount);
	}
	release();
	advance(kClickGapMs);
}

void AppDriver::scroll(Vec2F windowPoint, halnf amount) {
	moveTo(windowPoint);
	mInput.onScrolled(Vec2F(0.0f, amount));
	frame();
}

void AppDriver::keyDown(InputID key) {
	mInput.onKey(key, true);
	frame();
}

void AppDriver::keyUp(InputID key) {
	mInput.onKey(key, false);
	frame();
}

void AppDriver::key(InputID key) {
	keyDown(key);
	keyUp(key);
}

void AppDriver::shortcut(InputID modifier, InputID key) {
	keyDown(modifier);
	this->key(key);
	keyUp(modifier);
}

void AppDriver::type(const std::string& utf8) {
	mInput.onText(utf8);
	frame();
}

Vec2F AppDriver::centerOf(const Widget& widget) { return widget.windowRect().center(); }
