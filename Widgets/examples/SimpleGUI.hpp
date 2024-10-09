#include "Widgets.hpp"

namespace tp {

	class SimpleWidget : public CollapsableMenu {
	public:
		SimpleWidget() {
			this->addWidgetToMenu(&mSlider);

			this->addWidgetToMenu(&mInMenuButton1);
			this->addWidgetToMenu(&mInMenuButton2);

			this->addWidgetToMenu(&mLabel);

			mInMenuButton1.mLabel.mLabel = "Button1";

			mInMenuButton1.mCallback = []() {
				printf("asd\n");
			};

			mInMenuButton2.mLabel.mLabel = "Button2";
		}

	private:
		ButtonWidget mInMenuButton1;
		ButtonWidget mInMenuButton2;

		LabelWidget mLabel;
		NamedSliderWidget mSlider;
	};

	class SimpleWidget3 : public WorkspaceWidget  {
	public:
		SimpleWidget3() {

			mDockSpace.addSideWidget(&mButtons[0], DockLayoutWidget::BOTTOM);
			mDockSpace.addSideWidget(&mButtons[1], DockLayoutWidget::RIGHT);

			mDockSpace.removeSideWidget(DockLayoutWidget::BOTTOM);

			mDockSpace.addSideWidget(&mButtons[0], DockLayoutWidget::TOP);
			mDockSpace.addSideWidget(&mButtons[2], DockLayoutWidget::LEFT);

			mDockSpace.removeSideWidget(DockLayoutWidget::TOP);

			mDockSpace.addSideWidget(&mButtons[0], DockLayoutWidget::BOTTOM);

			mDockSpace.addSideWidget(&mButtons[3], DockLayoutWidget::TOP);
			mDockSpace.setCenterWidget(&mButtons[4]);

			/*
			mButtons[4].mCallback = [&]() { mDockSpace.toggleHiddenState(DockSpaceWidget::BOTTOM); };
			mButtons[0].mCallback = [&]() { mDockSpace.toggleHiddenState(DockSpaceWidget::TOP); };
			mButtons[2].mCallback = [&]() { mDockSpace.toggleHiddenState(DockSpaceWidget::RIGHT); };
			mButtons[3].mCallback = [&]() { mDockSpace.toggleHiddenState(DockSpaceWidget::LEFT); };
			*/

			mButtons[0].addWidgetToMenu(&mWidget);
			mButtons[1].addWidgetToMenu(&mWidget2);
		}

	private:
		FloatingWidget mButtons[5];

		SimpleWidget mWidget;
		SimpleWidget mWidget2;
		SimpleWidget mWidget3;
	};
}