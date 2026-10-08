#include "Control.hpp"

#include "Event.hpp"
#include "Theme.hpp"

#include <algorithm>

using namespace tp;
using namespace tp::ui;

Control::Control() :
	mBackground(Theme::standard().palette.control) {}

void Control::onAttached() {
	mBackground.jumpTo(backgroundTarget());
	for (AnimatedFloat* value : mRunningFloats)
		value->finish();
	mRunningFloats.clear();
	stopTicking();
}

void Control::activate() {
	if (!isEffectivelyEnabled()) return;

	onActivated();
	if (mOnActivate) mOnActivate();
}

void Control::onEvent(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown:
		case Event::Type::PointerUp:
			if (event.button() != PointerButton::Primary) return;
			refreshVisualState();
			event.accept();
			break;

		case Event::Type::Click:
			if (mOnClicked && isEffectivelyEnabled()) mOnClicked(event);
			else activate();
			event.accept();
			break;

		case Event::Type::LongPress:
			if (!mOnLongPress || !isEffectivelyEnabled()) return;
			mOnLongPress();
			event.accept();
			break;

		case Event::Type::KeyDown:
			if (event.keyCode() != InputID::ENTER && event.keyCode() != InputID::SPACE) return;
			activate();
			event.accept();
			break;

		default: break;
	}
}

void Control::onTick(halnf deltaMs) {
	bool animating = mBackground.advance(deltaMs);

	for (AnimatedFloat* value : mRunningFloats)
		animating = value->advance(deltaMs) || animating;
	mRunningFloats.erase(
		std::remove_if(
			mRunningFloats.begin(), mRunningFloats.end(), [](AnimatedFloat* value) { return !value->isAnimating(); }
		),
		mRunningFloats.end()
	);

	if (!animating) stopTicking();
	requestPaint();
}

void Control::onHoverChanged() { refreshVisualState(); }

void Control::onThemeChanged() { refreshVisualState(); }

void Control::onPressedChanged() { refreshVisualState(); }

void Control::onOwnedPopupChanged() { refreshVisualState(); }

RGBA Control::backgroundTarget() const {
	const Palette& palette = theme().palette;

	if (!isEffectivelyEnabled()) return Palette::mix(palette.control, palette.window, 0.5f);
	if (isPressed()) return palette.controlPressed;
	if (isHovered()) return palette.controlHovered;
	return palette.control;
}

void Control::refreshVisualState() {
	mBackground.animateTo(backgroundTarget());
	runAnimations();
}

void Control::animate(AnimatedFloat& value, halnf target) {
	value.animateTo(target);
	if (std::find(mRunningFloats.begin(), mRunningFloats.end(), &value) == mRunningFloats.end()) {
		mRunningFloats.push_back(&value);
	}
	runAnimations();
}

void Control::runAnimations() {
	startTicking();
	requestPaint();
}
