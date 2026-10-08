#include "InputRouter.hpp"

#include "PopupStack.hpp"
#include "TooltipController.hpp"
#include "Ui.hpp"
#include "WidgetInternals.hpp"
#include "WindowHost.hpp"

#include <algorithm>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kMultiClickIntervalMs = 400;
	constexpr halnf kMultiClickDistance = 4;
}

InputRouter::InputRouter(Ui& ui, PopupStack& popups, TooltipController& tooltips, Diagnostics& diagnostics) :
	mUi(ui),
	mPopups(popups),
	mTooltips(tooltips),
	mDiagnostics(diagnostics) {}

void InputRouter::dispatch(Event event) {
	if (event.isPointerEvent()) {
		mPointer = event.windowPosition();
		event.withButtonsDown(mButtonsDown[0], mButtonsDown[1], mButtonsDown[2]);
	}

	switch (event.type()) {
		case Event::Type::PointerMove: handlePointerMove(event); break;
		case Event::Type::PointerDown: handlePointerDown(event); break;
		case Event::Type::PointerUp: handlePointerUp(event); break;
		case Event::Type::Wheel: handleWheel(event); break;
		case Event::Type::KeyDown:
		case Event::Type::KeyUp:
		case Event::Type::TextInput: handleKey(event); break;
		case Event::Type::Gesture: handleGesture(event); break;
		case Event::Type::Click:
		case Event::Type::LongPress: break;
	}
}

void InputRouter::advanceClock(halnf deltaMs) {
	mClockMs += deltaMs;
	if (mLongPressArmed && mClockMs >= mLongPressDueMs) fireLongPress();
}

void InputRouter::armLongPress(const Event& event) {
	mLongPressConsumed = false;
	mLongPressArmed = event.button() == PointerButton::Primary && mPressed != nullptr;
	mLongPressDueMs = mClockMs + Ui::kLongPressMs;
	mLongPressOrigin = event.windowPosition();
	mLongPressModifiers = event.modifiers();
	mLongPressSample = event.sample();
}

void InputRouter::fireLongPress() {
	mLongPressArmed = false;
	if (!mPressed) return;

	Event longPress = Event::pointer(Event::Type::LongPress, mLongPressOrigin, { 0, 0 }, PointerButton::Primary)
											.withModifiers(mLongPressModifiers)
											.withSample(mLongPressSample)
											.withButtonsDown(mButtonsDown[0], mButtonsDown[1], mButtonsDown[2]);
	bubble(longPress, mPressed);
	mLongPressConsumed = longPress.isAccepted() || (mPressed && mTooltips.showFor(*mPressed));
}

void InputRouter::refreshHover() {
	updateHover(mCapture ? mCapture : widgetAt(mPointer));
	updatePressed();
}

Widget* InputRouter::pressedWidget() const {
	if (!mButtonsDown[(int) PointerButton::Primary] || !mPressed) return nullptr;
	if (mCapture) return mCapture;
	return mHovered && mPressed->isSelfOrAncestorOf(*mHovered) ? mPressed : nullptr;
}

void InputRouter::capture(Widget& widget) {
	mCapture = &widget;
	if (mPressed && mPressed != &widget) {
		mPressed = nullptr;
		updatePressed();
	}
}

void InputRouter::updatePressed() {
	const std::vector<Widget*> previousPath = mPressedPath;
	Widget* pressed = pressedWidget();

	mPressedPath.clear();
	if (pressed) {
		const std::vector<Widget*> hoverPath =
			pathTo(mHovered && pressed->isSelfOrAncestorOf(*mHovered) ? mHovered : pressed);
		for (Widget* widget : hoverPath) {
			if (pressed->isSelfOrAncestorOf(*widget)) mPressedPath.push_back(widget);
		}
	}

	updatePathFlags(previousPath, mPressedPath, WidgetInternals::setPressed);
}

void InputRouter::forget(const Widget& widget) {
	auto detaches = [&widget](Widget* tracked) { return tracked && widget.isSelfOrAncestorOf(*tracked); };

	if (detaches(mHovered)) {
		for (Widget* iterator = mHovered; iterator; iterator = iterator->parent()) {
			WidgetInternals::setHovered(*iterator, false);
		}
		mHovered = nullptr;
	}

	if (detaches(mPressed)) mPressed = nullptr;
	if (detaches(mCapture)) mCapture = nullptr;
	if (detaches(mGestureTarget)) mGestureTarget = nullptr;

	const auto inSubtree = [&widget](Widget* tracked) { return widget.isSelfOrAncestorOf(*tracked); };
	for (Widget* tracked : mPressedPath) {
		if (inSubtree(tracked)) WidgetInternals::setPressed(*tracked, false);
	}
	mPressedPath.erase(std::remove_if(mPressedPath.begin(), mPressedPath.end(), inSubtree), mPressedPath.end());
	if (detaches(mKeyboardFocus)) mKeyboardFocus = nullptr;

	const auto inDetachedSubtree = [&widget](Widget* tracked) { return widget.isSelfOrAncestorOf(*tracked); };
	if (std::any_of(mActivePath.begin(), mActivePath.end(), inDetachedSubtree)) {
		const std::vector<Widget*> previousPath = mActivePath;
		const auto detached = std::find_if(previousPath.begin(), previousPath.end(), inDetachedSubtree);

		mActivePath.assign(previousPath.begin(), detached);
		mActive = mActivePath.empty() ? nullptr : mActivePath.back();
		updatePathFlags(previousPath, mActivePath, WidgetInternals::setContainsActive);
	}
}

void InputRouter::release(const Widget& widget) {
	if (mCapture == &widget) mCapture = nullptr;
	updatePressed();
}

void InputRouter::setKeyboardFocus(Widget* widget) {
	if (mKeyboardFocus == widget) return;

	Widget* previous = mKeyboardFocus;
	mKeyboardFocus = widget;

	if (previous) WidgetInternals::notifyKeyboardFocusChanged(*previous);
	if (widget) WidgetInternals::notifyKeyboardFocusChanged(*widget);
}

void InputRouter::setActiveWidget(Widget* widget) {
	if (widget == mActive) return;

	const std::vector<Widget*> previousPath = mActivePath;
	mActive = widget;
	mActivePath = ownershipPathTo(widget);
	updatePathFlags(previousPath, mActivePath, WidgetInternals::setContainsActive);
}

std::vector<Widget*> InputRouter::ownershipPathTo(Widget* widget) const {
	std::vector<Widget*> path;
	Widget* iterator = widget;

	while (iterator) {
		std::vector<Widget*> segment = pathTo(iterator);
		path.insert(path.begin(), segment.begin(), segment.end());

		const long popup = mPopups.indexOf(*segment.front());
		iterator = popup >= 0 ? mPopups.entries()[(size_t) popup].request.owner : nullptr;
	}

	return path;
}

Widget* InputRouter::widgetAt(Vec2F windowPoint) const {
	const auto& entries = mPopups.entries();

	for (auto entry = entries.rbegin(); entry != entries.rend(); ++entry) {
		if (!entry->request.options.interactive) continue;
		if (Widget* hit = hitTestSubtree(*entry->request.content, windowPoint)) return hit;
		if (entry->request.options.modal) return nullptr;
	}

	if (!mUi.root()) return nullptr;
	return hitTestSubtree(*mUi.root(), windowPoint);
}

void InputRouter::handlePointerMove(Event& event) {
	mTooltips.onPointerActivity();
	if (mLongPressArmed) {
		const Vec2F offset = event.windowPosition() - mLongPressOrigin;
		if (offset.x * offset.x + offset.y * offset.y > kMultiClickDistance * kMultiClickDistance) mLongPressArmed = false;
	}
	updateHover(mCapture ? mCapture : widgetAt(mPointer));
	updatePressed();
	bubble(event, pointerTarget());
	applyCursor();
}

int InputRouter::countClick(const Event& event) {
	const Vec2F offset = event.windowPosition() - mLastPressPosition;
	const bool nearby = offset.x * offset.x + offset.y * offset.y <= kMultiClickDistance * kMultiClickDistance;
	const bool quick = mClockMs - mLastPressMs <= kMultiClickIntervalMs;
	const bool sameButton = event.button() == mLastPressButton;

	mClickCount = nearby && quick && sameButton ? mClickCount + 1 : 1;
	mLastPressMs = mClockMs;
	mLastPressPosition = event.windowPosition();
	mLastPressButton = event.button();
	return mClickCount;
}

void InputRouter::applyCursor() {
	Widget* target = pointerTarget();
	CursorShape cursor = CursorShape::Default;
	for (Widget* widget = target; widget; widget = widget->parent()) {
		if (widget->cursor() == CursorShape::Default) continue;
		cursor = widget->cursor();
		break;
	}

	if (cursor == mAppliedCursor) return;
	mAppliedCursor = cursor;
	if (WindowHost* host = mUi.windowHost()) host->setCursor(cursor);
}

void InputRouter::handlePointerDown(Event& event) {
	mButtonsDown[(int) event.button()] = true;
	event.withButtonsDown(mButtonsDown[0], mButtonsDown[1], mButtonsDown[2]);
	event.withClickCount(countClick(event));
	mTooltips.onPress();

	Widget* target = widgetAt(mPointer);
	dismissPopupsOutside(target);

	target = widgetAt(mPointer);
	updateHover(target);

	if (event.button() == PointerButton::Primary) mPressed = target;
	mKeyboardNavigating = false;
	updatePressed();
	setActiveWidget(target);
	setKeyboardFocus(nullptr);
	for (Widget* candidate = target; candidate; candidate = candidate->parent()) {
		if (WidgetInternals::acceptsKeyboardFocus(*candidate) && candidate->isEffectivelyEnabled()) {
			setKeyboardFocus(candidate);
			break;
		}
	}

	armLongPress(event);
	bubble(event, target);
}

void InputRouter::handlePointerUp(Event& event) {
	mButtonsDown[(int) event.button()] = false;
	event.withButtonsDown(mButtonsDown[0], mButtonsDown[1], mButtonsDown[2]);

	Widget* target = pointerTarget();
	bubble(event, target);

	if (mCapture && event.button() == PointerButton::Primary) mCapture = nullptr;

	Widget* released = widgetAt(mPointer);
	const bool primary = event.button() == PointerButton::Primary;
	const bool longPressed = primary && mLongPressConsumed;
	if (primary) {
		mLongPressArmed = false;
		mLongPressConsumed = false;
	}
	const bool isClick = primary && !longPressed && mPressed && released && mPressed->isSelfOrAncestorOf(*released);
	Widget* pressed = mPressed;
	if (event.button() == PointerButton::Primary) mPressed = nullptr;
	updatePressed();

	if (isClick) {
		Event click = Event::pointer(Event::Type::Click, mPointer, { 0, 0 }, PointerButton::Primary)
										.withModifiers(event.modifiers())
										.withButtonsDown(mButtonsDown[0], mButtonsDown[1], mButtonsDown[2])
										.withClickCount(mClickCount);
		bubble(click, pressed);
	}

	updateHover(widgetAt(mPointer));
	updatePressed();
	applyCursor();
}

void InputRouter::handleWheel(Event& event) {
	mTooltips.onPointerActivity();
	bubble(event, pointerTarget());
}

void InputRouter::handleGesture(Event& event) {
	mTooltips.onPointerActivity();
	if (event.touchGesture().phase == GesturePhase::Begin) {
		mGestureTarget = widgetAt(event.windowPosition());
		if (mGestureTarget) dismissPopupsOutside(mGestureTarget);
	}
	Widget* target = mGestureTarget;
	if (event.touchGesture().phase == GesturePhase::End) mGestureTarget = nullptr;
	bubble(event, target);
}

void InputRouter::handleKey(Event& event) {
	if (event.type() == Event::Type::KeyDown && !mKeyboardNavigating) {
		mKeyboardNavigating = true;
		if (mKeyboardFocus) mKeyboardFocus->requestPaint();
	}
	if (event.type() == Event::Type::KeyDown) mKeysDown.insert(event.keyCode());
	else if (event.type() == Event::Type::KeyUp) mKeysDown.erase(event.keyCode());

	const bool escape = event.type() == Event::Type::KeyDown && event.keyCode() == InputID::ESCAPE;

	if (
		escape && !mPopups.empty() && mPopups.entries().back().request.options.interactive &&
		mPopups.entries().back().request.options.closeOnEscape
	) {
		mPopups.closeFrom(mPopups.entries().size() - 1);
		updateHover(widgetAt(mPointer));
		return;
	}

	Widget* target = mKeyboardFocus && !isBlockedByModal(mKeyboardFocus) ? mKeyboardFocus : nullptr;
	for (auto entry = mPopups.entries().rbegin(); !target && entry != mPopups.entries().rend(); ++entry) {
		if (entry->request.options.interactive) target = entry->request.content;
	}
	if (!target && mUi.root()) target = WidgetInternals::defaultKeyTarget(*mUi.root());

	bubble(event, target);
}

void InputRouter::dismissPopupsOutside(Widget* target) {
	const long containing = target ? mPopups.indexContaining(*target) : -1;
	long firstToClose = (long) mPopups.entries().size();

	for (long index = firstToClose - 1; index > containing; index--) {
		const PopupRequest& request = mPopups.entries()[index].request;
		const bool pressedOwnTrigger = target && request.owner && request.owner->isSelfOrAncestorOf(*target);

		if (pressedOwnTrigger || !request.options.closeOnOutsidePress) break;
		firstToClose = index;
	}

	mPopups.closeFrom((size_t) firstToClose);
}

void InputRouter::updateHover(Widget* target) {
	if (target == mHovered) return;

	const std::vector<Widget*> previousPath = pathTo(mHovered);
	mHovered = target;
	updatePathFlags(previousPath, pathTo(target), WidgetInternals::setHovered);
	mTooltips.onHoverChanged(target);
}

void InputRouter::updatePathFlags(
	const std::vector<Widget*>& previousPath, const std::vector<Widget*>& nextPath, void (*setFlag)(Widget&, bool)
) {
	size_t common = 0;
	while (common < previousPath.size() && common < nextPath.size() && previousPath[common] == nextPath[common]) {
		common++;
	}

	for (size_t index = previousPath.size(); index > common; index--)
		setFlag(*previousPath[index - 1], false);
	for (size_t index = common; index < nextPath.size(); index++)
		setFlag(*nextPath[index], true);
}

void InputRouter::bubble(Event& event, Widget* target) {
	if (!target || isBlockedByModal(target)) return;

	const std::vector<Widget*> path = pathTo(target);

	event.mTarget = target;
	for (size_t index = 0; index + 1 < path.size(); index++) {
		WidgetInternals::filter(*path[index], event);
		if (event.isAccepted()) return;
	}

	for (auto receiver = path.rbegin(); receiver != path.rend(); ++receiver) {
		if (!(*receiver)->isEffectivelyEnabled()) continue;

		deliverTo(event, **receiver, target);
		if (event.isAccepted()) return;
	}
}

void InputRouter::deliverTo(Event& event, Widget& receiver, Widget* target) {
	event.mTarget = target;
	event.mReceiver = &receiver;
	mDiagnostics.stats.deliveredEvents++;
	if (mDiagnostics.observer) mDiagnostics.observer->onEventDelivering(receiver, event);
	WidgetInternals::deliver(receiver, event);
	if (mDiagnostics.observer) mDiagnostics.observer->onEventDelivered(receiver, event);
}

Widget* InputRouter::pointerTarget() const { return mCapture ? mCapture : mHovered; }

bool InputRouter::isBlockedByModal(const Widget* target) const {
	const long modal = mPopups.topmostModal();
	if (modal < 0 || !target) return false;
	return mPopups.indexContaining(*target) < modal;
}

Widget* InputRouter::hitTestSubtree(Widget& widget, Vec2F windowPoint) {
	if (!widget.isVisible()) return nullptr;

	const Vec2F local = widget.toLocal(windowPoint);
	if (!contains(widget.localRect(), local)) return nullptr;

	const auto& children = widget.children();
	for (auto child = children.rbegin(); child != children.rend(); ++child) {
		if (Widget* hit = hitTestSubtree(**child, windowPoint)) return hit;
	}

	return WidgetInternals::hitTest(widget, local) ? &widget : nullptr;
}

std::vector<Widget*> InputRouter::pathTo(Widget* widget) {
	std::vector<Widget*> path;
	for (Widget* iterator = widget; iterator; iterator = iterator->parent())
		path.push_back(iterator);
	std::reverse(path.begin(), path.end());
	return path;
}
