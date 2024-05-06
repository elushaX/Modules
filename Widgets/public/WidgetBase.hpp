#pragma once

#include "EventHandler.hpp"
#include "WidgetManager.hpp"
#include "List.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class Widget {
	public:
		Widget() { mArea = { 0, 0, 100, 100 }; }

		virtual ~Widget() = default;

		virtual void procWrapper(const Events& events) {
			if (!mEnable) return;
			if (!mVisible) return;

			procCallback(events);

			for (auto child : mChildWidgets) {
				const bool isChildVisible = getArea().isOverlap(child->getArea());
				child->setVisible(isChildVisible);
				child->procWrapper(events);
			}
		}

		virtual void drawWrapper(Canvas& canvas) {
			if (!mEnable) return;
			if (!mVisible) return;

			drawCallback(canvas);

			canvas.pushClamp(this->mArea);

			for (auto child : mChildWidgets) {
				child->drawWrapper(canvas);
			}

			canvas.popClamp();
		}

		virtual void updateConfigWrapper(WidgetManager& wm) {
			wm.setActiveId("Global");

			updateConfigCallback(wm);

			for (auto child : mChildWidgets) {
				child->updateConfigWrapper(wm);
			}
		}

		virtual void procCallback(const Events& events) {}
		virtual void drawCallback(Canvas& canvas) {}
		virtual void updateConfigCallback(WidgetManager& wm) {}

	public:
		void setEnable(bool enable) { mEnable = enable; }
		void setVisible(bool visible) { mVisible = visible; }
		const RectF& getArea() const { return mArea; }
		void setArea(const RectF& area) { mArea = area; }

	public:
		RectF mArea;
		List<Widget<Events, Canvas>*> mChildWidgets;
		bool mVisible = false;
		bool mEnable = true;
	};
}