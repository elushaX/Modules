#pragma once

#include "SimpleWidgets.hpp"
#include "LayoutWidget.hpp"

namespace tp {
	class FloatingWidget : public Widget {
	public:
		FloatingWidget() {
			setDebug("float", { 0.0, 0.9, 0.1, 0.7 });
		}

		void process(const EventHandler& events) override;

		void pickRect() override;

		void draw(Canvas& canvas) override;

		RectF resizeHandleRect();

		[[nodiscard]] bool propagateEventsToChildren() const override;

		[[nodiscard]] bool isFloating() const;

	private:
		bool mIsFloating = false;
		bool mIsResizing = false;

		halnf mHandleSize = 10;
		halnf mHandlePadding = 2;

		Vec2F mPointerStart;
		Vec2F mPointerCurrent;
	};

	class FloatingMenu : public FloatingWidget {
	public:
		FloatingMenu() {
			setDebug("float menu", { 0.0, 0.9, 0.1, 0.7 });

			addChild(&mMenuLayout);

			mMenuLayout.addChild(&mHeader);
			mMenuLayout.addChild(&mBodyLayout);

			mHeader.setText("Menu");
		}

	public:
		void addToMenu(Widget* widget) {
			mBodyLayout.addChild(widget);
		}

	private:
		VerticalLayout mMenuLayout;
		VerticalLayout mBodyLayout;
		LabelWidget mHeader;

		ButtonWidget mTestButton;
	};
}