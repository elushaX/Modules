#include "AndroidEntry.hpp"
#include "Gallery.hpp"
#include "WidgetApplication.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
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

	int runGallery() {
		GalleryApplication application;
		application.run();
		return 0;
	}
}

extern "C" void android_main(android_app* app) {
	const char* assets[] = { "rsc" };
	runAndroidApplication(app, runGallery, assets, 1);
}
