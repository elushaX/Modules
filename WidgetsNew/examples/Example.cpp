
#include "GraphicApplication.hpp"

#include "RootWidget.hpp"
#include "AnimationTestWidget.hpp"
#include "FloatingWidget.hpp"
#include "DockLayoutWidget.hpp"

using namespace tp;


/*
 * adjust child widgets in the layout view
 * make scrollable area
 * change Widget::clampMinMax function to include child enclosure
 * refactor all sizing in Widget -> move to separate files
 * refactor root widget -> reorganize code
*/


class WidgetApplication : public Application {
public:
	WidgetApplication() {
		setup1();
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

	void setup3() {
		mRootWidget.setRootWidget(&mDesktopLayout);
		mDesktopLayout.addChild(&mFloatingWidget);

		mFloatingWidget.addChild(&mButton);
		mFloatingWidget.addChild(&mButton2);
		mFloatingWidget.addChild(&mButton3);

		mButton.setSizePolicy(Widget::SizePolicy::Minimal, Widget::SizePolicy::Expanding);
		mButton2.setSizePolicy(Widget::SizePolicy::Minimal, Widget::SizePolicy::Expanding);
		mButton3.setSizePolicy(Widget::SizePolicy::Expanding, Widget::SizePolicy::Expanding);

		RootWidget::setWidgetArea(mFloatingWidget, { 100, 100, 400, 400 });
	}

	void setup2() {
		mRootWidget.setRootWidget(&mDesktopLayout);
		mDesktopLayout.addChild(&mWidget);

		mWidget.addChild(&mButton);
		mWidget.addChild(&mButton2);
		mWidget.addChild(&mButton3);

		mButton.setSizePolicy(Widget::SizePolicy::Expanding, Widget::SizePolicy::Minimal);
		mButton2.setSizePolicy(Widget::SizePolicy::Expanding, Widget::SizePolicy::Minimal);
		mButton3.setSizePolicy(Widget::SizePolicy::Expanding, Widget::SizePolicy::Minimal);

		mWidget.setSizePolicy(Widget::SizePolicy::Minimal, Widget::SizePolicy::Minimal);
		mWidget.setLayoutPolicy(Widget::LayoutPolicy::Horizontally);

		mButton.setAction([this]() { mButton2.setMinSize(mButton2.getMinSize() + 10); });
		mButton2.setAction([this]() { mButton.setMinSize(mButton.getMinSize() + 10); } );

		RootWidget::setWidgetArea(mWidget, { 400, 100, 150, 200 });
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

	FloatingWidget mFloatingWidget;
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
