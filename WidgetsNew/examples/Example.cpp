
#include "GraphicApplication.hpp"

#include "RootWidget.hpp"
#include "AnimationTestWidget.hpp"
#include "FloatingWidget.hpp"
#include "DockLayoutWidget.hpp"

using namespace tp;

class WidgetApplication : public Application {
public:
	WidgetApplication() {
		setup2();
	}

	void setup1() {
		mRootWidget.setRootWidget(&mDockLayout);

		mDockLayout.addChild(&mFloatingMenu);
		mDockLayout.addChild(&mFloatingMenu2);

		mDockLayout.setCenterWidget(&mButton4);
		mDockLayout.dockWidget(&mButton3, DockLayoutWidget::RIGHT);

		mFloatingMenu.addToMenu(&mButton);
		mFloatingMenu2.addToMenu(&mButton2);

		mButton.setAction([this]() { mButton2.setColor(RGBA::random()); });
		mButton2.setAction([this]() { mButton.setColor(RGBA::random()); });

		mButton3.setAction([this]() { mDockLayout.dockWidget(&mFloatingMenu, DockLayoutWidget::Side::LEFT); });
		mButton3.setText("dock");

		mButton4.setAction([this]() { mDockLayout.undockWidget(DockLayoutWidget::Side::LEFT); });

		// mLayoutWidget.addChild(&mButton2);

		// mWidgetFloating.addChild(&mButton);

		// mRootWidget.setRootWidget(&mAnimationTestWidget);

		// mAnimationTestWidget.addChild(&mWidgetFloating);
		// mAnimationTestWidget.addChild(&mLayoutWidget);

		RootWidget::setWidgetArea(mFloatingMenu2, { 300, 100, 150, 200 });
		RootWidget::setWidgetArea(mFloatingMenu, { 100, 100, 150, 200 });

		// mWidgetFloating.addChild(&mLayoutWidget);

		// mLayoutWidget.addChild(&mButton);
		// mLayoutWidget.addChild(&mLabel);
	}

	void setup2() {
		mRootWidget.setRootWidget(&mDesktopLayout);
		mDesktopLayout.addChild(&mWidget);

		mWidget.addChild(&mButton);
		mWidget.addChild(&mButton2);
		mWidget.addChild(&mButton3);

		mButton.setSizePolicy(Widget::SizePolicy::Expand, Widget::SizePolicy::Contract);
		mButton2.setSizePolicy(Widget::SizePolicy::Expand, Widget::SizePolicy::Contract);
		mButton3.setSizePolicy(Widget::SizePolicy::Expand, Widget::SizePolicy::Contract);

		mWidget.setSizePolicy(Widget::SizePolicy::Contract, Widget::SizePolicy::Contract);
		mWidget.setLayoutPolicy(Widget::LayoutPolicy::Horizontally);

		mButton.setAction([this]() { mButton2.setMinSize(mButton2.getMinSize() + 10); });
		mButton2.setAction([this]() { mButton.setMinSize(mButton.getMinSize() + 10); } );

		RootWidget::setWidgetArea(mWidget, { 300, 100, 150, 200 });
	}

	void processFrame(EventHandler* eventHandler, halnf deltaTime) override {
		const auto rec = RectF({ 0, 0 }, mWindow->getSize());
		mRootWidget.processFrame(eventHandler, rec);
	}

	void drawFrame(Canvas* canvas) override {
		mRootWidget.drawFrame(*canvas);

		drawDebug();
	}

	bool forceNewFrame() override {
		return mRootWidget.needsUpdate();
	}

private:
	Widget mWidget;

	ButtonWidget mButton;
	ButtonWidget mButton2;
	ButtonWidget mButton3;
	ButtonWidget mButton4;

	FloatingMenu mFloatingMenu;
	FloatingMenu mFloatingMenu2;

	LabelWidget mLabel;
	FloatingWidget mWidgetFloating;
	AnimationTestWidget mAnimationTestWidget;
	DesktopLayout mDesktopLayout;

	DockLayoutWidget mDockLayout;

	RootWidget mRootWidget;
};

int main() {
	{
		WidgetApplication gui;
		gui.run();
	}
}
