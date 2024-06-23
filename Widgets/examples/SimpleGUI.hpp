#include "Widgets.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class SimpleWidget : public CollapsableMenu<Events, Canvas> {
	public:
		SimpleWidget() {
			this->addWidgetToMenu(&mInMenuButton1);
			this->addWidgetToMenu(&mInMenuButton2);

			this->addWidgetToMenu(&mLabel);
			this->addWidgetToMenu(&mSlider);

			mInMenuButton1.mLabel.mLabel = "Button1";
			mInMenuButton2.mLabel.mLabel = "Button2";
		}

	private:
		ButtonWidget<EventHandler, Canvas> mInMenuButton1;
		ButtonWidget<EventHandler, Canvas> mInMenuButton2;

		LabelWidget<EventHandler, Canvas> mLabel;
		NamedSliderWidget<EventHandler, Canvas> mSlider;
	};

	template <typename Events, typename Canvas>
	class SimpleWidget2 : public Widget<Events, Canvas> {
	public:
		SimpleWidget2() {
			this->mChildWidgets.pushBack(&mFloating);
			mFloating.addWidgetToMenu(&mWidget);
			mFloating.addWidgetToMenu(&mWidget2);
			
			// this->mChildWidgets.pushBack(&mFloating2);
			// mFloating2.addWidgetToMenu(&mWidget2);
		}

	private:
		FloatingWidget<EventHandler, Canvas> mFloating;
		SimpleWidget<EventHandler, Canvas> mWidget;

		FloatingWidget<EventHandler, Canvas> mFloating2;
		SimpleWidget<EventHandler, Canvas> mWidget2;
	};
}