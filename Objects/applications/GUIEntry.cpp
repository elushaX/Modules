
#include "Window.hpp"

#include "GUI.h"

using namespace tp;
using namespace obj;

class GUIWindow {
	Window* window;
	ObjectsGUI gui;

public:
	GUIWindow() {
		tp::HeapAllocGlobal::startIgnore();
		window = Window::createWindow(1500, 900, "Objects GUI");
		tp::HeapAllocGlobal::stopIgnore();

		gui.cd(NDO->create("dict"), "root");
	}

	void run() {
		while (!window->shouldClose()) {
			window->processEvents();
			gui.draw();
			window->draw();
		}
	}

	~GUIWindow() { Window::destroyWindow(window); }
};

int main() {

	tp::ModuleManifest* deps[] = { &gModuleObjects, &gModuleGraphics, nullptr };
	tp::ModuleManifest module("ObjectsTests", nullptr, nullptr, deps);

	if (module.initialize()) {
		{
			GUIWindow window;
			window.run();
		}
		module.deinitialize();
	}

	return 0;
}
