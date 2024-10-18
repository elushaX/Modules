
#include "WidgetApplication.hpp"

#include "Sketch3D.hpp"
#include "Sketch3DWidget.hpp"

using namespace tp;

class Sketch3DApplication : public WidgetApplication {
public:
	Sketch3DApplication() :
		mGui(*mGraphics->getCanvas(), { 1920, 1080 }) {
		setRoot(&mGui);
	}

private:
	Sketch3DGUI mGui;
};

void runApp() {
	Sketch3DApplication app;
	app.run();
}

int main() { runApp(); }