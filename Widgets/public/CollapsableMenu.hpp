#pragma once

#include "ScrollableWidget.hpp"
#include "ButtonWidget.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class CollapsableMenu : public Widget<Events, Canvas> {
	public:
		CollapsableMenu() {
			this->mChildWidgets.pushBack(&mHeader);
			this->mChildWidgets.pushBack(&mBody);
		}

		void eventProcess(const Events&) override {
			if (mHeader.isReleased()) {
				toggleCollapsed();
			}

			updateGeometry();
		}

		void eventDraw(Canvas& canvas) override {
			canvas.rect(this->mArea, mBorderColor, rounding);
			canvas.rect(this->mArea.shrink(mBorderSize), mMenuColor, rounding);
		}

	public:
		void addWidgetToMenu(Widget<Events, Canvas>* widget) {
			mBody.addWidget(widget);
		}

		void setLabel(const std::string& string) {
			mHeader.mLabel = string;
		}

		void toggleCollapsed() {
			setCollapsed(!getCollapsed());
		}

		void setCollapsed(bool collapsed) {
			if (mLocked) return;
			if (collapsed && !mCollapsed) mPrevHeight = this->mArea.size.y;
			if (!collapsed && mCollapsed) this->mArea.size.y = mPrevHeight;
			mCollapsed = collapsed;
		}

		[[nodiscard]] bool getCollapsed() const{
			return mCollapsed;
		}

		void updateGeometry() {
			mHeader.setArea(getHeaderRect());

			mBody.mEnable = !mCollapsed;
			if (mCollapsed) {
				this->mArea.size.y = headerHeight;
			} else {
				if (mAdjustHeight) {
					this->mArea.size.y = headerHeight + getBodyRect().size.y + mPadding * 2;
				}

				mBody.setArea(getBodyRect());
			}
		}

	private:
		RectF getHeaderRect() {
			RectF out = { this->mArea.pos, { this->mArea.size.x, headerHeight } };
			return out.shrink(mPadding);
		}

		RectF getBodyRect() {
			RectF out = { { this->mArea.pos.x, this->mArea.pos.y + headerHeight },
							 { this->mArea.size.x, this->mArea.size.y - headerHeight - mPadding }
			};

			out.size.y -= mBorderSize * 2;

			if (mAdjustHeight) out.size.y = mBody.getContentSize();

			if (mBody.getContentSize()) {
				out = out.shrink(mPadding);
				out.size.y += mPadding * 2 + 1;
			}

			return out;
		}

	public:
		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("CollapsableMenu");

			headerHeight = wm.getNumber("HeaderHeight", 35);
			mMenuColor = wm.getColor("MenuColor", RGBA(0, 0, 0, 1.f));
			rounding = wm.getNumber("Rounding", "Rounding");
			mBorderColor = wm.getColor("BorderColor", RGBA(0.16, 0.16, 0.16, 1.f));
			mBorderSize = wm.getNumber("BorderSize", 2.f);
			mPadding = wm.getNumber("Padding", "Padding");
		}

	protected:
		ScrollableWindow<Events, Canvas> mBody;
		LabelWidget<Events, Canvas> mHeader;

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