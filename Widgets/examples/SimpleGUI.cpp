
#include "ChatGUI.hpp"

#include "GraphicApplication.hpp"

using namespace tp;

class SimpleGUI : public Application {
public:
	SimpleGUI() {
		mGui.mContents.append(&mButton);
		mGui.mContents.append(&mSlider);
		mGui.mContents.append(&mLabel);
	}

	void processFrame(EventHandler* eventHandler) override {

		mGui.updateConfigCache(mWidgetManager);

		mSlider.updateConfigCache(mWidgetManager);
		mLabel.updateConfigCache(mWidgetManager);
		mButton.updateConfigCache(mWidgetManager);

		const auto rec = RectF({ 0, 0 }, mWindow->getSize());
		mGui.proc(*eventHandler, rec, rec);
	}

	void drawFrame(Canvas* canvas) override { mGui.draw(*canvas); }

private:
	WidgetManager mWidgetManager;
	ScrollableWindow<EventHandler, Canvas> mGui;

	ButtonWidget<EventHandler, Canvas> mButton;
	LabelWidget<EventHandler, Canvas> mLabel;
	SliderWidget<EventHandler, Canvas> mSlider;
};

int main() {
	{
		SimpleGUI gui;
		gui.run();
	}
}
