#pragma once

#include "WidgetBase.hpp"
#include "Buffer.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class ScrollBarWidget : public Widget<Events, Canvas> {
	public:
		ScrollBarWidget() = default;

		// takes whole area
		void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) override {
			this->mArea = aArea;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;

			auto area = getHandle();
			mHovered = getHandleHandle().isInside(events.getPointer());

			if (mSizeFraction > 1.f) {
				mPositionFraction = 0;
				return;
			}

			if (!areaParent.isOverlap(area)) {
				mIsScrolling = false;
				return;
			}

			if (events.getScrollY() != 0 && areaParent.isInside(events.getPointer())) {
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

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;

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
		void updateConfigCache(WidgetManager& wm) override {
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
		ScrollableWindow() = default;
		~ScrollableWindow() = default;

		// takes whole area
		void procBody(const Events& events) override {
			updateContents();
			updateContentSize();

			const auto padding = mPadding;

			mScroller.mSizeFraction = this->mArea.w / mContentSize;
			mScroller.proc(events, this->mArea, this->mArea);

			if (mScroller.mSizeFraction > 1.f) {
				setOffset(0);
			} else {
				setOffset((-mScroller.mPositionFraction) * mContentSize);
			}

			for (auto widget : mContents) {
				widget->proc(
					events,
					this->mArea,
					{ this->mArea.x + padding, widget->mArea.y, mScroller.getViewport().z - padding * 2, widget->mArea.w }
				);
			}
		}

		void drawBody(Canvas& canvas) override {
			mScroller.draw(canvas);

			canvas.pushClamp(this->mArea);
			for (auto widget : mContents) {
				widget->draw(canvas);
			}
			canvas.popClamp();
		}

	public:
		void updateConfigCache(WidgetManager& wm) override {
			wm.setActiveId("ScrollableWidget");

			mDefaultColor = wm.getColor("Default", "Base");
			mPadding = wm.getNumber("Padding", "Padding");

			mScroller.updateConfigCache(wm);

			for (auto item : mContents) {
				item->updateConfigCache(wm);
			}
		}

	private:
		void updateContents() {
			if (mContents.size()) {
				const halnf offset = mContents.first()->mArea.y + mPadding;

				halnf start = 0;
				for (auto widget : mContents) {
					widget->mArea.y = start;
					start += widget->mArea.w + mPadding;
				}

				for (auto widget : mContents) {
					widget->mArea.y += offset;
				}
			}
		}

		void updateContentSize() {
			mContentSize = 0;
			if (mContents.size()) {
				mContentSize = mContents.last()->mArea.y - mContents.first()->mArea.y;
				mContentSize += mContents.last()->mArea.w;
				mContentSize += 2 * mPadding;
			}
		}

		void setOffset(const halnf offset) {
			if (!mContents.size()) return;
			auto newOffset = offset - mContents.first()->mArea.y + mPadding;
			for (auto widget : mContents) {
				widget->mArea.y += newOffset;
			}
		}

	public:
		halnf mContentSize = 0;

		Buffer<Widget<Events, Canvas>*> mContents;
		ScrollBarWidget<Events, Canvas> mScroller;

		RGBA mDefaultColor;
		halnf mPadding = 0;
	};
}