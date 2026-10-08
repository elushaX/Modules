#include "Gallery.hpp"
#include "WidgetApplication.hpp"

using namespace tp;
using namespace tp::ui;

class GalleryApplication : public WidgetApplication {
public:
	GalleryApplication() {
		setMenuBarVisible(false);
		debugTools().attachTo(mGallery.menuBar);
		setRoot(mGallery);
	}

	~GalleryApplication() override { debugTools().detach(); }

	[[nodiscard]] AppStorage persistentAppStorage() const override { return AppStorage::inHome("widgets-gallery"); }

private:
	gallery::Gallery mGallery;
};

int main() {
	GalleryApplication application;
	application.run();
	return 0;
}
