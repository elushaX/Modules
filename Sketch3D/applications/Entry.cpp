
#include "GraphicApplication.hpp"

#include "Sketch3D.hpp"
#include "Sketch3DWidget.hpp"

using namespace tp;

class Sketch3DApplication : public Application {
public:
	Sketch3DApplication() : mGui(*mGraphics->getCanvas(), {1920, 1080}) {}

	void processFrame(EventHandler* eventHandler) override {
		auto rec = RectF( { 0, 0 }, mWindow->getSize() );
		mGui.proc(*eventHandler, rec, rec);
	}

	void drawFrame(Canvas* canvas) override {
		mGui.draw(*canvas);
	}

private:
	Sketch3DGUI<EventHandler, Canvas> mGui;
};


void runApp() {
	tp::GlobalGUIConfig config;
	tp::gGlobalGUIConfig = &config;

	Sketch3DApplication app;
	app.run();
}

int main() {
	runApp();
}