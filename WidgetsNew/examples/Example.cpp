
#include "GraphicApplication.hpp"

#include "RootWidget.hpp"
#include "AnimationTestWidget.hpp"
#include "FloatingWidget.hpp"
#include "SimpleWidgets.hpp"

using namespace tp;

class WidgetApplication : public Application {
public:
	WidgetApplication() {
		mRootWidget.setRootWidget(&mWidget);

		mWidget.addChild(&mWidgetFloating);

		RootWidget::setWidgetArea(mWidgetFloating, { 100, 100, 150, 200 });

		mWidgetFloating.addChild(&mButton);
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

	FloatingWidget mWidgetFloating;

	AnimationTestWidget mWidget;
	RootWidget mRootWidget;
};

int main() {
	{
		WidgetApplication gui;
		gui.run();
	}
}
