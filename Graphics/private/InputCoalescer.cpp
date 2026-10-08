#include "InputCoalescer.hpp"

#include "WindowInput.hpp"

using namespace tp;

class InputCoalescer::Recorder : public WindowInputObserver {
public:
	explicit Recorder(InputCoalescer& coalescer) :
		mCoalescer(coalescer) {}

	void onPointerSample(const PointerSample& sample) override {
		if (tap()) tap()->onPointerSample(sample);
		mCoalescer.pointerSample(sample);
	}
	void onPointerMoved(Vec2F position) override {
		if (tap()) tap()->onPointerMoved(position);
		mCoalescer.pointerMoved(position);
	}
	void onPointerLeft() override {
		if (tap()) tap()->onPointerLeft();
		mCoalescer.pointerLeft();
	}
	void onPointerButton(InputID button, bool pressed) override {
		if (tap()) tap()->onPointerButton(button, pressed);
		mCoalescer.pointerButton(button, pressed);
	}
	void onScrolled(Vec2F delta) override {
		if (tap()) tap()->onScrolled(delta);
		mCoalescer.scrolled(delta);
	}
	void onKey(InputID key, bool pressed) override {
		if (tap()) tap()->onKey(key, pressed);
		mCoalescer.key(key, pressed);
	}
	void onKeyRepeat(InputID key) override {
		if (tap()) tap()->onKeyRepeat(key);
		mCoalescer.keyRepeat(key);
	}
	void onText(const std::string& utf8) override {
		if (tap()) tap()->onText(utf8);
		mCoalescer.text(utf8);
	}
	void onKeyboardFocus(bool focused) override {
		if (tap()) tap()->onKeyboardFocus(focused);
		mCoalescer.keyboardFocus(focused);
	}
	void onGesture(const TouchGesture& gesture) override {
		if (tap()) tap()->onGesture(gesture);
		mCoalescer.gesture(gesture);
	}

private:
	[[nodiscard]] WindowInputObserver* tap() const { return mCoalescer.tap(); }

	InputCoalescer& mCoalescer;
};

InputCoalescer::InputCoalescer() :
	mRecorder(std::make_unique<Recorder>(*this)) {}

InputCoalescer::~InputCoalescer() = default;

WindowInputObserver& InputCoalescer::recorder() { return *mRecorder; }

void InputCoalescer::push(Pending event, DeliveryPolicy policy) {
	mStatistics.received++;

	if (policy != DeliveryPolicy::Every && !mEvents.empty() && mEvents.back().kind == event.kind) {
		Pending& previous = mEvents.back();
		if (policy == DeliveryPolicy::Latest) previous.vector = event.vector;
		else previous.vector += event.vector;
		previous.sample = event.sample;
		return;
	}

	mEvents.push_back(std::move(event));
}

void InputCoalescer::pointerMoved(Vec2F position) {
	push({ Kind::Motion, position, InputID::SPACE, false, {}, mSample }, mPolicies.pointerMotion);
}

void InputCoalescer::pointerLeft() { push({ Kind::Leave }, DeliveryPolicy::Every); }

void InputCoalescer::pointerButton(InputID button, bool pressed) {
	push({ Kind::Button, { 0, 0 }, button, pressed, {}, mSample }, DeliveryPolicy::Every);
}

void InputCoalescer::scrolled(Vec2F delta) { push({ Kind::Scroll, delta }, mPolicies.scroll); }

void InputCoalescer::key(InputID key, bool pressed) {
	push({ Kind::Key, { 0, 0 }, key, pressed }, DeliveryPolicy::Every);
}

void InputCoalescer::keyRepeat(InputID key) { push({ Kind::Repeat, { 0, 0 }, key }, DeliveryPolicy::Every); }

void InputCoalescer::text(const std::string& utf8) {
	push({ Kind::Text, { 0, 0 }, InputID::SPACE, false, utf8 }, DeliveryPolicy::Every);
}

void InputCoalescer::keyboardFocus(bool focused) {
	push({ Kind::Focus, { 0, 0 }, InputID::SPACE, focused }, DeliveryPolicy::Every);
}

void InputCoalescer::gesture(const TouchGesture& gesture) {
	mStatistics.received++;
	if (
		gesture.phase == GesturePhase::Update && !mEvents.empty() && mEvents.back().kind == Kind::Gesture &&
		mEvents.back().gesture.phase == GesturePhase::Update
	) {
		accumulate(mEvents.back().gesture, gesture);
		return;
	}
	Pending event{ Kind::Gesture };
	event.gesture = gesture;
	mEvents.push_back(std::move(event));
}

void InputCoalescer::configure(const WindowConfigure& configure) {
	mStatistics.configuresReceived++;
	mConfigure = configure;
	mHasConfigure = true;
}

WindowConfigure InputCoalescer::takeConfigure() {
	mHasConfigure = false;
	mStatistics.configuresApplied++;
	return mConfigure;
}

void InputCoalescer::deliver(WindowInputObserver& observer) {
	std::vector<Pending> events;
	events.swap(mEvents);

	for (const Pending& event : events) {
		if (event.kind == Kind::Scroll && event.vector == Vec2F{ 0, 0 }) continue;
		mStatistics.delivered++;
		const bool carriesSample = event.kind == Kind::Motion || event.kind == Kind::Button;
		if (carriesSample && !(event.sample == mDeliveredSample)) {
			mDeliveredSample = event.sample;
			observer.onPointerSample(event.sample);
		}
		switch (event.kind) {
			case Kind::Motion: observer.onPointerMoved(event.vector); break;
			case Kind::Leave: observer.onPointerLeft(); break;
			case Kind::Button: observer.onPointerButton(event.id, event.flag); break;
			case Kind::Scroll: observer.onScrolled(event.vector); break;
			case Kind::Key: observer.onKey(event.id, event.flag); break;
			case Kind::Repeat: observer.onKeyRepeat(event.id); break;
			case Kind::Text: observer.onText(event.text); break;
			case Kind::Focus: observer.onKeyboardFocus(event.flag); break;
			case Kind::Gesture: observer.onGesture(event.gesture); break;
		}
	}
}
