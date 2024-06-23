#pragma once

#include "CollapsableMenu.hpp"

namespace tp {
	template <typename Events, typename Canvas>
	class FloatingWidget : public CollapsableMenu<Events, Canvas> {
	public:
		FloatingWidget() {
			this->mArea = { 0, 0, 300, 300 };
			this->mAdjustHeight = false;
		}

		void eventProcess(const Events& events) override {
			checkFloating(events);
			checkResizing(events);

			mActionStartRelativePos = events.getPointer() - this->mArea.pos;

			CollapsableMenu<Events, Canvas>::eventProcess(events);
		}

		void eventDraw(Canvas& canvas) override {
			CollapsableMenu<Events, Canvas>::eventDraw(canvas);

			if (!this->getCollapsed()) {
				auto rect = getResizeHandle();
				canvas.rect(rect, mResizeHandleColor, 0);
				canvas.circle(rect.pos - this->mBorderSize, rect.w, this->mMenuColor);
			}
		}

		void eventUpdateConfiguration(WidgetManager& wm) override {
			CollapsableMenu<Events, Canvas>::eventUpdateConfiguration(wm);

			wm.setActiveId("FloatingWidget");

			mResizeHandleSize = wm.getNumber("ResizeHandleSize", 15);
			mResizeHandleColor = wm.getColor("ResizeHandleColor", RGBA(0.16, 0.16, 0.16, 1.f));
		}

	private:
		void checkFloating(const Events& events) {
			if (this->mHeader.isHolding() && events.getPointerDelta().length2() > 4 ) {
				mFloating = true;
			}

			if (mFloating && this->mHeader.isReleased()) {
				mFloating = false;
				this->mHeader.clearEvents();
			}

			if (mFloating) {
				auto relativePos = events.getPointer() - this->mArea.pos;
				this->mArea.pos += relativePos - mActionStartRelativePos;
			}
		}

		void checkResizing(const Events& events) {
			if (this->getCollapsed()) return;

			if (events.isPressed(InputID::MOUSE1) && getResizeHandle().isInside(events.getPointer())) {
				mResizing = true;
			}

			if (events.isReleased(InputID::MOUSE1)) {
				mResizing = false;
			}

			if (mResizing) {
				auto relativePos = events.getPointer() - this->mArea.pos;
				this->mArea.size += relativePos - mActionStartRelativePos;
			}

			if (!this->mCollapsed) {
				this->mArea.size.clamp(mMinSize, { FLT_MAX, FLT_MAX });
			}
		}

		RectF getResizeHandle() {
			auto size = Vec2F(mResizeHandleSize);
			auto pos = this->mArea.pos + this->mArea.size - size;
			return { pos, size };
		}

	private:
		Vec2F mMinSize = { 70, 70 };

		halnf mResizeHandleSize = 10;
		RGBA mResizeHandleColor = {};

		bool mFloating = false;
		bool mResizing = false;

		Vec2F mActionStartRelativePos = {};
	};
}