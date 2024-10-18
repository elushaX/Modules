#include "FloatingLayoutWidget.hpp"

using namespace tp;

FloatingLayoutWidget::FloatingLayoutWidget()  = default;

void FloatingLayoutWidget::eventProcess(const tp::Events& events) {
	updateActiveWindow(events);
}

void FloatingLayoutWidget::updateActiveWindow(const tp::Events& events) {
	mIsPassThrough = true;
	for (auto childNode = this->mChildWidgets.firstNode(); childNode; childNode = childNode->next) {
		auto child = childNode->data;
		if (child->mArea.isInside(events.getPointer())) {
			mIsPassThrough = false;
		}
	}

	if (events.isPressed(InputID::MOUSE1)) {

		Widget* activeChild = nullptr;

		for (auto childNode = this->mChildWidgets.firstNode(); childNode; childNode = childNode->next) {
			auto child = childNode->data;

			if (child->mArea.isInside(events.getPointer())) {

				mChildWidgets.detach(childNode);
				mChildWidgets.pushFront(childNode);

				child->mHandlesEvents = true;
				activeChild = child;
				break;
			}
		}

		if (activeChild) {
			for (auto child : this->mChildWidgets) {
				if (activeChild != child.data() && !child->mIsDocked) child->mHandlesEvents = false;
			}
		}

	}
}

bool FloatingLayoutWidget::handlesEvent() const { return !mIsPassThrough; }
