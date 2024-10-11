#pragma once

#include "SimpleWidgets.hpp"
#include "LayoutWidget.hpp"

namespace tp {
	class FloatingWidget : public Widget {
	public:
		FloatingWidget() {
			setDebug("float", { 0.0, 0.9, 0.1, 1 });

			mSizePolicy = { SizePolicy::Fixed, SizePolicy::Fixed };
			mLayoutPolicy = LayoutPolicy::Horizontally;
		}

		void process(const EventHandler& events) override;

		void pickRect() override;

		void draw(Canvas& canvas) override;

		RectF resizeHandleRect();

		[[nodiscard]] bool needsNextFrame() const override;

		[[nodiscard]] bool propagateEventsToChildren() const override;
		[[nodiscard]] bool processesEvents() const override { return true; }

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

			// addChild(&mMenuLayout);

			addChild(&mHeader);
			addChild(&mBodyLayout);

			mHeader.setText("Menu");

			mHeader.setSizePolicy(SizePolicy::Expanding, SizePolicy::Minimal);
			mBodyLayout.setSizePolicy(SizePolicy::Expanding, SizePolicy::Expanding);

			setLayoutPolicy(LayoutPolicy::Vertically);
			mBodyLayout.setLayoutPolicy(LayoutPolicy::Vertically);
		}

	public:
		void addToMenu(Widget* widget) {
			widget->setSizePolicy(SizePolicy::Expanding, SizePolicy::Minimal);
			mBodyLayout.addChild(widget);
		}

	private:
		// VerticalLayout mMenuLayout;
		Widget mBodyLayout;
		LabelWidget mHeader;

		// ButtonWidget mTestButton;
	};
}