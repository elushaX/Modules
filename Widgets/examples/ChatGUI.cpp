
#include "ChatGUI.hpp"

#include "GraphicApplication.hpp"

using namespace tp;

class ExampleGUI : public Application {
public:
	ExampleGUI() = default;

	void processFrame(EventHandler* eventHandler) override {
		auto rec = RectF({ 0, 0 }, mWindow->getSize());

		mGui.updateConfigCache(mWidgetManager);

		mGui.proc(*eventHandler, rec, rec);
	}

	void drawFrame(Canvas* canvas) override { mGui.draw(*canvas); }

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