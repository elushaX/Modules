#include "TestSketchesRoot.hpp"
#include "ColorModel.hpp"
#include "Session.hpp"
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
	class PickerViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class PickerIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	struct PickerScene {
		Document document;
		Brush brush;
		PickerViewport viewport;
		PickerIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		PickerScene() {
			root.canvas.settings().viewStiffness = 1;
			brush.apply(*findBrushPreset(BrushType::Round));
			brush.setSize(40);
		}

		void paint(const RGBA& color, Vec2F from, Vec2F to) {
			brush.color = color;
			driver.drag(from, to);
			document.settlePaint();
		}

		Vec2F canvasPoint(Vec2F local) { return root.canvas.windowRect().pos + local; }
	};

	void checkRgb(const RGBA& expected, const RGBA& actual, halnf tolerance = 0.02f) {
		CHECK_CLOSE(expected.r, actual.r, tolerance);
		CHECK_CLOSE(expected.g, actual.g, tolerance);
		CHECK_CLOSE(expected.b, actual.b, tolerance);
	}
}

SUITE(PickerTool) {
	TEST(DocumentSamplesTheActiveLayerOrAllVisibleLayers) {
		PickerScene scene;
		scene.paint({ 1, 0, 0, 1 }, scene.canvasPoint({ 300, 300 }), scene.canvasPoint({ 400, 300 }));
		scene.document.addLayer("Top");
		const Vec2F world = scene.document.screenToWorld({ 350, 300 });
		CHECK_CLOSE(0.0f, scene.document.sampleColor(world, false).a, 1e-6f);
		const RGBA all = scene.document.sampleColor(world, true);
		CHECK_CLOSE(1.0f, all.a, 0.02f);
		checkRgb({ 1, 0, 0, 1 }, all);
		scene.document.setLayerVisible(*scene.document.layers().front(), false);
		CHECK_CLOSE(0.0f, scene.document.sampleColor(world, true).a, 1e-6f);
	}

	TEST(EyedropperToolPicksTheCanvasColor) {
		PickerScene scene;
		scene.paint({ 0, 0.5f, 1, 1 }, scene.canvasPoint({ 300, 300 }), scene.canvasPoint({ 400, 300 }));
		scene.brush.color = { 1, 1, 0, 1 };
		const size_t revision = scene.document.history().revision();

		scene.driver.click(scene.root.toolStrip.button("eyedropper"));
		CHECK(scene.root.canvas.pickerTool());
		CHECK(scene.root.toolStrip.button("eyedropper").isSelected());
		CHECK(!scene.root.toolStrip.button("brush").isSelected());

		scene.driver.click(scene.canvasPoint({ 350, 300 }));
		checkRgb({ 0, 0.5f, 1, 1 }, scene.brush.color);
		checkRgb({ 0, 0.5f, 1, 1 }, scene.root.colors.primary().rgba());
		CHECK_EQUAL(revision, scene.document.history().revision());
	}

	TEST(DraggingKeepsPickingAndEmptyCanvasKeepsTheLastColor) {
		PickerScene scene;
		scene.paint({ 1, 0, 0, 1 }, scene.canvasPoint({ 200, 300 }), scene.canvasPoint({ 260, 300 }));
		scene.paint({ 0, 1, 0, 1 }, scene.canvasPoint({ 500, 300 }), scene.canvasPoint({ 560, 300 }));
		scene.root.canvas.setPickerTool(true);
		scene.driver.moveTo(scene.canvasPoint({ 230, 300 }));
		scene.driver.press();
		checkRgb({ 1, 0, 0, 1 }, scene.brush.color);
		CHECK(scene.root.canvas.isPicking());
		scene.driver.moveTo(scene.canvasPoint({ 530, 300 }));
		checkRgb({ 0, 1, 0, 1 }, scene.brush.color);
		scene.driver.moveTo(scene.canvasPoint({ 800, 600 }));
		checkRgb({ 0, 1, 0, 1 }, scene.brush.color);
		scene.driver.release();
		CHECK(!scene.root.canvas.isPicking());
	}

	TEST(SampleAllLayersCanBeTurnedOff) {
		PickerScene scene;
		scene.paint({ 1, 0, 0, 1 }, scene.canvasPoint({ 300, 300 }), scene.canvasPoint({ 400, 300 }));
		scene.document.addLayer("Top");
		scene.brush.color = { 0, 0, 1, 1 };
		scene.root.canvas.settings().picker.sampleAllLayers = false;
		scene.root.canvas.setPickerTool(true);
		scene.driver.click(scene.canvasPoint({ 350, 300 }));
		checkRgb({ 0, 0, 1, 1 }, scene.brush.color);

		scene.root.canvas.settings().picker.sampleAllLayers = true;
		scene.driver.click(scene.canvasPoint({ 350, 300 }));
		checkRgb({ 1, 0, 0, 1 }, scene.brush.color);
	}

	TEST(SampleOpacitySetsTheBrushOpacity) {
		PickerScene scene;
		scene.brush.setOpacity(0.5f);
		scene.brush.color = { 1, 0, 0, 1 };
		scene.driver.click(scene.canvasPoint({ 350, 300 }));
		scene.document.settlePaint();
		const halnf painted = scene.document.sampleColor(scene.document.screenToWorld({ 350, 300 }), true).a;
		REQUIRE CHECK(painted > 0.1f && painted < 0.9f);
		scene.brush.setOpacity(1);
		scene.root.canvas.setPickerTool(true);
		scene.driver.click(scene.canvasPoint({ 350, 300 }));
		CHECK_CLOSE(1.0f, scene.brush.opacity(), 1e-6f);
		checkRgb({ 1, 0, 0, 1 }, scene.brush.color, 0.03f);

		scene.root.canvas.settings().picker.sampleOpacity = true;
		scene.driver.click(scene.canvasPoint({ 350, 300 }));
		CHECK_CLOSE(painted, scene.brush.opacity(), 0.01f);
	}

	TEST(AltPicksTemporarily) {
		PickerScene scene;
		scene.paint({ 0, 1, 0, 1 }, scene.canvasPoint({ 300, 300 }), scene.canvasPoint({ 400, 300 }));
		scene.brush.color = { 1, 0, 0, 1 };
		const size_t revision = scene.document.history().revision();
		scene.driver.pressKey(InputID::LEFT_ALT);
		scene.driver.click(scene.canvasPoint({ 350, 300 }));
		scene.driver.releaseKey(InputID::LEFT_ALT);
		checkRgb({ 0, 1, 0, 1 }, scene.brush.color);
		CHECK_EQUAL(revision, scene.document.history().revision());
		CHECK(!scene.root.canvas.pickerTool());
		scene.driver.drag(scene.canvasPoint({ 600, 500 }), scene.canvasPoint({ 650, 500 }));
		CHECK(scene.document.history().revision() > revision);
	}

	TEST(ShortcutAndOtherToolsSwitchThePicker) {
		PickerScene scene;
		scene.root.commands.run(Command::ToolPicker);
		CHECK(scene.root.canvas.pickerTool());
		scene.driver.click(scene.root.toolStrip.button("eraser"));
		CHECK(!scene.root.canvas.pickerTool());
		CHECK(scene.root.toolStrip.button("eraser").isSelected());
		scene.root.commands.run(Command::ToolPicker);
		scene.driver.click(scene.root.toolStrip.button("brush"));
		CHECK(!scene.root.canvas.pickerTool());
		CHECK(scene.root.toolStrip.button("brush").isSelected());
	}

	TEST(PickingFromAnEraserKeepsPaintingWithTheColor) {
		PickerScene scene;
		scene.paint({ 0, 0, 1, 1 }, scene.canvasPoint({ 300, 300 }), scene.canvasPoint({ 400, 300 }));
		scene.brush.setEraserMode(true);
		scene.root.canvas.setPickerTool(true);
		scene.driver.click(scene.canvasPoint({ 350, 300 }));
		CHECK(!scene.brush.erases());
	}

	TEST(EachStyleDrawsItsPreview) {
		for (const PickerStyle style : { PickerStyle::Ring, PickerStyle::Balloon, PickerStyle::Magnifier }) {
			PickerScene scene;
			scene.paint({ 1, 0, 0, 1 }, scene.canvasPoint({ 300, 300 }), scene.canvasPoint({ 400, 300 }));
			scene.brush.color = { 0, 0, 1, 1 };
			scene.root.canvas.settings().picker.style = style;
			scene.root.canvas.setPickerTool(true);
			scene.driver.moveTo(scene.canvasPoint({ 350, 300 }));
			scene.driver.press();
			bool sawNew = false;
			bool sawOld = false;
			for (const PaintCommand& command : scene.driver.paint().commands()) {
				const RGBA& color = command.color;
				if (color.r > 0.95f && color.g < 0.05f && color.b < 0.05f && color.a > 0.95f) sawNew = true;
				if (color.b > 0.95f && color.r < 0.05f && color.g < 0.05f && color.a > 0.95f) sawOld = true;
			}
			CHECK(sawNew);
			if (style != PickerStyle::Magnifier) CHECK(sawOld);
			scene.driver.release();
		}
	}

	TEST(CursorStyleDrawsNoPreview) {
		PickerScene scene;
		scene.paint({ 1, 0, 0, 1 }, scene.canvasPoint({ 300, 300 }), scene.canvasPoint({ 400, 300 }));
		scene.root.canvas.settings().picker.style = PickerStyle::Cursor;
		scene.root.canvas.setPickerTool(true);
		scene.driver.moveTo(scene.canvasPoint({ 350, 300 }));
		scene.driver.press();
		size_t previews = 0;
		for (const PaintCommand& command : scene.driver.paint().commands())
			if (command.color.r > 0.95f && command.color.g < 0.05f && command.color.a > 0.95f) previews++;
		CHECK_EQUAL((size_t) 0, previews);
		scene.driver.release();
	}

	TEST(PickerMenuTogglesTheReferenceOptions) {
		PickerScene scene;
		PickerSettings& settings = scene.root.canvas.settings().picker;
		CHECK(settings.sampleAllLayers);
		CHECK(!settings.sampleOpacity);
		CHECK(settings.style == PickerStyle::Ring);
		const auto entry = [&scene](const std::string& text) -> MenuItem* {
			for (const auto& item : scene.root.pickerMenu.items())
				if (item->entry().text == text) return item.get();
			return nullptr;
		};
		for (const char* text :
				 { "Color picker tool", "Sample all layers", "Sample opacity", "Cursor", "Ring", "Balloon", "Magnifier" })
			CHECK(entry(text) != nullptr);
		REQUIRE CHECK(entry("Magnifier") != nullptr);
		entry("Magnifier")->entry().action();
		CHECK(settings.style == PickerStyle::Magnifier);
		CHECK(entry("Magnifier")->entry().isChecked());
		CHECK(!entry("Ring")->entry().isChecked());
		entry("Sample all layers")->entry().action();
		CHECK(!settings.sampleAllLayers);
		entry("Sample opacity")->entry().action();
		CHECK(settings.sampleOpacity);
		bool nested = false;
		for (const auto& item : scene.root.colorMenu.items())
			if (item->entry().text == "Color picker" && item->entry().submenu == &scene.root.pickerMenu) nested = true;
		CHECK(nested);
	}

	TEST(PickerSettingsAreSavedWithTheSession) {
		ViewSettings settings;
		settings.picker = { false, true, PickerStyle::Balloon };
		Document document;
		const std::string text = SessionSnapshot::capture(document, Brush(), settings, {}).serialize();
		const auto restored = SessionSnapshot::parse(text);
		REQUIRE CHECK(restored.has_value());
		CHECK(!restored->settings.picker.sampleAllLayers);
		CHECK(restored->settings.picker.sampleOpacity);
		CHECK(restored->settings.picker.style == PickerStyle::Balloon);
	}

	TEST(TabletPickerPicksFromTheCanvas) {
		Document document;
		Brush brush;
		PickerViewport viewport;
		PickerIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };
		UiDriver driver{ studio, { 1280, 800 } };
		classic.canvas.settings().viewStiffness = 1;
		brush.apply(*findBrushPreset(BrushType::Round));
		brush.setSize(40);
		brush.color = { 1, 0, 1, 1 };
		const Vec2F at = classic.canvas.windowRect().pos + Vec2F{ 400, 400 };
		driver.drag(at, at + Vec2F{ 80, 0 });
		document.settlePaint();
		brush.color = { 0, 1, 0, 1 };
		classic.commands.run(Command::ToolPicker);
		driver.click(at + Vec2F{ 40, 0 });
		checkRgb({ 1, 0, 1, 1 }, brush.color);
	}
}
