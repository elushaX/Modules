#include "TestSketchesRoot.hpp"
#include "ColorModel.hpp"
#include "FakeWindowHost.hpp"
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
	class PanelViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class PanelIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	struct PanelScene {
		Document document;
		Brush brush;
		PanelViewport viewport;
		PanelIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		FakeWindowHost host;
		UiDriver driver{ root, { 1280, 800 } };

		PanelScene() {
			root.canvas.settings().viewStiffness = 1;
			brush.color = HueColor{ 120, 0.5f, 0.8f }.rgba();
			root.refreshFromState();
			driver.layout();
		}

		ColorPopup& popup() { return root.toolBar.colorPopup; }

		void open() {
			driver.click(root.toolBar.color);
			CHECK(driver.ui().isPopupOpen(popup()));
		}

		void page(ColorPage page) {
			popup().pages.revealTab((size_t) page);
			driver.settle();
			const RectF bar = popup().pages.tabScroller().windowRect();
			const RectF tab = popup().pages.tabButton((size_t) page).windowRect();
			CHECK(tab.pos.x >= bar.pos.x - 0.5f && tab.pos.x + tab.size.x <= bar.pos.x + bar.size.x + 0.5f);
			driver.click(tab.center());
			driver.layout();
			CHECK(popup().page() == page);
		}
	};

	void checkHue(const HueColor& expected, const HueColor& actual, halnf tolerance = 0.01f) {
		CHECK_CLOSE(expected.hueDegrees, actual.hueDegrees, 360.0f * tolerance);
		CHECK_CLOSE(expected.saturation, actual.saturation, tolerance);
		CHECK_CLOSE(expected.value, actual.value, tolerance);
	}

	Vec2F sliderPoint(LabeledSlider& slider, halnf value) { return slider.slider().windowPointForValue(value); }
}

SUITE(HslModel) {
	TEST(HslRoundTripsThroughHsv) {
		for (const HueColor& color : { HueColor{ 30, 0.8f, 0.6f }, HueColor{ 200, 0.3f, 0.9f }, HueColor{ 0, 0, 0.5f } }) {
			const HueColor back = hueColorOf(hslOf(color));
			checkHue(color, back, 1e-3f);
		}
		const HslColor red = hslOf(HueColor{ 0, 1, 1 });
		CHECK_CLOSE(1.0f, red.saturation, 1e-4f);
		CHECK_CLOSE(0.5f, red.lightness, 1e-4f);
		CHECK_CLOSE(1.0f, hslOf(HueColor{ 0, 0, 1 }).lightness, 1e-4f);
	}
}

SUITE(ColorPanel) {
	TEST(PopupHasTheReferencePages) {
		PanelScene scene;
		scene.open();
		const char* titles[] = { "Wheel", "Square", "RGB", "HSV", "HSL" };
		REQUIRE CHECK_EQUAL((size_t) ColorPage::Count, scene.popup().pages.tabCount());
		for (size_t index = 0; index < (size_t) ColorPage::Count; index++)
			CHECK_EQUAL(std::string(titles[index]), scene.popup().pages.tabButton(index).text());
		CHECK(scene.popup().page() == ColorPage::Wheel);
		CHECK(scene.popup().wheel.isVisible());
		CHECK(!scene.popup().square.isVisible());
	}

	TEST(PagesShowTheBrushColor) {
		PanelScene scene;
		scene.open();
		ColorPopup& popup = scene.popup();
		const RGBA rgb = HueColor{ 120, 0.5f, 0.8f }.rgba();
		CHECK_CLOSE(std::round(rgb.r * 255), popup.red.value(), 0.5f);
		CHECK_CLOSE(std::round(rgb.g * 255), popup.green.value(), 0.5f);
		CHECK_CLOSE(120.0f, popup.hsvHue.value(), 0.5f);
		CHECK_CLOSE(50.0f, popup.hsvSaturation.value(), 0.5f);
		CHECK_CLOSE(80.0f, popup.hsvValue.value(), 0.5f);
		CHECK_CLOSE(hslOf(HueColor{ 120, 0.5f, 0.8f }).lightness * 100, popup.hslLightness.value(), 0.5f);
		CHECK_EQUAL(colorHex(rgb), popup.hex.text());
	}

	TEST(SquareSetsSaturationAndValue) {
		PanelScene scene;
		scene.open();
		scene.page(ColorPage::Square);
		ColorSquare& square = scene.popup().square;
		REQUIRE CHECK(square.isVisible());
		scene.driver.click(square.windowPointFor(1.0f, 0.25f));
		checkHue({ 120, 1.0f, 0.25f }, hueColorOf(scene.brush.color), 0.03f);
		scene.driver.click(square.windowPointForHue(3.14159265f));
		CHECK_CLOSE(180.0f, scene.root.colors.primary().hueDegrees, 4.0f);
		CHECK_CLOSE(0.25f, scene.root.colors.primary().value, 0.03f);
	}

	TEST(RgbSlidersEditTheColor) {
		PanelScene scene;
		scene.open();
		scene.page(ColorPage::Rgb);
		ColorPopup& popup = scene.popup();
		REQUIRE CHECK(popup.rgbPage.isVisible());
		scene.driver.click(sliderPoint(popup.red, 255));
		CHECK_CLOSE(1.0f, scene.brush.color.r, 0.02f);
		CHECK(!popup.red.slider().trackGradient().empty());
		CHECK_CLOSE(scene.brush.color.g, popup.red.slider().trackGradient().front().g, 1e-3f);
		bool gradientPainted = false;
		for (const PaintCommand& command : scene.driver.paint().commands())
			if (command.kind == PaintCommand::Kind::Gradient) gradientPainted = true;
		CHECK(gradientPainted);
	}

	TEST(HsvSlidersEditTheColor) {
		PanelScene scene;
		scene.open();
		scene.page(ColorPage::Hsv);
		ColorPopup& popup = scene.popup();
		scene.driver.click(sliderPoint(popup.hsvHue, 240));
		CHECK_CLOSE(240.0f, scene.root.colors.primary().hueDegrees, 3.0f);
		scene.driver.click(sliderPoint(popup.hsvValue, 100));
		CHECK_CLOSE(1.0f, scene.root.colors.primary().value, 0.02f);
		CHECK_EQUAL((size_t) 7, popup.hsvHue.slider().trackGradient().size());
	}

	TEST(HslSlidersEditTheColor) {
		PanelScene scene;
		scene.open();
		scene.page(ColorPage::Hsl);
		ColorPopup& popup = scene.popup();
		scene.driver.click(sliderPoint(popup.hslLightness, 100));
		CHECK_CLOSE(1.0f, scene.brush.color.r, 0.02f);
		CHECK_CLOSE(1.0f, scene.brush.color.g, 0.02f);
		CHECK_CLOSE(1.0f, scene.brush.color.b, 0.02f);
		CHECK_CLOSE(120.0f, scene.root.colors.primary().hueDegrees, 1.0f);
		CHECK_EQUAL((size_t) 3, popup.hslLightness.slider().trackGradient().size());
	}

	TEST(HexFieldAcceptsAColor) {
		PanelScene scene;
		scene.open();
		ColorPopup& popup = scene.popup();
		scene.driver.click(popup.hex);
		popup.hex.selectAll();
		scene.driver.type("#FF0000");
		scene.driver.key(InputID::ENTER);
		CHECK_CLOSE(1.0f, scene.brush.color.r, 1e-3f);
		CHECK_CLOSE(0.0f, scene.brush.color.g, 1e-3f);
		checkHue({ 0, 1, 1 }, scene.root.colors.primary());
	}

	TEST(InvalidHexRestoresTheShownColor) {
		PanelScene scene;
		scene.open();
		ColorPopup& popup = scene.popup();
		const std::string before = popup.hex.text();
		scene.driver.click(popup.hex);
		popup.hex.selectAll();
		scene.driver.type("nope");
		scene.driver.key(InputID::ENTER);
		CHECK_EQUAL(before, popup.hex.text());
	}

	TEST(CompareShowsOldAndNewAndRestoresOld) {
		PanelScene scene;
		scene.open();
		ColorPopup& popup = scene.popup();
		const RGBA old = scene.brush.color;
		scene.page(ColorPage::Hsv);
		scene.driver.click(sliderPoint(popup.hsvHue, 0));
		CHECK_CLOSE(old.g, popup.compare.previous.g, 1e-3f);
		CHECK_CLOSE(scene.brush.color.r, popup.compare.current.r, 1e-3f);
		CHECK(std::fabs(old.g - scene.brush.color.g) > 0.1f);

		scene.driver.click(popup.compare.windowRect().pos + popup.compare.previousRect().center());
		CHECK_CLOSE(old.r, scene.brush.color.r, 0.01f);
		CHECK_CLOSE(old.g, scene.brush.color.g, 0.01f);
		CHECK_CLOSE(old.b, scene.brush.color.b, 0.01f);
	}

	TEST(ReopeningMarksTheNewOldColor) {
		PanelScene scene;
		scene.open();
		scene.page(ColorPage::Hsv);
		scene.driver.click(sliderPoint(scene.popup().hsvHue, 0));
		const RGBA picked = scene.brush.color;
		scene.driver.key(InputID::ESCAPE);
		scene.open();
		CHECK_CLOSE(picked.r, scene.popup().compare.previous.r, 1e-3f);
		CHECK_CLOSE(picked.g, scene.popup().compare.previous.g, 1e-3f);
	}

	TEST(CopyButtonCopiesHex) {
		PanelScene scene;
		scene.driver.ui().setWindowHost(&scene.host);
		scene.open();
		scene.driver.click(scene.popup().copy);
		CHECK_EQUAL(colorHex(scene.brush.color), scene.host.clipboard);
		scene.driver.ui().setWindowHost(nullptr);
	}

	TEST(ColorCommandsUpdateTheOpenPanel) {
		PanelScene scene;
		scene.open();
		scene.root.commands.run(Command::Brighter);
		CHECK_CLOSE(85.0f, scene.popup().hsvValue.value(), 0.5f);
		CHECK_EQUAL(colorHex(scene.brush.color), scene.popup().hex.text());
		scene.root.commands.run(Command::DefaultColors);
		CHECK_CLOSE(0.0f, scene.popup().hsvValue.value(), 0.5f);
		CHECK_CLOSE(50.0f, scene.popup().hsvSaturation.value(), 0.5f);
		CHECK_CLOSE(120.0f, scene.popup().hsvHue.value(), 0.5f);
	}

	TEST(WheelKeepsHueOfGrays) {
		PanelScene scene;
		scene.open();
		scene.page(ColorPage::Hsv);
		scene.driver.click(sliderPoint(scene.popup().hsvSaturation, 0));
		CHECK_CLOSE(120.0f, scene.root.colors.primary().hueDegrees, 1.0f);
		CHECK_CLOSE(120.0f, scene.popup().hsvHue.value(), 1.0f);
	}
}

SUITE(TabletColor) {
	struct Tablet {
		Document document;
		Brush brush;
		PanelViewport viewport;
		PanelIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };
		UiDriver driver{ studio, { 1280, 800 } };

		Tablet() {
			classic.canvas.settings().viewStiffness = 1;
			brush.color = HueColor{ 120, 0.5f, 0.8f }.rgba();
			driver.layout();
		}

		const MenuItem* quick(Menu& menu, const std::string& text) {
			for (const auto& item : menu.items())
				if (item->entry().text == text) return item.get();
			return nullptr;
		}
	};

	TEST(ColorButtonOpensThePagedPicker) {
		Tablet tablet;
		tablet.driver.click(tablet.studio.color);
		ColorPopup& popup = tablet.classic.toolBar.colorPopup;
		REQUIRE CHECK(tablet.driver.ui().isPopupOpen(popup));
		popup.pages.revealTab((size_t) ColorPage::Hsv);
		tablet.driver.settle();
		tablet.driver.click(popup.pages.tabButton((size_t) ColorPage::Hsv));
		CHECK(popup.page() == ColorPage::Hsv);
		tablet.driver.layout();
		tablet.driver.click(popup.hsvValue.slider().windowPointForValue(100));
		CHECK_CLOSE(1.0f, tablet.classic.colors.primary().value, 0.02f);
	}

	TEST(SettingsReachTheColorMenu) {
		Tablet tablet;
		bool found = false;
		for (const auto& entry : tablet.classic.mainMenu.items())
			if (entry->entry().text == "Color" && entry->entry().submenu == &tablet.classic.colorMenu) found = true;
		CHECK(found);
	}
}
