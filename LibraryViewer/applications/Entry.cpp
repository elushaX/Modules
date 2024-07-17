
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

	void processFrame(EventHandler* eventHandler, halnf delta) override {
		auto rec = RectF{ { 0, 0 }, mWindow->getSize() };

		gui.setVisible(true);
		gui.setArea(rec);
		gui.updateConfigWrapper(mWidgetManager);
		gui.procWrapper(*eventHandler, rec);
	}

	void drawFrame(Canvas* canvas) override { gui.drawWrapper(*canvas); }

private:
	Player player;
	Library library;

	tp::WidgetManager mWidgetManager;
	LibraryWidget gui;
};

int main() {

	LibraryViewer lib;
	lib.run();
}
