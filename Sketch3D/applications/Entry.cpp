
#include "Sketch3D.hpp"

#include "Graphics.hpp"
#include "Window.hpp"
#include "Widgets.hpp"

void runApp() {

	tp::GlobalGUIConfig config;
	tp::gGlobalGUIConfig = &config;

	tp::LabelWidget<tp::Window::Events, tp::Graphics::Canvas> gui;

	auto window = tp::Window::createWindow(800, 600, "Window 1");

	if (window) {
		while (!window->shouldClose()) {
			window->processEvents();

			auto area = window->getCanvas().getAvaliableArea();

			gui.proc(window->getEvents(), { area.x, area.y, area.z, area.w }, { area.x, area.y, area.z, area.w });
			gui.draw(window->getCanvas());

			tp::sleep(100);

			window->draw();
		}
	}

	tp::Window::destroyWindow(window);
}

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleGraphics, &tp::gModuleStrings, &tp::gModuleWidgets, nullptr };
	tp::ModuleManifest binModule("LibViewEntry", nullptr, nullptr, deps);

	if (!binModule.initialize()) {
		return 1;
	}

	tp::HeapAllocGlobal::disableCallstack();

	runApp();

	binModule.deinitialize();
}