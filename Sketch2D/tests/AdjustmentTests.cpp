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
	class AdjustViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class AdjustIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	struct AdjustScene {
		Document document;
		Brush brush;
		AdjustViewport viewport;
		AdjustIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		AdjustScene() {
			uint1* pixels = document.activeLayer()->tileAt({ 0, 0 })->pixels();
			const auto put = [pixels](int4 x, int4 r, int4 g, int4 b, int4 a) {
				uint1* pixel = pixels + ((size_t) kTileSize + (size_t) x) * 4;
				pixel[0] = (uint1) (r * a / 255);
				pixel[1] = (uint1) (g * a / 255);
				pixel[2] = (uint1) (b * a / 255);
				pixel[3] = (uint1) a;
			};
			put(1, 255, 0, 0, 255);
			put(2, 128, 128, 128, 255);
			put(3, 51, 102, 153, 128);
			document.commit("Pixels");
		}

		RGBA at(int4 x) { return document.sampleColor({ (halnf) x + 0.5f, 1.5f }, false); }

		void setSlider(LabeledSlider& slider, halnf value) {
			driver.layout();
			driver.click(slider.slider().windowPointForValue(value));
		}
	};

	void checkRgba(const RGBA& expected, const RGBA& actual, halnf tolerance = 0.02f) {
		CHECK_CLOSE(expected.r, actual.r, tolerance);
		CHECK_CLOSE(expected.g, actual.g, tolerance);
		CHECK_CLOSE(expected.b, actual.b, tolerance);
		CHECK_CLOSE(expected.a, actual.a, tolerance);
	}

	void run(Adjustment adjustment, halnf rgba[4]) { adjustment.apply(rgba); }
}

SUITE(Adjustments) {
	TEST(BrightnessAddsAndExposureMultiplies) {
		halnf pixel[4] = { 0.25f, 0.5f, 0.1f, 1 };
		Adjustment adjustment;
		adjustment.brightness = 0.1f;
		adjustment.exposure = 2;
		run(adjustment, pixel);
		CHECK_CLOSE(1.1f, pixel[0], 1e-5f);
		CHECK_CLOSE(2.1f, pixel[1], 1e-5f);
		CHECK_CLOSE(0.5f, pixel[2], 1e-5f);
	}

	TEST(HueRotatesAndSaturationScales) {
		halnf red[4] = { 1, 0, 0, 1 };
		Adjustment hue;
		hue.kind = AdjustmentKind::HueSaturation;
		hue.hueDegrees = 120;
		run(hue, red);
		CHECK_CLOSE(0.0f, red[0], 1e-4f);
		CHECK_CLOSE(1.0f, red[1], 1e-4f);
		CHECK_CLOSE(0.0f, red[2], 1e-4f);

		halnf color[4] = { 0.8f, 0.4f, 0.4f, 1 };
		Adjustment gray;
		gray.kind = AdjustmentKind::HueSaturation;
		gray.saturation = -1;
		run(gray, color);
		CHECK_CLOSE(0.6f, color[0], 1e-4f);
		CHECK_CLOSE(0.6f, color[1], 1e-4f);
		CHECK_CLOSE(0.6f, color[2], 1e-4f);

		halnf dark[4] = { 0.5f, 0.5f, 0.5f, 1 };
		Adjustment light;
		light.kind = AdjustmentKind::HueSaturation;
		light.lightness = 0.5f;
		run(light, dark);
		CHECK_CLOSE(0.75f, dark[0], 1e-4f);
	}

	TEST(ColorBalanceShiftsEachChannelTowardsItsEnd) {
		halnf pixel[4] = { 0.5f, 0.5f, 0.5f, 1 };
		Adjustment balance;
		balance.kind = AdjustmentKind::ColorBalance;
		balance.red = 0.5f;
		balance.blue = -0.5f;
		run(balance, pixel);
		CHECK_CLOSE(0.75f, pixel[0], 1e-5f);
		CHECK_CLOSE(0.5f, pixel[1], 1e-5f);
		CHECK_CLOSE(0.25f, pixel[2], 1e-5f);
	}

	TEST(ColorizeKeepsLightness) {
		halnf pixel[4] = { 0.2f, 0.6f, 0.4f, 1 };
		Adjustment colorize;
		colorize.kind = AdjustmentKind::Colorize;
		colorize.hueDegrees = 0;
		colorize.saturation = 1;
		run(colorize, pixel);
		CHECK_CLOSE(0.8f, pixel[0], 1e-4f);
		CHECK_CLOSE(0.0f, pixel[1], 1e-4f);
		CHECK_CLOSE(0.0f, pixel[2], 1e-4f);
		CHECK_CLOSE(0.4f, (pixel[0] + std::min(pixel[1], pixel[2])) / 2, 1e-4f);
		CHECK(!colorize.isNeutral());
	}

	TEST(AlphaFadesOrStrengthens) {
		halnf pixel[4] = { 1, 1, 1, 0.5f };
		Adjustment alpha;
		alpha.kind = AdjustmentKind::Alpha;
		alpha.alpha = -0.5f;
		run(alpha, pixel);
		CHECK_CLOSE(0.25f, pixel[3], 1e-5f);
		alpha.alpha = 0.5f;
		run(alpha, pixel);
		CHECK_CLOSE(0.625f, pixel[3], 1e-5f);
	}

	TEST(DialogPreviewsLiveAndApplyCommits) {
		AdjustScene scene;
		const size_t revision = scene.document.history().revision();
		scene.driver.key(InputID::U, Modifiers{ false, true, false });
		CHECK(scene.root.adjustments.isOpen());
		CHECK(scene.root.adjustments.kind() == AdjustmentKind::HueSaturation);
		CHECK_EQUAL(std::string("Hue"), scene.root.adjustments.first.name());
		scene.setSlider(scene.root.adjustments.first, 120);
		checkRgba({ 0, 1, 0, 1 }, scene.at(1));
		CHECK_EQUAL(revision, scene.document.history().revision());
		scene.setSlider(scene.root.adjustments.first, 0);
		checkRgba({ 1, 0, 0, 1 }, scene.at(1));
		scene.setSlider(scene.root.adjustments.first, 120);
		scene.root.adjustments.accept();
		CHECK(!scene.root.adjustments.isOpen());
		CHECK_EQUAL(revision + 1, scene.document.history().revision());
		checkRgba({ 0, 1, 0, 1 }, scene.at(1));
		scene.root.commands.run(Command::Undo);
		checkRgba({ 1, 0, 0, 1 }, scene.at(1));
	}

	TEST(CancelRestoresThePixels) {
		AdjustScene scene;
		scene.root.commands.run(Command::AdjustValue);
		scene.setSlider(scene.root.adjustments.first, 0.5f);
		CHECK(scene.at(2).r > 0.9f);
		const size_t revision = scene.document.history().revision();
		scene.root.adjustments.cancel();
		CHECK_CLOSE(128.0f / 255.0f, scene.at(2).r, 0.01f);
		CHECK_EQUAL(revision, scene.document.history().revision());
		CHECK(!scene.document.isAdjusting());
	}

	TEST(AdjustmentsRespectTheSelectionAndAlpha) {
		AdjustScene scene;
		scene.document.editSelection().addRectangle(0, 0, 2, 4, SelectionMode::Replace);
		scene.root.commands.run(Command::AdjustValue);
		scene.setSlider(scene.root.adjustments.first, -1);
		scene.root.adjustments.accept();
		checkRgba({ 0, 0, 0, 1 }, scene.at(1));
		CHECK_CLOSE(128.0f / 255.0f, scene.at(2).r, 0.01f);
		CHECK_CLOSE(128.0f / 255.0f, scene.at(3).a, 0.01f);
	}

	TEST(AlphaAdjustmentChangesTransparency) {
		AdjustScene scene;
		scene.root.commands.run(Command::AdjustAlpha);
		CHECK(!scene.root.adjustments.second.isVisible());
		scene.setSlider(scene.root.adjustments.first, -0.5f);
		scene.root.adjustments.accept();
		CHECK_CLOSE(0.5f, scene.at(1).a, 0.01f);
		CHECK_CLOSE(1.0f, scene.at(1).r, 0.02f);
	}

	TEST(AlphaAdjustmentLeavesAlphaLockedLayersAlone) {
		AdjustScene scene;
		scene.document.toggleAlphaLock();
		scene.root.commands.run(Command::AdjustAlpha);
		scene.setSlider(scene.root.adjustments.first, -0.5f);
		scene.root.adjustments.accept();
		CHECK_CLOSE(1.0f, scene.at(1).a, 0.01f);
	}

	TEST(EmptyLayerExplains) {
		AdjustScene scene;
		scene.document.addLayer("Empty");
		scene.root.commands.run(Command::AdjustColorize);
		CHECK(!scene.root.adjustments.isOpen());
		CHECK_EQUAL(std::string("The layer is empty"), scene.root.lastNotice());
	}

	TEST(EditAdjustMenuListsEveryAdjustment) {
		AdjustScene scene;
		std::vector<std::string> texts;
		for (const auto& item : scene.root.adjustImageMenu.items())
			texts.push_back(item->entry().text);
		CHECK(
			texts ==
			std::vector<std::string>({ "Brightness & exposure", "Hue & saturation", "Color balance", "Colorize", "Alpha" })
		);
	}
}
