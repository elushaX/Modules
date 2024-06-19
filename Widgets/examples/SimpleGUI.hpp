#include "Widgets.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class SimpleWidget : public ScrollableWindow<Events, Canvas> {
	public:
		SimpleWidget() {
			this->addWidget(&mLabel);
			this->addWidget(&mButton);
			this->addWidget(&mSlider);
			this->addWidget(&mCollapsableMenu);

			mCollapsableMenu.addWidgetToMenu(&mInMenuButton1);
			mCollapsableMenu.addWidgetToMenu(&mInMenuButton2);

			mInMenuButton1.mLabel.mLabel = "Button1";
			mInMenuButton2.mLabel.mLabel = "Button2";

			this->mArea.size = { 500, 500 };
		}

	private:
		CollapsableMenu<Events, Canvas> mCollapsableMenu;
		ButtonWidget<EventHandler, Canvas> mInMenuButton1;
		ButtonWidget<EventHandler, Canvas> mInMenuButton2;

		ButtonWidget<EventHandler, Canvas> mButton;
		LabelWidget<EventHandler, Canvas> mLabel;
		NamedSliderWidget<EventHandler, Canvas> mSlider;
	};

	template <typename Events, typename Canvas>
	class SimpleWidget2 : public Widget<Events, Canvas> {
	public:
		SimpleWidget2() {
			this->mChildWidgets.pushBack(&mFloating);
			mFloating.addWidgetToMenu(&mWidget);
		}

	private:
		FloatingWidget<EventHandler, Canvas> mFloating;
		SimpleWidget<EventHandler, Canvas> mWidget;
	};
}