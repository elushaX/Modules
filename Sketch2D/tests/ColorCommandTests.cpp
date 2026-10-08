#include "TestSketchesRoot.hpp"
#include "ColorModel.hpp"
#include "FakeWindowHost.hpp"
#include "Sketch2DUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class ColorViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class ColorIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string&) const override { return { 1 }; }
	};

	struct ColorScene {
		Document document;
		Brush brush;
		ColorViewport viewport;
		ColorIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		FakeWindowHost host;

		ColorScene() { root.canvas.settings().viewStiffness = 1; }
	};

	void checkHue(const HueColor& expected, const HueColor& actual) {
		CHECK_CLOSE(expected.hueDegrees, actual.hueDegrees, 0.05f);
		CHECK_CLOSE(expected.saturation, actual.saturation, 1e-3f);
		CHECK_CLOSE(expected.value, actual.value, 1e-3f);
	}

	const MenuItem* findItem(Menu& menu, const std::string& text) {
		for (const auto& item : menu.items())
			if (item->text() == text) return item.get();
		return nullptr;
	}
}

SUITE(ColorModel) {
	TEST(DefaultsMatchReference) {
		ColorPair pair;
		checkHue({ 220, 0.6f, 0.5f }, pair.primary());
		checkHue({ 0, 0, 1 }, pair.secondary());
	}

	TEST(RgbConvertsBackToHue) {
		for (const HueColor& color : { HueColor{ 0, 1, 1 },
																	 HueColor{ 120, 0.5f, 0.8f },
																	 HueColor{ 220, 0.6f, 0.5f },
																	 HueColor{ 300, 0.25f, 0.3f } })
			checkHue(color, hueColorOf(color.rgba()));
		checkHue({ 0, 0, 0.4f }, hueColorOf({ 0.4f, 0.4f, 0.4f, 1 }));
	}

	TEST(HexRoundTrips) {
		CHECK_EQUAL(std::string("FF8000"), colorHex({ 1, 0.5f, 0, 1 }));
		const auto parsed = parseColorHex("#ff8000");
		REQUIRE CHECK(parsed.has_value());
		CHECK_CLOSE(1.0f, parsed->r, 1e-3f);
		CHECK_CLOSE(128.0f / 255.0f, parsed->g, 1e-3f);
		CHECK(parseColorHex(" 0f0 ").has_value());
		CHECK_CLOSE(1.0f, parseColorHex("0f0")->g, 1e-3f);
		CHECK(!parseColorHex("12345").has_value());
		CHECK(!parseColorHex("zz0000").has_value());
		CHECK(!parseColorHex("").has_value());
	}

	TEST(DefaultColorsSetBlackOnWhiteKeepingHues) {
		ColorPair pair;
		pair.resetToDefaults();
		checkHue({ 220, 0.6f, 0 }, pair.primary());
		checkHue({ 0, 0, 1 }, pair.secondary());
	}

	TEST(SwapExchangesBothColors) {
		ColorPair pair;
		pair.swap();
		checkHue({ 0, 0, 1 }, pair.primary());
		checkHue({ 220, 0.6f, 0.5f }, pair.secondary());
	}

	TEST(CoolerMovesTowardBlueWithoutPassingIt) {
		ColorPair pair;
		pair.setPrimary(HueColor{ 100, 1, 1 });
		pair.cooler();
		CHECK_CLOSE(110.0f, pair.primary().hueDegrees, 1e-3f);
		pair.setPrimary(HueColor{ 235, 1, 1 });
		pair.cooler();
		CHECK_CLOSE(240.0f, pair.primary().hueDegrees, 1e-3f);
		pair.setPrimary(HueColor{ 300, 1, 1 });
		pair.cooler();
		CHECK_CLOSE(290.0f, pair.primary().hueDegrees, 1e-3f);
	}

	TEST(WarmerMovesTowardRedWithoutWrapping) {
		ColorPair pair;
		pair.setPrimary(HueColor{ 100, 1, 1 });
		pair.warmer();
		CHECK_CLOSE(90.0f, pair.primary().hueDegrees, 1e-3f);
		pair.setPrimary(HueColor{ 5, 1, 1 });
		pair.warmer();
		CHECK_CLOSE(0.0f, pair.primary().hueDegrees, 1e-3f);
		pair.setPrimary(HueColor{ 300, 1, 1 });
		pair.warmer();
		CHECK_CLOSE(310.0f, pair.primary().hueDegrees, 1e-3f);
		pair.setPrimary(HueColor{ 355, 1, 1 });
		pair.warmer();
		CHECK_CLOSE(0.0f, pair.primary().hueDegrees, 1e-3f);
	}

	TEST(HueRotatesAndComplements) {
		ColorPair pair;
		pair.setPrimary(HueColor{ 350, 1, 1 });
		pair.rotateHue(ColorPair::kHueStep);
		CHECK_CLOSE(0.0f, pair.primary().hueDegrees, 1e-3f);
		pair.rotateHue(-ColorPair::kHueStep);
		CHECK_CLOSE(350.0f, pair.primary().hueDegrees, 1e-3f);
		pair.complement();
		CHECK_CLOSE(170.0f, pair.primary().hueDegrees, 1e-3f);
	}

	TEST(SaturationAndValueStayInRange) {
		ColorPair pair;
		pair.setPrimary(HueColor{ 0, 0.02f, 0.98f });
		pair.changeSaturation(-ColorPair::kAmountStep);
		pair.changeValue(ColorPair::kAmountStep);
		CHECK_CLOSE(0.0f, pair.primary().saturation, 1e-6f);
		CHECK_CLOSE(1.0f, pair.primary().value, 1e-6f);
	}

	TEST(GrayRgbKeepsThePreviousHue) {
		ColorPair pair;
		pair.setPrimary(RGBA(0.5f, 0.5f, 0.5f, 1));
		checkHue({ 220, 0, 0.5f }, pair.primary());
	}
}

SUITE(ColorCommands) {
	TEST(CommandIdsMatchTheReference) {
		CHECK_EQUAL(1000229, (int4) Command::DefaultColors);
		CHECK_EQUAL(1000231, (int4) Command::CopyColor);
		CHECK_EQUAL(1000232, (int4) Command::PasteColor);
		CHECK_EQUAL(1000241, (int4) Command::Brighter);
		CHECK_EQUAL(1000252, (int4) Command::Lightness100);
	}

	TEST(ColorMenuChangesTheBrushColor) {
		ColorScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		Menu& menu = scene.root.colorMenu;
		for (const char* text : { "Default colors",
															"Swap colors",
															"Copy",
															"Paste",
															"Darker",
															"Brighter",
															"Grayer",
															"Purer",
															"Cooler",
															"Warmer",
															"Hue compliment",
															"Hue rotate left",
															"Hue rotate right",
															"Lightness" })
			CHECK(findItem(menu, text) != nullptr);

		findItem(menu, "Default colors")->entry().action();
		CHECK_CLOSE(0.0f, scene.brush.color.r + scene.brush.color.g + scene.brush.color.b, 1e-4f);
		findItem(menu, "Swap colors")->entry().action();
		CHECK_CLOSE(3.0f, scene.brush.color.r + scene.brush.color.g + scene.brush.color.b, 1e-4f);
		CHECK_EQUAL(std::string("Swap colors"), scene.root.lastNotice());
	}

	TEST(ShortcutsAdjustTheBrushColor) {
		ColorScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.color = HueColor{ 0, 1, 1 }.rgba();
		driver.key(InputID::W, alt(InputID::W).modifiers);
		checkHue({ 10, 1, 1 }, hueColorOf(scene.brush.color));
		driver.key(InputID::Z, alt(InputID::Z).modifiers);
		checkHue({ 10, 1, 0.95f }, hueColorOf(scene.brush.color));
		driver.key(InputID::A, alt(InputID::A).modifiers);
		checkHue({ 10, 0.95f, 0.95f }, hueColorOf(scene.brush.color));
		driver.key(InputID::N3, alt(InputID::N3).modifiers);
		checkHue({ 10, 0.95f, 0.3f }, hueColorOf(scene.brush.color));
		CHECK_EQUAL(std::string("Lightness 30%"), scene.root.lastNotice());
	}

	TEST(ZeroThenDigitPicksAFineLightness) {
		ColorScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.color = ColorPair::kDefaultPrimary.rgba();
		driver.key(InputID::N0, alt(InputID::N0).modifiers);
		checkHue({ 220, 0.6f, 1.0f }, scene.root.colors.primary());
		driver.key(InputID::N4, alt(InputID::N4).modifiers);
		CHECK_CLOSE(0.04f, scene.root.colors.primary().value, 1e-4f);
		driver.advance(400);
		driver.key(InputID::N4, alt(InputID::N4).modifiers);
		CHECK_CLOSE(0.4f, scene.root.colors.primary().value, 1e-4f);
	}

	TEST(WheelEditsAreThePrimaryColor) {
		ColorScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.color = HueColor{ 120, 1, 0.5f }.rgba();
		scene.root.commands.run(Command::Brighter);
		checkHue({ 120, 1, 0.55f }, hueColorOf(scene.brush.color));
	}

	TEST(CopyAndPasteUseTheClipboard) {
		ColorScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.ui().setWindowHost(&scene.host);
		scene.brush.color = RGBA(1, 0.5f, 0, 1);
		scene.root.commands.run(Command::CopyColor);
		CHECK_EQUAL(std::string("FF8000"), scene.host.clipboard);
		CHECK_EQUAL(std::string("Copied FF8000"), scene.root.lastNotice());

		scene.host.clipboard = "#00ff00";
		scene.root.commands.run(Command::PasteColor);
		CHECK_CLOSE(1.0f, scene.brush.color.g, 1e-3f);
		CHECK_CLOSE(0.0f, scene.brush.color.r, 1e-3f);

		scene.host.clipboard = "hello";
		scene.root.commands.run(Command::PasteColor);
		CHECK_EQUAL(std::string("The clipboard has no color"), scene.root.lastNotice());
		CHECK_CLOSE(1.0f, scene.brush.color.g, 1e-3f);
		driver.ui().setWindowHost(nullptr);
	}

	TEST(ColorCommandsLeaveEraserMode) {
		ColorScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.setEraserMode(true);
		scene.root.commands.run(Command::Darker);
		CHECK(!scene.brush.erases());
	}
}
