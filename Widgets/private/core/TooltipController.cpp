#include "TooltipController.hpp"

#include "Label.hpp"
#include "Ui.hpp"

using namespace tp;
using namespace tp::ui;

TooltipController::TooltipController(Ui& ui) :
	mUi(ui),
	mBubble(std::make_unique<TooltipBubble>()) {}

TooltipController::~TooltipController() = default;

void TooltipController::onHoverChanged(Widget* hovered) {
	Widget* candidate = hovered;
	while (candidate && candidate->tooltip().empty())
		candidate = candidate->parent();

	if (candidate == mCandidate) return;

	hide();
	mCandidate = candidate;
	mDwellMs = 0;
}

void TooltipController::onPointerActivity() {
	if (mPinned) return;
	hide();
	mDwellMs = 0;
}

void TooltipController::onPress() {
	mPinned = false;
	onPointerActivity();
}

bool TooltipController::showFor(Widget& widget) {
	Widget* candidate = &widget;
	while (candidate && candidate->tooltip().empty())
		candidate = candidate->parent();
	if (!candidate) return false;
	hide();
	mCandidate = candidate;
	show();
	mPinned = true;
	return true;
}

void TooltipController::advance(halnf deltaMs) {
	if (!mCandidate || isShown() || mUi.isTouchMode()) return;

	mDwellMs += deltaMs;
	if (mDwellMs >= mUi.theme().metrics.tooltipDelayMs) show();
}

void TooltipController::forget(const Widget& widget) {
	if (mCandidate && widget.isSelfOrAncestorOf(*mCandidate)) {
		hide();
		mCandidate = nullptr;
	}
}

bool TooltipController::isShown() const { return mUi.isPopupOpen(*mBubble); }

Widget* TooltipController::bubble() const { return isShown() ? mBubble.get() : nullptr; }

void TooltipController::show() {
	mBubble->setText(mCandidate->tooltip());

	PopupRequest request;
	request.content = mBubble.get();
	request.owner = nullptr;
	request.anchor = mCandidate->windowRect();
	request.options.placement = Placement::Below;
	request.options.align = PlacementAlign::Center;
	request.options.gap = mUi.theme().metrics.popupGap * 2;
	request.options.closeOnOutsidePress = true;
	request.options.interactive = false;

	mUi.openPopup(request);
}

void TooltipController::hide() {
	mPinned = false;
	if (isShown()) mUi.closePopup(*mBubble);
}
