
#include "GraphicApplication.hpp"

#include "GUI.h"

using namespace tp;
using namespace obj;

class SimpleGUI : public Application {
public:
	SimpleGUI() { gui.cd(objects_api::create<DictObject>(), "root"); }

	void processFrame(EventHandler* eventHandler, halnf delta) override {}

	void drawFrame(Canvas* canvas) override {
		canvas->rect({ { 0, 0 }, mWindow->getSize() }, RGBA(0.f, 0.f, 0.f, 1.f), 0);
		gui.draw();
	}

private:
	ObjectsGUI gui;
};

int main() {

	tp::ModuleManifest* deps[] = { &gModuleObjects, nullptr };
	tp::ModuleManifest module("ObjectsTests", nullptr, nullptr, deps);

	if (module.initialize()) {
		{
			SimpleGUI gui;
			gui.run();
		}
		module.deinitialize();
	}

	return 0;
}
