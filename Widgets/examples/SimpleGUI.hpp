#include "Widgets.hpp"

namespace tp {

	class SimpleWidget : public CollapsableMenu {
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
		ButtonWidget mInMenuButton1;
		ButtonWidget mInMenuButton2;

		LabelWidget mLabel;
		NamedSliderWidget mSlider;
	};

	class SimpleWidget2 : public Widget {
	public:
		SimpleWidget2() {
			this->mChildWidgets.pushBack(&mFloating);
			mFloating.addWidgetToMenu(&mWidget);
			mFloating.addWidgetToMenu(&mWidget2);

			// this->mChildWidgets.pushBack(&mFloating2);
			// mFloating2.addWidgetToMenu(&mWidget2);
		}

	private:
		FloatingWidget mFloating;
		SimpleWidget mWidget;

		FloatingWidget mFloating2;
		SimpleWidget mWidget2;
	};
}