#include "DockspaceWidget.hpp"

using namespace tp;

void DockSpaceWidget::eventProcess(const tp::Events& events) {

	if (this->mChildWidgets.size() > 1 && events.isPressed(InputID::MOUSE1)) {

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
				if (activeChild != child.data()) child->mHandlesEvents = false;
			}
		}
	}
}

void DockSpaceWidget::eventDraw(Canvas& canvas) {
	canvas.rect(this->mArea, RGBA(0, 0, 0, 1), 0);
}
