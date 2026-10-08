#include "TestSketchesRoot.hpp"
#include "Sketch2DUI.hpp"
#include "StudioUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class FilterViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class FilterIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	void put(Document& document, int4 x, int4 y, const RGBA& color) {
		uint1* pixel = document.activeLayer()->tileAt({ 0, 0 })->pixels() + ((size_t) y * kTileSize + (size_t) x) * 4;
		pixel[0] = (uint1) std::lround(color.r * color.a * 255);
		pixel[1] = (uint1) std::lround(color.g * color.a * 255);
		pixel[2] = (uint1) std::lround(color.b * color.a * 255);
		pixel[3] = (uint1) std::lround(color.a * 255);
	}

	struct FilterScene {
		Document document;
		Brush brush;
		FilterViewport viewport;
		FilterIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		FilterScene() {
			put(document, 1, 1, { 1, 0, 0, 1 });
			put(document, 2, 1, { 0.2f, 0.4f, 0.6f, 0.5f });
			put(document, 3, 1, { 1, 1, 1, 1 });
			put(document, 4, 1, { 0, 0, 0, 1 });
			document.commit("Pixels");
		}

		RGBA at(int4 x, int4 y) { return document.sampleColor({ (halnf) x + 0.5f, (halnf) y + 0.5f }, false); }
	};

	void checkRgba(const RGBA& expected, const RGBA& actual) {
		CHECK_CLOSE(expected.r, actual.r, 0.02f);
		CHECK_CLOSE(expected.g, actual.g, 0.02f);
		CHECK_CLOSE(expected.b, actual.b, 0.02f);
		CHECK_CLOSE(expected.a, actual.a, 0.02f);
	}
}

SUITE(PixelFilters) {
	TEST(InvertFlipsColorsAndKeepsAlpha) {
		FilterScene scene;
		scene.root.commands.run(Command::Invert);
		checkRgba({ 0, 1, 1, 1 }, scene.at(1, 1));
		checkRgba({ 0.8f, 0.6f, 0.4f, 0.5f }, scene.at(2, 1));
		CHECK_CLOSE(0.0f, scene.at(10, 10).a, 1e-6f);
		scene.root.commands.run(Command::Undo);
		checkRgba({ 1, 0, 0, 1 }, scene.at(1, 1));
	}

	TEST(DesaturateUsesLuma) {
		FilterScene scene;
		scene.root.commands.run(Command::Desaturate);
		checkRgba({ 0.299f, 0.299f, 0.299f, 1 }, scene.at(1, 1));
		const halnf value = 0.299f * 0.2f + 0.587f * 0.4f + 0.114f * 0.6f;
		checkRgba({ value, value, value, 0.5f }, scene.at(2, 1));
	}

	TEST(ValueToAlphaTurnsWhiteTransparent) {
		FilterScene scene;
		scene.root.commands.run(Command::ValueToAlpha);
		CHECK(scene.at(3, 1).a < 0.01f);
		checkRgba({ 0, 0, 0, 1 }, scene.at(4, 1));
		checkRgba({ 0, 0, 0, 0.701f }, scene.at(1, 1));
	}

	TEST(ValueToAlphaInvertedTurnsBlackTransparent) {
		FilterScene scene;
		scene.root.commands.run(Command::ValueToAlphaInverted);
		CHECK(scene.at(4, 1).a < 0.01f);
		checkRgba({ 1, 1, 1, 1 }, scene.at(3, 1));
		checkRgba({ 1, 1, 1, 0.299f }, scene.at(1, 1));
	}

	TEST(FiltersRespectTheSelection) {
		FilterScene scene;
		scene.document.editSelection().addRectangle(0, 0, 2, 2, SelectionMode::Replace);
		scene.root.commands.run(Command::Invert);
		checkRgba({ 0, 1, 1, 1 }, scene.at(1, 1));
		checkRgba({ 1, 1, 1, 1 }, scene.at(3, 1));
	}

	TEST(AlphaLockRefusesValueToAlphaButAllowsInvert) {
		FilterScene scene;
		scene.document.toggleAlphaLock();
		scene.root.commands.run(Command::ValueToAlpha);
		CHECK_EQUAL(std::string("You can't change alpha on a alpha locked layer"), scene.root.lastNotice());
		checkRgba({ 1, 1, 1, 1 }, scene.at(3, 1));
		scene.root.commands.run(Command::Invert);
		checkRgba({ 0, 0, 0, 1 }, scene.at(3, 1));
	}

	TEST(EmptyLayerExplains) {
		FilterScene scene;
		scene.document.addLayer("Empty");
		const size_t revision = scene.document.history().revision();
		scene.root.commands.run(Command::Desaturate);
		CHECK_EQUAL(std::string("The layer is empty"), scene.root.lastNotice());
		CHECK_EQUAL(revision, scene.document.history().revision());
	}

	TEST(EditMenuListsFiltersAndPixelEdits) {
		FilterScene scene;
		std::vector<std::string> texts;
		for (const auto& item : scene.root.editMenu.items())
			texts.push_back(item->entry().text);
		for (const char* text : { "Fill color",
															"Clear",
															"Crop",
															"Free Transform",
															"Flip horizontal",
															"Flip vertical",
															"Invert",
															"Desaturate",
															"Copy value to alpha",
															"Copy value to alpha inverted" }) {
			bool found = false;
			for (const std::string& existing : texts)
				found = found || existing == text;
			CHECK(found);
		}
	}

	TEST(TabletQuickMenuHasTheEditMenu) {
		Document document;
		Brush brush;
		FilterViewport viewport;
		FilterIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };
		UiDriver driver{ studio, { 1280, 800 } };
		bool found = false;
		for (const auto& item : classic.mainMenu.items())
			if (item->entry().text == "Edit" && item->entry().submenu == &classic.editMenu) found = true;
		CHECK(found);
	}
}
