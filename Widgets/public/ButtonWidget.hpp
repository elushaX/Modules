#pragma once

#include "LabelWidget.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class ButtonWidget : public Widget<Events, Canvas> {
	public:
		ButtonWidget() {
			this->setArea({ 0, 0, 100, 100 });
			this->mChildWidgets.pushBack(&mLabel);
		}

		ButtonWidget(const std::string& label, const tp::RectF& aArea) {
			this->setArea(aArea);
			mLabel.mLabel = label;
			this->mChildWidgets.pushBack(&mLabel);
		}

		void procCallback(const Events& events) override {
			mLabel.setArea(this->mArea);

			mIsHover = this->mArea.isInside(events.getPointer());

			if (events.isPressed(InputID::MOUSE1) && mIsHover) {
				mIsPressed = true;
			}

			if (mIsPressed && mIsHover && events.isReleased(InputID::MOUSE1)) {
				mIsReleased = true;
				mIsPressed = false;
			}

			if (!mIsHover) mIsPressed = false;
		}

		void drawCallback(Canvas& canvas) override {
			if (mIsPressed) {
				canvas.rect(this->mArea, pressedColor, rounding);
			} else if (mIsHover) {
				canvas.rect(this->mArea, hoveredColor, rounding);
			} else {
				canvas.rect(this->mArea, accentColor, rounding);
			}
		}

	public:
		void updateConfigCallback(WidgetManager& wm) override {
			wm.setActiveId("Button");

			pressedColor = wm.getColor("Pressed", "Action");
			hoveredColor = wm.getColor("Hovered", "Interaction");
			accentColor = wm.getColor("Default", "Accent");
			rounding = wm.getNumber("Rounding", "Rounding");

			// mPressEvent = wm.getEventState("Activate", { { InputID::MOUSE1, InputState::PRESSED } });
		}

	public:
		LabelWidget<Events, Canvas> mLabel;

		bool mIsHover = false;
		bool mIsPressed = false;
		bool mIsReleased = false;

		RGBA pressedColor;
		RGBA hoveredColor;
		RGBA accentColor;
		halnf rounding = 0;

		// InputState mPressEvent;
	};
}