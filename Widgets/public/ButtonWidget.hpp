#pragma once

#include "LabelWidget.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class ButtonWidget : public Widget<Events, Canvas> {
	public:
		ButtonWidget() {
			this->createConfig("Button");
			this->addColor("Pressed", "Action");
			this->addColor("Hovered", "Interaction");
			this->addColor("Default", "Accent");
			this->addValue("Rounding", "Rounding");
		}

		void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) override {
			this->mArea = aArea;
			mIsHover = false;

			if (!areaParent.isOverlap(aArea)) {
				mIsReleased = false;
				mIsPressed = false;
				return;
			}

			mIsHover = aArea.isInside(events.getPos());

			if (events.isPressed() && mIsHover) {
				mIsPressed = true;
			}

			if (mIsPressed && mIsHover && events.isReleased()) {
				mIsReleased = true;
				mIsPressed = false;
			}

			if (!mIsHover) mIsPressed = false;

			mLabel.proc(events, aArea, aArea);
		}

		void draw(Canvas& canvas) const override {
			if (mIsPressed) {
				canvas.rect(this->mArea, this->getColor("Pressed"), this->getValue("Rounding"));
			} else if (mIsHover) {
				canvas.rect(this->mArea, this->getColor("Hovered"), this->getValue("Rounding"));
			} else {
				canvas.rect(this->mArea, this->getColor("Default"), this->getValue("Rounding"));
			}
			mLabel.draw(canvas);
		}

	public:
		LabelWidget<Events, Canvas> mLabel;
		bool mIsHover = false;
		bool mIsPressed = false;
		bool mIsReleased = false;
	};
}