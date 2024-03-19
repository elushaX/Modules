
#include "ExampleGUI.hpp"

#include "Graphics.hpp"

using namespace tp;

void runApp() {
	auto window = tp::Window::createWindow({ 800, 600 }, "Window 1");

	tp::ComplexWidget<tp::EventHandler, tp::Canvas> gui;

	if (window) {
		while (!window->shouldClose()) {
			window->processEvents();

			auto area = window->getCanvas().getAvaliableArea();

			gui.proc(window->getEvents(), {}, { area.x, area.y, area.z, area.w });
			gui.draw(window->getCanvas());

			tp::sleep(20);

			window->draw();
		}
	}

	tp::Window::destroyWindow(window);
}

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleWidgets, nullptr };
	tp::ModuleManifest binModule("Chat", nullptr, nullptr, deps);

	if (!binModule.initialize()) {
		return 1;
	}

	{
		tp::GlobalGUIConfig config;
		tp::gGlobalGUIConfig = &config;
		runApp();
	}

	binModule.deinitialize();
}
