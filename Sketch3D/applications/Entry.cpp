
#include "GraphicApplication.hpp"

#include "Sketch3D.hpp"
#include "Sketch3DWidget.hpp"

using namespace tp;

class Sketch3DApplication : public Application {
public:
	Sketch3DApplication() :
		mGui(*mGraphics->getCanvas(), { 1920, 1080 }) {}

	void processFrame(EventHandler* eventHandler, halnf delta) override {
		auto rec = RectF({ 0, 0 }, mWindow->getSize());

		mGui.setVisible(true);
		mGui.setArea(rec);
		mGui.updateConfigWrapper(mWidgetManager);
		mGui.procWrapper(*eventHandler, rec);
	}

	void drawFrame(Canvas* canvas) override { mGui.drawWrapper(*canvas); }

private:
	WidgetManager mWidgetManager;
	Sketch3DGUI mGui;
};

void runApp() {
	Sketch3DApplication app;
	app.run();
}

int main() { runApp(); }