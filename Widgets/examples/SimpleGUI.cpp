
#include "SimpleGUI.hpp"

#include "GraphicApplication.hpp"


using namespace tp;

class SimpleGUI : public Application {
public:
	SimpleGUI() {

	}

	void processFrame(EventHandler* eventHandler) override {
		const auto rec = RectF({ 0, 0 }, mWindow->getSize());

		mGui.setArea(rec);
		mGui.updateConfigWrapper(mWidgetManager);
		mGui.procWrapper(*eventHandler, rec);
	}

	void drawFrame(Canvas* canvas) override {
		canvas->rect(mGui.mArea, { 0.1f, 0.1f, 0.1f, 1.f });
		mGui.drawWrapper(*canvas);
	}

private:
	WidgetManager mWidgetManager;

	SimpleWidget2 mGui;
};

int main() {
	{
		SimpleGUI gui;
		gui.run();
	}
}
