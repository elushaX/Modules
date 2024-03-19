
#include "Graphics.hpp"

#include "GUI.h"

using namespace tp;
using namespace obj;

int main() {

	tp::ModuleManifest* deps[] = { &gModuleObjects, &gModuleGraphics, nullptr };
	tp::ModuleManifest module("ObjectsTests", nullptr, nullptr, deps);

	if (module.initialize()) {
		auto window = Window::createWindow({ 1500, 900 }, "Objects GUI");

		{
			Graphics graphics(window);
			ObjectsGUI gui;

			gui.cd(NDO->create("dict"), "root");

			while (!window->shouldClose()) {
				window->processEvents();
				graphics.proc();
				gui.draw();
				graphics.draw();
				window->draw();
			}
		}

		Window::destroyWindow(window);

		module.deinitialize();
	}

	return 0;
}
