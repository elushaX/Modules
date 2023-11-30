
#include "NewPlacement.hpp"
#include "WavPlayer.hpp"
#include "LibraryGui.hpp"

// load mpre track info!!
// display if track presents on fylesystem!!
// enable playback!!
// monitor resource usage
// sorting

void runApp() {

	TrackPlayer player;

	Library library;
	library.loadJson(getHome() + "Library.json");

	auto gui = LibraryWidget<tp::Window::Events, tp::Graphics::Canvas>(&library, &player);

  auto window = tp::Window::createWindow(800, 600, "Window 1");

	if (window) {
		while (!window->shouldClose()) {
			window->processEvents();
			
			auto area = window->getCanvas().getAvaliableArea();
			
			gui.proc(window->getEvents(), {}, { area.x, area.y, area.z, area.w });
			gui.draw(window->getCanvas(), {}, { area.x, area.y, area.z, area.w });

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

  runApp();
  
	binModule.deinitialize();
}
