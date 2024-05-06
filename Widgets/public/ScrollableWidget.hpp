#pragma once

#include "WidgetBase.hpp"
#include "Buffer.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class ScrollBarWidget : public Widget<Events, Canvas> {
	public:
		ScrollBarWidget() = default;

		// takes whole area
		void procCallback(const Events& events) override {
			auto area = getHandle();
			mHovered = getHandleHandle().isInside(events.getPointer());

			if (mSizeFraction > 1.f) {
				mPositionFraction = 0;
				return;
			}

			if (events.getScrollY() != 0) {
				auto offset = events.getScrollY() < 0 ? 1.0f : -1.0f;
				if (scrollInertia * offset > 0) {
					scrollInertia += offset;
				} else {
					scrollInertia = -scrollInertia + offset;
				}
			}

			if (tp::abs(scrollInertia) > 0.1f) {
				auto offset = scrollInertia * mScrollFactor;
				mPositionFraction += offset;
				mPositionFraction = tp::clamp(mPositionFraction, 0.f, 1.f - mSizeFraction);
				scrollInertia *= 0.f;
				return;
			}

			if (events.isPressed(InputID::MOUSE1) && area.isInside(events.getPointer())) {
				mIsScrolling = true;
			} else if (events.isReleased(InputID::MOUSE1)) {
				mIsScrolling = false;
			}

			if (mIsScrolling) {
				tp::halnf pos = events.getPointer().y;
				pos = (pos - area.y - mSizeFraction * area.w / 2.f) / area.w;
				mPositionFraction = tp::clamp(pos, 0.f, 1.f - mSizeFraction);
			}

			mPositionFraction = tp::clamp(mPositionFraction, 0.f, 1.f - mSizeFraction);
		}

		void drawCallback(Canvas& canvas) override {
			auto area = getHandle();

			if (mSizeFraction > 1.f) return;
			// if (!areaParent.isOverlap(getHandle())) return;

			tp::RGBA col = mHandleColor;

			if (mIsScrolling) {
				col = mScrollingColor;
			} else if (mHovered) {
				col = mHoveredColor;
			}

			canvas.rect(area, mDefaultColor, mRounding);
			canvas.rect(getHandleHandle(), col, mRounding);
		}

		RectF getHandleHandle() const {
			auto area = getHandle();
			auto sliderSize = tp::clamp(area.w * mSizeFraction, mMinSize * 2, area.w);
			auto diffSize = sliderSize - area.w * mSizeFraction;
			return { area.x, area.y + (area.w - diffSize) * mPositionFraction, area.z, sliderSize };
		}

		RectF getViewport() const {
			if (mSizeFraction > 1.f) {
				return this->mArea;
			}
			return { this->mArea.x, this->mArea.y, this->mArea.z - mHandleSize, this->mArea.w };
		}

		RectF getHandle() const {
			return { this->mArea.x + this->mArea.z - mHandleSize + mPadding,
							 this->mArea.y + mPadding,
							 mHandleSize - mPadding * 2,
							 this->mArea.w - mPadding * 2 };
		}

	public:
		void updateConfigCallback(WidgetManager& wm) override {
			wm.setActiveId("Scrollbar");

			mDefaultColor = wm.getColor("Default", "Base");
			mHandleColor = wm.getColor("Handle", "Accent");
			mHoveredColor = wm.getColor("Hovered", "Interaction");
			mScrollingColor = wm.getColor("Scrolling", "Action");
			mPadding = wm.getNumber("Padding", "Padding");
			mHandleSize = wm.getNumber("HandleSize", 20.f);
			mMinSize = wm.getNumber("MinSize", 20.f);
			mRounding = wm.getNumber("Rounding", "Rounding");
		}

	public:
		halnf mScrollFactor = 0.f;
		halnf scrollInertia = 0.f;
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

	template <typename Events, typename Canvas>
	class ScrollableWindow : public Widget<Events, Canvas> {
	public:
		ScrollableWindow() {
			this->mChildWidgets.pushBack(&mScroller);
			this->mChildWidgets.pushBack(&mContentWidget);
		}

		~ScrollableWindow() = default;

		// takes whole area
		void procCallback(const Events& events) override {
			List<Widget<Events, Canvas>*>& content = mContentWidget.mChildWidgets;

			updateContents(content);
			updateContentSize(content);

			const auto padding = mPadding;

			mScroller.mSizeFraction = mContentWidget.mArea.w / mContentSize;
			mScroller.setArea(this->mArea);
			mContentWidget.setArea(mScroller.getViewport());

			if (mScroller.mSizeFraction > 1.f) {
				setOffset(content, 0);
			} else {
				setOffset(content, (-mScroller.mPositionFraction) * mContentSize);
			}

			for (auto widget : content) {
				widget->setArea({ mContentWidget.mArea.x + padding,
													widget->mArea.y,
													mScroller.getViewport().z - padding * 2,
													widget->mArea.w });
			}
		}

		void addWidget(Widget<Events, Canvas>* widget) { mContentWidget.mChildWidgets.pushBack(widget); }
		void clearContent() { mContentWidget.mChildWidgets.removeAll(); }
		List<Widget<Events, Canvas>*>& getContent() { return mContentWidget.mChildWidgets; }

		void updateConfigCallback(WidgetManager& wm) override {
			wm.setActiveId("ScrollableWidget");
			mPadding = wm.getNumber("Padding", "Padding");
		}

	private:
		void updateContents(List<Widget<Events, Canvas>*>& contentWidgets) {
			if (contentWidgets.size()) {
				const halnf offset = contentWidgets.first()->mArea.y + mPadding;

				halnf start = 0;
				for (auto widget : contentWidgets) {
					widget->mArea.y = start;
					start += widget->mArea.w + mPadding;
				}

				for (auto widget : contentWidgets) {
					widget->mArea.y += offset;
				}
			}
		}

		void updateContentSize(List<Widget<Events, Canvas>*>& contentWidgets) {
			mContentSize = 0;
			if (contentWidgets.size()) {
				mContentSize = contentWidgets.last()->mArea.y - contentWidgets.first()->mArea.y;
				mContentSize += contentWidgets.last()->mArea.w;
				mContentSize += 2 * mPadding;
			}
		}

		void setOffset(List<Widget<Events, Canvas>*>& contentWidgets, const halnf offset) {
			if (!contentWidgets.size()) return;
			auto newOffset = offset - contentWidgets.first()->mArea.y + mPadding;
			for (auto widget : contentWidgets) {
				widget->mArea.y += newOffset;
			}
		}

	private:
		Widget<Events, Canvas> mContentWidget;
		ScrollBarWidget<Events, Canvas> mScroller;

		halnf mContentSize = 0;

		halnf mPadding = 0;
	};
}