
#include "ChatGUI.hpp"

#include "GraphicApplication.hpp"

using namespace tp;

class ExampleGUI : public Application {
public:
	ExampleGUI() = default;

	void processFrame(EventHandler* eventHandler) override {
		auto rec = RectF({ 0, 0 }, mWindow->getSize());

		mGui.updateConfigWrapper(mWidgetManager);
		mGui.setArea(rec);
		mGui.setVisible(true);

		mGui.procWrapper(*eventHandler, rec);
	}

	void drawFrame(Canvas* canvas) override { mGui.drawWrapper(*canvas); }

private:
	WidgetManager mWidgetManager;
	ComplexWidget<EventHandler, Canvas> mGui;
};

int main() {
	{
		ExampleGUI gui;
		gui.run();
	}
}
