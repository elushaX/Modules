
#include "GraphicApplication.hpp"

#include "RootWidget.hpp"
#include "AnimationTestWidget.hpp"
#include "FloatingWidget.hpp"
#include "SimpleWidgets.hpp"
#include "LayoutWidget.hpp"

using namespace tp;

class WidgetApplication : public Application {
public:
	WidgetApplication() {
		mRootWidget.setRootWidget(&mLayoutWidget);

		mLayoutWidget.addChild(&mFloatingMenu);
		// mLayoutWidget.addChild(&mButton2);

		// mWidgetFloating.addChild(&mButton);

		// mRootWidget.setRootWidget(&mAnimationTestWidget);

		// mAnimationTestWidget.addChild(&mWidgetFloating);
		// mAnimationTestWidget.addChild(&mLayoutWidget);

		// RootWidget::setWidgetArea(mButton, { 100, 100, 150, 200 });

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
	LabelWidget mLabel;
	ButtonWidget mButton;
	ButtonWidget mButton2;
	FloatingWidget mWidgetFloating;
	AnimationTestWidget mAnimationTestWidget;
	DesktopLayout mLayoutWidget;
	FloatingMenu mFloatingMenu;

	RootWidget mRootWidget;
};

int main() {
	{
		WidgetApplication gui;
		gui.run();
	}
}
