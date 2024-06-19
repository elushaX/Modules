#pragma once

#include "LabelWidget.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class ButtonWidget : public Widget<Events, Canvas> {
	public:
		ButtonWidget() {
			this->setArea({ 0, 0, 100, 30 });
			this->mChildWidgets.pushBack(&mLabel);
		}

		ButtonWidget(const std::string& label, const tp::RectF& aArea) {
			this->setArea(aArea);
			mLabel.mLabel = label;
			this->mChildWidgets.pushBack(&mLabel);
		}

		bool isFired() { return this->isReleased(); }

		void eventProcess(const Events&) override { mLabel.setArea(this->mArea); }

		void eventDraw(Canvas& canvas) override {
			if (this->isHolding()) {
				canvas.rect(this->mArea, pressedColor, rounding);
			} else if (this->isFocus()) {
				canvas.rect(this->mArea, hoveredColor, rounding);
			} else {
				canvas.rect(this->mArea, accentColor, rounding);
			}
		}

		void setLabel(const std::string& string) {
			mLabel.mLabel = string;
		}

	public:
		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("Button");

			pressedColor = wm.getColor("Pressed", "Action");
			hoveredColor = wm.getColor("Hovered", "Interaction");
			accentColor = wm.getColor("Default", "Accent");
			rounding = wm.getNumber("Rounding", "Rounding");
		}

	public:
		LabelWidget<Events, Canvas> mLabel;

		RGBA pressedColor;
		RGBA hoveredColor;
		RGBA accentColor;
		halnf rounding = 0;
	};
}