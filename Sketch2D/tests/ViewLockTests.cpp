#include "TestSketchesRoot.hpp"
#include "Sketch2DUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class LockViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class LockIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	struct LockScene {
		Document document;
		Brush brush;
		LockViewport viewport;
		LockIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		LockScene() { root.canvas.settings().viewStiffness = 1; }
	};
}

SUITE(ViewLock) {
	TEST(LockedViewIgnoresZoomPanAndRotate) {
		LockScene scene;
		scene.root.commands.run(Command::LockView);
		CHECK(scene.root.canvas.settings().lockView);
		CHECK_EQUAL(std::string("View locked"), scene.root.lastNotice());
		const Vec2F pan = scene.document.panWorld();
		scene.root.commands.run(Command::ZoomIn);
		scene.root.commands.run(Command::RotateLeft);
		const Vec2F center = scene.root.canvas.windowRect().pos + Vec2F{ 300.0f, 300.0f };
		scene.driver.moveTo(center);
		scene.driver.press(PointerButton::Middle);
		scene.driver.moveTo(center + Vec2F{ 100.0f, 50.0f });
		scene.driver.release(PointerButton::Middle);
		CHECK_CLOSE(1.0f, scene.document.zoom(), 1e-6f);
		CHECK_CLOSE(0.0f, scene.document.rotation(), 1e-6f);
		CHECK_CLOSE(pan.x, scene.document.panWorld().x, 1e-4f);
		CHECK_CLOSE(pan.y, scene.document.panWorld().y, 1e-4f);

		scene.root.commands.run(Command::LockView);
		scene.root.commands.run(Command::ZoomIn);
		CHECK(scene.document.zoom() > 1.0f);
	}

	TEST(FullScreenTogglesThroughTheWindow) {
		LockScene scene;
		CHECK(!scene.root.commands.isEnabled(Command::FullScreen));
		bool fullScreen = false;
		scene.root.isFullScreen = [&fullScreen]() { return fullScreen; };
		scene.root.setFullScreen = [&fullScreen](bool value) { fullScreen = value; };
		scene.driver.key(InputID::F11);
		CHECK(fullScreen);
		CHECK(scene.root.commands.isChecked(Command::FullScreen));
		scene.root.commands.run(Command::FullScreen);
		CHECK(!fullScreen);
	}

	TEST(ViewMenuListsLockAndFullScreen) {
		LockScene scene;
		std::vector<std::string> texts;
		for (const auto& item : scene.root.viewMenu.items())
			texts.push_back(item->entry().text);
		bool lock = false, full = false;
		for (const std::string& text : texts) {
			lock = lock || text == "Lock view";
			full = full || text == "Full screen";
		}
		CHECK(lock);
		CHECK(full);
	}
}
