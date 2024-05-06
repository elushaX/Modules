
#include "GUI.hpp"
#include "GraphicApplication.hpp"

using namespace tp;

class LibraryViewer : public Application {
public:
	LibraryViewer() :
		gui(&library, &player) {
		library.loadJson(getHome() + "Library.json");
		library.checkExisting();

		gui.updateTracks();
	}

	void processFrame(EventHandler* eventHandler) override {
		auto rec = RectF{ { 0, 0 }, mWindow->getSize() };

		gui.updateConfigCache(mWidgetManager);
		gui.proc(*eventHandler, rec, rec);
	}

	void drawFrame(Canvas* canvas) override { gui.draw(*canvas); }

private:
	Player player;
	Library library;

	tp::WidgetManager mWidgetManager;
	LibraryWidget<EventHandler, Canvas> gui;
};

int main() {

	LibraryViewer lib;
	lib.run();
}
