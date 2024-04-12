#pragma once

#include "LabelWidget.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class ButtonWidget : public Widget<Events, Canvas> {
	public:
		ButtonWidget() { this->mArea = { 0, 0, 100, 100 }; }

		ButtonWidget(const std::string& label, const tp::RectF& aArea) {
			this->mArea = aArea;
			this->mLabel.mLabel = label;
		}

		void procBody(const Events& events) {
			mIsHover = this->mArea.isInside(events.getPointer());

			if (events.isPressed(InputID::MOUSE1) && mIsHover) {
				mIsPressed = true;
			}

			if (mIsPressed && mIsHover && events.isReleased(InputID::MOUSE1)) {
				mIsReleased = true;
				mIsPressed = false;
			}

			if (!mIsHover) mIsPressed = false;

			mLabel.proc(events, this->mArea, this->mArea);
		}

		void drawBody(Canvas& canvas) override {
			if (mIsPressed) {
				canvas.rect(this->mArea, pressedColor, rounding);
			} else if (mIsHover) {
				canvas.rect(this->mArea, hoveredColor, rounding);
			} else {
				canvas.rect(this->mArea, accentColor, rounding);
			}
			mLabel.draw(canvas);
		}

	public:
		void updateConfigCache(WidgetManager& wm) override {
			wm.setActiveId("Button");

			pressedColor = wm.getColor("Pressed", "Action");
			hoveredColor = wm.getColor("Hovered", "Interaction");
			accentColor = wm.getColor("Default", "Accent");
			rounding = wm.getNumber("Rounding", "Rounding");

			// mPressEvent = wm.getEventState("Activate", { { InputID::MOUSE1, InputState::PRESSED } });

			mLabel.updateConfigCache(wm);
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

		InputState mPressEvent;
	};
}