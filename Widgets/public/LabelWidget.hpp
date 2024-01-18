#pragma once

#include "WidgetBase.hpp"

namespace tp {
	template <typename Events, typename Canvas>
	class LabelWidget : public Widget<Events, Canvas> {
	public:
		LabelWidget() {
			this->createConfig("Label");
			this->addValue("Size", "FontSize");
			this->addValue("Padding", "Padding");
			this->addColor("Default", "Front");
		}

		void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) override {
			this->mArea = aArea;
		}

		void draw(Canvas& canvas) const override {
			canvas.text(
				mLabel.read(),
				this->mArea,
				this->getValue("Size"),
				Canvas::CC,
				this->getValue("Padding"),
				this->getColor("Default")
			);
		}

	public:
		String mLabel = "Label";
	};
}