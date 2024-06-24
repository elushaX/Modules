#pragma once

#include "ScrollableWidget.hpp"
#include "ButtonWidget.hpp"

namespace tp {

	class CollapsableMenu : public Widget {
	public:
		CollapsableMenu();
		void eventProcess(const Events&) override;
		void eventDraw(Canvas& canvas) override;

	public:
		void addWidgetToMenu(Widget* widget);
		void setLabel(const std::string& string);

		void toggleCollapsed();
		void setCollapsed(bool collapsed);
		[[nodiscard]] bool getCollapsed() const;
		void updateGeometry();

	private:
		RectF getHeaderRect();
		RectF getBodyRect();

	public:
		void eventUpdateConfiguration(WidgetManager& wm) override;

	protected:
		ScrollableWindow mBody;
		LabelWidget mHeader;

		RGBA mMenuColor;
		RGBA mBorderColor;

		halnf headerHeight = 30;
		halnf rounding = 0;
		halnf mBorderSize = 0;
		halnf mPadding = 0;

		halnf mPrevHeight = 200;

		bool mCollapsed = true;
		bool mLocked = false;
		bool mAdjustHeight = true;
	};
}