
#include "ChatGUI.hpp"

#include "GraphicApplication.hpp"

using namespace tp;

class SimpleGUI : public Application {
public:
	SimpleGUI() {
		mGui.addWidget(&mButton);
		mGui.addWidget(&mLabel);
		mGui.addWidget(&mSlider);
	}

	void processFrame(EventHandler* eventHandler) override {
		const auto rec = RectF({ 0, 0 }, mWindow->getSize());

		mGui.setArea(rec);
		mGui.setVisible(true);
		mGui.updateConfigWrapper(mWidgetManager);
		mGui.procWrapper(*eventHandler);
	}

	void drawFrame(Canvas* canvas) override {
		canvas->rect(mGui.mArea, { 0, 0, 0, 1 });
		mGui.drawWrapper(*canvas);
	}

private:
	WidgetManager mWidgetManager;
	ScrollableWindow<EventHandler, Canvas> mGui;

	ButtonWidget<EventHandler, Canvas> mButton;
	LabelWidget<EventHandler, Canvas> mLabel;
	NamedSliderWidget<EventHandler, Canvas> mSlider;
};

int main() {
	{
		SimpleGUI gui;
		gui.run();
	}
}
