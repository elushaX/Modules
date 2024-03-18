#pragma once

#include "WidgetBase.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class ScrollBarWidget : public Widget<Events, Canvas> {
	public:
		ScrollBarWidget() {
			this->createConfig("ScrollBar");
			this->addColor("Default", "Base");
			this->addColor("Handle", "Accent");
			this->addColor("Hovered", "Interaction");
			this->addColor("Scrolling", "Action");
			this->addValue("Padding", "Padding");
			this->addValue("HandleSize", 20.f);
			this->addValue("MinSize", 20.f);
			this->addValue("Rounding", "Rounding");
		}

		// takes whole area
		void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) override {
			this->mArea = aArea;

			auto area = getHandle();
			mHovered = getHandleHandle().isInside(events.getPos());

			if (mSizeFraction > 1.f) {
				mPositionFraction = 0;
				return;
			}

			if (!areaParent.isOverlap(area)) {
				mIsScrolling = false;
				return;
			}

			if (events.getScrollY() != 0 && areaParent.isInside(events.getPos())) {
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

			if (events.isPressed() && area.isInside(events.getPos())) {
				mIsScrolling = true;
			} else if (!events.isDown()) {
				mIsScrolling = false;
			}

			if (mIsScrolling) {
				tp::halnf pos = events.getPos().y;
				pos = (pos - area.y - mSizeFraction * area.w / 2.f) / area.w;
				mPositionFraction = tp::clamp(pos, 0.f, 1.f - mSizeFraction);
			}

			mPositionFraction = tp::clamp(mPositionFraction, 0.f, 1.f - mSizeFraction);
		}

		void draw(Canvas& canvas) override {
			auto area = getHandle();

			if (mSizeFraction > 1.f) return;
			// if (!areaParent.isOverlap(getHandle())) return;

			tp::RGBA col = this->getColor("Handle");

			if (mIsScrolling) {
				col = this->getColor("Scrolling");
			} else if (mHovered) {
				col = this->getColor("Hovered");
			}

			canvas.rect(area, this->getColor("Default"), this->getValue("Rounding"));

			canvas.rect(getHandleHandle(), col, this->getValue("Rounding"));
		}

		RectF getHandleHandle() const {
			auto minSize = this->getValue("MinSize");
			if (mIsScrolling) {
				minSize = this->getValue("MinSize") * 2;
			} else if (mHovered) {
				minSize = this->getValue("MinSize") * 2;
			}
			auto area = getHandle();
			auto sliderSize = tp::clamp(area.w * mSizeFraction, minSize, area.w);
			auto diffSize = sliderSize - area.w * mSizeFraction;
			return { area.x, area.y + (area.w - diffSize) * mPositionFraction, area.z, sliderSize };
		}

		RectF getViewport() const {
			if (mSizeFraction > 1.f) {
				return this->mArea;
			}
			return { this->mArea.x, this->mArea.y, this->mArea.z - this->getValue("HandleSize"), this->mArea.w };
		}

		RectF getHandle() const {
			return { this->mArea.x + this->mArea.z - this->getValue("HandleSize") + this->getValue("Padding"),
							 this->mArea.y + this->getValue("Padding"),
							 this->getValue("HandleSize") - this->getValue("Padding") * 2,
							 this->mArea.w - this->getValue("Padding") * 2 };
		}

	public:
		halnf mScrollFactor = 0.f;
		halnf scrollInertia = 0.f;
		bool mIsScrolling = false;
		halnf mSizeFraction = 1.f;
		halnf mPositionFraction = 0.f;
		bool mHovered = false;
	};

	template <typename Events, typename Canvas>
	class ScrollableWindow : public Widget<Events, Canvas> {
	public:
		ScrollableWindow() {
			this->createConfig("ScrollableWindow");
			this->addColor("Default", "Base");
			this->addValue("Padding", "Padding");
		}

		~ScrollableWindow() {
			for (auto content : mContents) {
				delete content.data();
			}
		}

		// takes whole area
		void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) override {
			this->mArea = aArea;
			this->mVisible = areaParent.isOverlap(aArea);

			if (!this->mVisible) return;

			updateContents();
			updateContentSize();

			const auto padding = this->getValue("Padding");

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

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;
			mScroller.draw(canvas);

			canvas.pushClamp(this->mArea);
			for (auto widget : mContents) {
				widget->draw(canvas);
			}
			canvas.popClamp();
		}

	private:
		void updateContents() {
			if (mContents.size()) {
				const auto padding = this->getValue("Padding");
				const halnf offset = mContents.first()->mArea.y + padding;

				halnf start = 0;
				for (auto widget : mContents) {
					widget->mArea.y = start;
					start += widget->mArea.w + padding;
				}

				for (auto widget : mContents) {
					widget->mArea.y += offset;
				}
			}
		}

		void updateContentSize() {
			mContentSize = 0;
			if (mContents.size()) {
				const auto padding = this->getValue("Padding");
				mContentSize = mContents.last()->mArea.y - mContents.first()->mArea.y;
				mContentSize += mContents.last()->mArea.w;
				mContentSize += 2 * padding;
			}
		}

		void setOffset(const halnf offset) {
			if (!mContents.size()) return;
			const auto padding = this->getValue("Padding");
			auto newOffset = offset - mContents.first()->mArea.y + padding;
			for (auto widget : mContents) {
				widget->mArea.y += newOffset;
			}
		}

	public:
		halnf mContentSize = 0;
		Buffer<Widget<Events, Canvas>*> mContents;
		ScrollBarWidget<Events, Canvas> mScroller;
	};
}