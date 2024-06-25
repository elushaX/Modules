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

	class SimpleWidget2 : public DockSpaceWidget {
	public:
		SimpleWidget2() {
			this->mChildWidgets.pushBack(&mFloating);
			mFloating.addWidgetToMenu(&mWidget);

			this->mChildWidgets.pushBack(&mFloating2);
			mFloating2.addWidgetToMenu(&mWidget2);

			mFloating2.mArea = { 200, 100, 100, 100 };

			this->mChildWidgets.pushBack(&mFloating3);
			mFloating3.addWidgetToMenu(&mWidget3);

			mFloating3.mArea = { 400, 100, 100, 100 };
		}

	private:
		FloatingWidget mFloating;
		SimpleWidget mWidget;

		FloatingWidget mFloating2;
		SimpleWidget mWidget2;

		FloatingWidget mFloating3;
		SimpleWidget mWidget3;
	};
}