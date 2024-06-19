#pragma once

#include "CollapsableMenu.hpp"

namespace tp {
	template <typename Events, typename Canvas>
	class FloatingWidget : public CollapsableMenu<Events, Canvas> {
	public:
		FloatingWidget() {
			this->mArea = { 0, 0, 300, 300 };
		}

		virtual void procWrapper(const Events& events, const RectF& parentArea) override {
			if (!this->mEnable) return;

			this->checkVisibility(events, parentArea);

			if (!this->mVisible) return;

			this->checkFocus(events);

			this->checkClicked(events);

			mParentArea = parentArea;
			this->eventProcess(events);

			for (auto child : this->mChildWidgets) {
				child->procWrapper(events, this->getArea());
			}
		}

		void eventProcess(const Events& events) override {
			CollapsableMenu<Events, Canvas>::eventProcess(events);

			if (this->mHeader.isHolding()) {
				this->mArea.pos += events.getPointerDelta();
				if (!this->mArea.isEnclosedIn(mParentArea)) {
					this->mArea.pos -= events.getPointerDelta();
				}
			}
		}

		RectF mParentArea;

	};
}