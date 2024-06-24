#pragma once

#include "LabelWidget.hpp"

namespace tp {

	//TODO : clean up this mess
	// takes the whole parent area
	class SplitView : public Widget {
	public:
		SplitView();

		void eventProcess(const Events& events) override;
		void eventDraw(Canvas& canvas) override;

		[[nodiscard]] bool getFirstEnabled() const;
		[[nodiscard]] bool getSecondEnabled() const;

		RectF getFirst() const;
		RectF getFirstHeader() const;
		RectF getSecondHeader() const;
		RectF getSecond() const ;
		RectF getHandle() const;

	public:
		void eventUpdateConfiguration(WidgetManager& wm) override;

	public:
		halnf mFactor = 0.7f;
		bool mResizeInProcess = false;
		bool mIsHover = false;

		RGBA mHandleColor;
		RGBA mHoveredColor;
		RGBA mResizingColor;
		halnf mMinSize = 0;
		halnf mHandleSize = 0;

		// Headers
		halnf mHeaderSize = 0;
		bool mHeaders = false;
		enum CollapsedSize { NONE, LEFT, RIGHT } mCollapsedSide = NONE;

		LabelWidget mLeftLabel;
		LabelWidget mRightLabel;
	};
}