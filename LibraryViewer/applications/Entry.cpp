
#include "NewPlacement.hpp"
#include "WavPlayer.hpp"
#include "LibraryGui.hpp"

// monitor resource usage
// sorting

void runApp() {

	TrackPlayer player;

	player.startStreamTrack(0);
	player.playSong();

	Library library;
	library.loadJson("library/Library.json");

	auto gui = LibraryWidget<tp::Window::Events, tp::Graphics::Canvas>(&library);

  auto window = tp::Window::createWindow(800, 600, "Window 1");

	if (window) {
		while (!window->shouldClose()) {
			window->processEvents();
			
			auto area = window->getCanvas().getAvaliableArea();
			
			gui.proc(window->getEvents(), {}, { area.x, area.y, area.z, area.w });
			gui.draw(window->getCanvas(), {}, { area.x, area.y, area.z, area.w });

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
