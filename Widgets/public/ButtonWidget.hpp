#pragma once

#include "LabelWidget.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class ButtonWidget : public Widget<Events, Canvas> {
	public:
		ButtonWidget() {
			this->mArea = { 0, 0, 100, 100 };
			this->mId = "Button";
		}

		ButtonWidget(const std::string& label, const tp::RectF& aArea) {
			this->mId = "Button";
			this->mArea = aArea;
			this->mLabel.mLabel = label;
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
				canvas.rect(this->mArea, pressedColor, rounding);
			} else if (mIsHover) {
				canvas.rect(this->mArea, hoveredColor, rounding);
			} else {
				canvas.rect(this->mArea, accentColor, rounding);
			}
			mLabel.draw(canvas);
		}

	public:
		void setupConfig(WidgetManager& wm) {
			if (!wm.createWidgetConfig(this->mId)) return;
			wm.addReference(this->mId, "Pressed", "Action");
			wm.addReference(this->mId, "Hovered", "Interaction");
			wm.addReference(this->mId, "Default", "Accent");
			wm.addReference(this->mId, "Rounding", "Rounding");

			mLabel.setupConfig(wm);
		}

		void updateConfigCache(const WidgetManager& wm) override {
			pressedColor = wm.getColor(this->mId, "Pressed");
			hoveredColor = wm.getColor(this->mId, "Hovered");
			accentColor = wm.getColor(this->mId, "Default");
			rounding = wm.getNumber(this->mId, "Rounding");

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
	};
}