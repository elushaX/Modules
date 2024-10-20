#pragma once

#include "FloatingWidget.hpp"

namespace tp {
	class CollapsingMenuWidget : public Widget {
	public:
		CollapsingMenuWidget();

		void process(const EventHandler& events) override;
		void draw(Canvas& canvas) override;
		[[nodiscard]] bool processesEvents() const override { return true; }

	public:
		void setCollapsed(bool val);
		LabelWidget* getHeader() { return &mHeader; }
		Widget* getContainer() { return &mContent; }

	private:
		bool mHover = false;
		LabelWidget mHeader;
		Widget mContent;

	private:
		halnf rounding = 5;
		RGBA colorHover = RGBA(0.7);
		RGBA colorBG = RGBA(0.3);
	};

	class FloatingScrollableMenu : public FloatingWidget {
	public:
		FloatingScrollableMenu();

	public:
		void addToMenu(Widget* widget) {
			widget->setSizePolicy(SizePolicy::Expanding, SizePolicy::Minimal);
			mContentWidget.addChild(widget);
		}

		const List<Widget*>& getContent() {
			return mContentWidget.getChildren();
		}

		void clearChildren() {
			mContentWidget.clear();
		}

		void setText(const std::string& text);

	private:
		// VerticalLayout mMenuLayout;
		Widget mBodyLayout;
		Widget mContentWidget;
		ScrollableBarWidget mScrollBar;

		LabelWidget mHeader;

		// ButtonWidget mTestButton;
	};
}