#pragma once

#include "LabelWidget.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class ButtonWidget : public Widget<Events, Canvas> {
	public:
		ButtonWidget() {
			this->mArea = { 0, 0, 100, 100 };
			this->createConfig("Button");
			this->addColor("Pressed", "Action");
			this->addColor("Hovered", "Interaction");
			this->addColor("Default", "Accent");
			this->addValue("Rounding", "Rounding");
		}

		ButtonWidget(const std::string& label, const tp::RectF& aArea) {
			this->mArea = aArea;
			this->mLabel.mLabel = label;
			
			this->createConfig("Button");
			this->addColor("Pressed", "Action");
			this->addColor("Hovered", "Interaction");
			this->addColor("Default", "Accent");
			this->addValue("Rounding", "Rounding");
		}

		void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) override {
			this->mArea = aArea;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;

			mIsHover = false;

			if (!areaParent.isOverlap(aArea)) {
				mIsReleased = false;
				mIsPressed = false;
				return;
			}

			mIsHover = aArea.isInside(events.getPointer());

			if (events.isPressed(InputID::MOUSE1) && mIsHover) {
				mIsPressed = true;
			}

			if (mIsPressed && mIsHover && events.isReleased(InputID::MOUSE1)) {
				mIsReleased = true;
				mIsPressed = false;
			}

			if (!mIsHover) mIsPressed = false;

			mLabel.proc(events, aArea, aArea);
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;

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