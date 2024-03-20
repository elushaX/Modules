
#include "GraphicApplication.hpp"

#include "GUI.h"

using namespace tp;
using namespace obj;

class ExampleGUI : public Application {
public:
	ExampleGUI() {
		gui.cd(NDO->create("dict"), "root");
	}

	void processFrame(EventHandler* eventHandler) override {
	}

	void drawFrame(Canvas* canvas) override {
		canvas->rect({ { 0, 0 }, mWindow->getSize() }, RGBA(0.f, 0.f, 0.f, 1.f), 0);
		gui.draw();
	}

private:
	ObjectsGUI gui;
};

int main() {

	tp::ModuleManifest* deps[] = { &gModuleObjects, &gModuleGraphics, nullptr };
	tp::ModuleManifest module("ObjectsTests", nullptr, nullptr, deps);

	if (module.initialize()) {
		{
			ExampleGUI gui;
			gui.run();
		}
		module.deinitialize();
	}

	return 0;
}
