
#include "GUI.hpp"
#include "Player.hpp"
#include "GraphicApplication.hpp"

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

using namespace tp;

class LibraryViewer : public Application {
public:
	LibraryViewer() : gui(&library, &player) {
		library.loadJson(getHome() + "Library.json");
		library.checkExisting();

		gui.updateTracks();
	}

	void processFrame(EventHandler* eventHandler) override {
		auto rec = RectF{ { 0, 0 }, mWindow->getSize() };
		gui.proc(*eventHandler, rec, rec);
	}

	void drawFrame(Canvas* canvas) override {
		gui.draw(*canvas);
	}

private:
	Player player;
	Library library;
	LibraryWidget<EventHandler, Canvas> gui;
};

void runApp() {
	tp::GlobalGUIConfig config;
	tp::gGlobalGUIConfig = &config;

	LibraryViewer lib;
	lib.run();
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
