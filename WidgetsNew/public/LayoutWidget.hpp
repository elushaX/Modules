#pragma once

#include "Widget.hpp"

namespace tp {
	class DesktopLayout : public Widget {
	public:
		DesktopLayout() { setDebug("desktop layout", { 0, 0, 0.9, 0.9 }); }

		void draw(Canvas& canvas) override {
			// canvas.rect(getRelativeArea(), RGBA(0, 0, 0, 1));
		}

		void adjustRect() override {
			// do nothing
		}

		void adjustChildrenRect() override {
			// todo : adjust by viewport pos
		}

		[[nodiscard]] bool processesEvents() const override { return false; }
	};


	class VerticalLayout : public Widget {
	public:
		VerticalLayout() {
			setDebug("vertical", { 0.8, 0.1, 0.1, 0.9 });
		}

		void draw(Canvas& canvas) override {
			// canvas.frame(getRelativeArea(), RGBA(1));
		}

		void adjustChildrenRect() override {
			auto content = &mChildren;
			auto area = getRelativeArea();

			auto iterRect = area.shrink(mPadding);
			iterRect.height = ((iterRect.height + mGap) / (float) content->size()) - mGap;

			for (auto widget :(*content)) {
				widget->setArea(iterRect);
				iterRect.y += iterRect.height + mGap;
			}
		}

		[[nodiscard]] bool processesEvents() const override { return false; }

	private:
		halnf mGap = 10;
		halnf mPadding = 10;
	};

}