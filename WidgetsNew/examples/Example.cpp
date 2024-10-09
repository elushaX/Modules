
#include "GraphicApplication.hpp"

#include "RootWidget.hpp"
#include "AnimationTestWidget.hpp"
#include "FloatingWidget.hpp"
#include "DockLayoutWidget.hpp"

using namespace tp;

class WidgetApplication : public Application {
public:
	WidgetApplication() {
		mRootWidget.setRootWidget(&mDockLayout);

		mDockLayout.addChild(&mFloatingMenu);
		mDockLayout.addChild(&mFloatingMenu2);

		mDockLayout.setCenterWidget(&mButton3);
		mDockLayout.dockWidget(&mButton4, DockLayoutWidget::RIGHT);

		mFloatingMenu.addToMenu(&mButton);
		mFloatingMenu2.addToMenu(&mButton2);

		mButton.setAction([this]() { mButton2.setColor(RGBA::random()); });
		mButton2.setAction([this]() { mButton.setColor(RGBA::random()); });

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
	ButtonWidget mButton;
	ButtonWidget mButton2;
	ButtonWidget mButton3;
	ButtonWidget mButton4;

	FloatingMenu mFloatingMenu;
	FloatingMenu mFloatingMenu2;

	LabelWidget mLabel;
	FloatingWidget mWidgetFloating;
	AnimationTestWidget mAnimationTestWidget;
	DesktopLayout mLayoutWidget;

	DockLayoutWidget mDockLayout;

	RootWidget mRootWidget;
};

int main() {
	{
		WidgetApplication gui;
		gui.run();
	}
}
