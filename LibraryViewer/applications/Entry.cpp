
#include "GUI.hpp"
#include "Player.hpp"

// 1) artworks
// 2) how to easily add more songs?
// 3) GUi :
// - seeker
// - song idx
// - non-existing highlight
// - prev next
// - remove debug gui
// 4) queue & repeat & shuffle...
// 5) new database with history

void runApp() {

	Player player;
	Library library;
	library.loadJson(getHome() + "Library.json");
	library.checkExisting();

	tp::GlobalGUIConfig config;
	tp::gGlobalGUIConfig = &config;

	tp::LibraryWidget<tp::Window::Events, tp::Graphics::Canvas> gui(&library, &player);

	auto window = tp::Window::createWindow(800, 600, "Window 1");

	if (window) {
		while (!window->shouldClose()) {
			window->processEvents();

			auto area = window->getCanvas().getAvaliableArea();

			gui.proc(window->getEvents(), { area.x, area.y, area.z, area.w }, { area.x, area.y, area.z, area.w });
			gui.draw(window->getCanvas());

			// tp::sleep(100);

			window->draw();
		}
	}

	tp::Window::destroyWindow(window);
}

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleLibraryViewer, nullptr };
	tp::ModuleManifest binModule("LibViewEntry", nullptr, nullptr, deps);

	if (!binModule.initialize()) {
		return 1;
	}

	tp::HeapAllocGlobal::disableCallstack();

	runApp();

	binModule.deinitialize();
}
