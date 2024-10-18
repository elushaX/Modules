#pragma once

#include "Widget.hpp"

namespace tp {
	// just the scrolling bar
	class ScrollableBarWidget : public Widget {
	public:
		ScrollableBarWidget() = default;

		[[nodiscard]] bool processesEvents() const override { return true; }
		[[nodiscard]] bool propagateEventsToChildren() const override { return false; }
		[[nodiscard]] bool needsNextFrame() const override { return mScrolling; }

		void process(const EventHandler& events) override;
		void draw(Canvas& canvas) override;

		void updateSizeFactor(halnf factor); // holder-size divided by content-size

		[[nodiscard]] bool getDirection() const { return mVertical; }
		[[nodiscard]] halnf getPosFactor() const { return mPosFactor - mSizeFactor / 2; }

	private:
		[[nodiscard]] const RectF& getHandleRect() const;
		void updateHandleRect();
		void jumpTo(halnf);
		void moveBy(halnf);
		void clamp();

	private: // state
		bool mVertical = true;
		bool mScrolling = false;
		bool mHandleHovered = false;

		Vec2F mStartPos = {};

		halnf mPosFactor = 0.2f; // (center-of-holder - content-start) / content-size
		halnf mSizeFactor = 0.3f;

	private: // TODO : make params static
		halnf mRounding = 5;
		halnf mHandlePadding = 0;

		RGBA mBGColor = { 0.1, 0.1, 0.1, 0.0 };
		RGBA mHandleColor = { 0.2, 0.2, 0.2, 1 };
		RGBA mHandleHoverColor = { 0.3, 0.3, 0.3, 1 };
		RGBA mHandleSlideColor = { 0.5, 0.5, 0.5, 1 };

	private: // cache
		RectF mHandleRect;
	};

	class ScrollableWidget : public Widget {
	public:
		ScrollableWidget();
	};
}