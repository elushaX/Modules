#pragma once

#include "WidgetBase.hpp"
#include "Buffer.hpp"

namespace tp {

	class ScrollBarWidget : public Widget {
	public:
		ScrollBarWidget();

		// takes whole area
		void eventProcess(const Events& events) override;
		void eventDraw(Canvas& canvas) override;

		RectF getHandleHandle() const;
		RectF getViewport() const;
		RectF getHandle() const;

	public:
		void eventUpdateConfiguration(WidgetManager& wm) override;

	public:
		bool mIsScrolling = false;
		halnf mSizeFraction = 1.f;
		halnf mPositionFraction = 0.f;
		bool mHovered = false;

		RGBA mDefaultColor;
		RGBA mHandleColor;
		RGBA mHoveredColor;
		RGBA mScrollingColor;
		halnf mPadding = 0;
		halnf mHandleSize = 10;
		halnf mMinSize = 10;
		halnf mRounding = 10;
	};

	class ScrollableWindow : public Widget {
	public:
		ScrollableWindow();

		virtual ~ScrollableWindow();

		// takes whole area
		void eventProcess(const Events& events) override;

		void addWidget(Widget* widget);
		void clearContent();
		List<Widget*>& getContent();

		void eventUpdateConfiguration(WidgetManager& wm) override;

		[[nodiscard]] halnf getContentSize() const;

	private:
		void updateContents(List<Widget*>& contentWidgets);

		// ready content size
		void updateContentSize(List<Widget*>& contentWidgets);
		void setOffset(List<Widget*>& contentWidgets, const halnf offset);

	private:
		Widget mContentWidget;
		ScrollBarWidget mScroller;

		halnf mContentSize = 0;

		halnf mPadding = 0;
	};
}