
#include "Window.hpp"

#include "imgui.h"

int main() {
	tp::ModuleManifest* deps[] = { &tp::gModuleGraphics, nullptr };
	tp::ModuleManifest testModule("Example", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	{
		auto window = tp::Window::createWindow(800, 600, "Window 1");

		if (window) {
			while (!window->shouldClose()) {
				window->processEvents();

				window->getGraphics().getDebugGui().drawDebugInfoWindow();

				window->draw();
			}
		}

		tp::Window::destroyWindow(window);
	}

	testModule.deinitialize();
}
