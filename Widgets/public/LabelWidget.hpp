#pragma once

#include "WidgetBase.hpp"

namespace tp {
	template <typename Events, typename Canvas>
	class LabelWidget : public Widget<Events, Canvas> {
	public:
		LabelWidget() { this->mId = "Label"; }

		void proc(const Events&, const tp::RectF& areaParent, const tp::RectF& aArea) override {
			this->mArea = aArea;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;
			canvas.text(mLabel.c_str(), this->mArea, fontSize, Canvas::CC, padding, fontColor);
		}

	public:
		void setupConfig(WidgetManager& wm) {
			if (!wm.createWidgetConfig(this->mId)) return;
			wm.addReference(this->mId, "Size", "FontSize");
			wm.addReference(this->mId, "Padding", "Padding");
			wm.addReference(this->mId, "Default", "Front");
		}

		void updateConfigCache(const WidgetManager& wm) override {
			fontSize = wm.getNumber(this->mId, "Size");
			padding = wm.getNumber(this->mId, "Padding");
			fontColor = wm.getColor(this->mId, "Default");
		}

	public:
		std::string mLabel = "Label";

		halnf fontSize = 10;
		halnf padding = 0;
		RGBA fontColor = { 1, 1, 1, 1 };
	};
}