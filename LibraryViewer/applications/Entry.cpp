
#include "GUI.hpp"
#include "WidgetApplication.hpp"

using namespace tp;

class LibraryViewer : public WidgetApplication {
public:
	LibraryViewer() :
		gui(&library, &player) {
		library.loadJson(getHome() + "Library.json");
		library.checkExisting();

		gui.updateTracks();

		setRoot(&gui);
	}

private:
	Player player;
	Library library;

	LibraryWidget gui;
};

int main() {

	LibraryViewer lib;
	lib.run();
}
