
#include "WidgetApplication.hpp"

#include "Sketch3D.hpp"
#include "SketchGUI.hpp"

using namespace tp;

class Sketch3DApplication : public WidgetApplication {
public:
	Sketch3DApplication() {
		setRoot(&mGui);

		mGui.createRenderWidget(mGraphics->getCanvas(), { 2560, 1440 });
		mGui.setProject(&mSketch);
	}

private:
	SketchGUI mGui;
	Project mSketch;
};

void runApp() {
	Sketch3DApplication app;
	app.run();
}

int main() { runApp(); }
