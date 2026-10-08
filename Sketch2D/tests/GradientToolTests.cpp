#include "TestSketchesRoot.hpp"
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
	class GradientViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class GradientIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	struct GradientScene {
		Document document;
		Brush brush;
		GradientViewport viewport;
		GradientIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		GradientScene() {
			root.canvas.settings().viewStiffness = 1;
			brush.apply(*findBrushPreset(BrushType::Round));
		}

		void frame() {
			document.setInfinite(false);
			document.setFrame(0, 0, 200, 100);
		}

		Vec2F screenOf(Vec2F world) { return root.canvas.windowRect().pos + document.worldToScreen(world); }
		RGBA at(int4 x, int4 y) { return document.sampleColor({ (halnf) x + 0.5f, (halnf) y + 0.5f }, false); }
	};

	void checkRgba(const RGBA& expected, const RGBA& actual, halnf tolerance = 0.02f) {
		CHECK_CLOSE(expected.r, actual.r, tolerance);
		CHECK_CLOSE(expected.g, actual.g, tolerance);
		CHECK_CLOSE(expected.b, actual.b, tolerance);
		CHECK_CLOSE(expected.a, actual.a, tolerance);
	}
}

SUITE(GradientTool) {
	TEST(PositionAlongLinearAndRadialShapes) {
		CHECK_CLOSE(0.0f, Document::gradientPosition({ 0, 50 }, { 0, 0 }, { 100, 0 }, GradientShape::Linear), 1e-6f);
		CHECK_CLOSE(0.5f, Document::gradientPosition({ 50, 80 }, { 0, 0 }, { 100, 0 }, GradientShape::Linear), 1e-6f);
		CHECK_CLOSE(1.0f, Document::gradientPosition({ 300, 0 }, { 0, 0 }, { 100, 0 }, GradientShape::Linear), 1e-6f);
		CHECK_CLOSE(0.0f, Document::gradientPosition({ -50, 0 }, { 0, 0 }, { 100, 0 }, GradientShape::Linear), 1e-6f);
		CHECK_CLOSE(0.5f, Document::gradientPosition({ 0, 50 }, { 0, 0 }, { 100, 0 }, GradientShape::Radial), 1e-6f);
		CHECK_CLOSE(1.0f, Document::gradientPosition({ 0, 150 }, { 0, 0 }, { 100, 0 }, GradientShape::Radial), 1e-6f);
	}

	TEST(NeedsAFramedCanvas) {
		GradientScene scene;
		CHECK(
			scene.document.drawGradient({ 0, 0 }, { 100, 0 }, { 1, 0, 0, 1 }, { 0, 0, 1, 1 }, GradientSettings()) ==
			GradientResult::NoFrame
		);
	}

	TEST(LinearFillsTheFrameFromPrimaryToSecondary) {
		GradientScene scene;
		scene.frame();
		CHECK(
			scene.document.drawGradient({ 0, 0 }, { 200, 0 }, { 1, 0, 0, 1 }, { 0, 0, 1, 1 }, GradientSettings()) ==
			GradientResult::Drawn
		);
		checkRgba({ 1, 0, 0, 1 }, scene.at(0, 50));
		checkRgba({ 0.5f, 0, 0.5f, 1 }, scene.at(100, 10));
		checkRgba({ 0, 0, 1, 1 }, scene.at(199, 90));
		CHECK_CLOSE(0.0f, scene.at(220, 50).a, 1e-6f);
		CHECK_CLOSE(0.0f, scene.at(50, 110).a, 1e-6f);
	}

	TEST(RadialGrowsFromTheStart) {
		GradientScene scene;
		scene.frame();
		GradientSettings settings;
		settings.shape = GradientShape::Radial;
		scene.document.drawGradient({ 100, 50 }, { 150, 50 }, { 1, 1, 1, 1 }, { 0, 0, 0, 1 }, settings);
		checkRgba({ 1, 1, 1, 1 }, scene.at(100, 50));
		checkRgba({ 0.5f, 0.5f, 0.5f, 1 }, scene.at(100, 25), 0.03f);
		checkRgba({ 0.5f, 0.5f, 0.5f, 1 }, scene.at(75, 50), 0.03f);
		checkRgba({ 0, 0, 0, 1 }, scene.at(0, 0));
	}

	TEST(ColorToTransparentFadesOut) {
		GradientScene scene;
		scene.frame();
		GradientSettings settings;
		settings.toTransparent = true;
		scene.document.drawGradient({ 0, 0 }, { 200, 0 }, { 0, 1, 0, 1 }, { 0, 0, 1, 1 }, settings);
		checkRgba({ 0, 1, 0, 1 }, scene.at(0, 50));
		const RGBA middle = scene.at(100, 50);
		CHECK_CLOSE(0.5f, middle.a, 0.02f);
		CHECK_CLOSE(1.0f, middle.g, 0.02f);
		CHECK_CLOSE(0.0f, middle.b, 0.02f);
		CHECK(scene.at(199, 50).a < 0.02f);
	}

	TEST(EraserGradientFadesExistingPixels) {
		GradientScene scene;
		scene.frame();
		scene.document.drawGradient({ 0, 0 }, { 1, 0 }, { 1, 0, 0, 1 }, { 1, 0, 0, 1 }, GradientSettings());
		GradientSettings eraser;
		eraser.eraser = true;
		eraser.toTransparent = true;
		scene.document.drawGradient({ 0, 0 }, { 200, 0 }, { 0, 0, 0, 1 }, { 0, 0, 0, 1 }, eraser);
		CHECK(scene.at(0, 50).a < 0.02f);
		CHECK_CLOSE(0.5f, scene.at(100, 50).a, 0.03f);
		CHECK_CLOSE(1.0f, scene.at(100, 50).r, 0.03f);
		CHECK(scene.at(199, 50).a > 0.98f);
	}

	TEST(DrawsOverExistingPaintWithOpacity) {
		GradientScene scene;
		scene.frame();
		scene.document.drawGradient({ 0, 0 }, { 1, 0 }, { 1, 0, 0, 1 }, { 1, 0, 0, 1 }, GradientSettings());
		scene.document.drawGradient({ 0, 0 }, { 1, 0 }, { 0, 0, 1, 0.5f }, { 0, 0, 1, 0.5f }, GradientSettings());
		checkRgba({ 0.5f, 0, 0.5f, 1 }, scene.at(100, 50));
	}

	TEST(ToolDragsFromPrimaryToSecondaryAndUndoes) {
		GradientScene scene;
		scene.frame();
		scene.brush.color = { 1, 0, 0, 1 };
		scene.root.colors.setSecondary(hueColorOf(RGBA(0, 0, 1, 1)));
		scene.root.commands.run(Command::ToolGradient);
		CHECK(scene.root.canvas.tool() == CanvasTool::Gradient);
		CHECK(scene.root.toolStrip.button("gradient").isSelected());
		const size_t revision = scene.document.history().revision();
		scene.driver.moveTo(scene.screenOf({ 0, 50 }));
		scene.driver.press();
		scene.driver.moveTo(scene.screenOf({ 100, 50 }));
		CHECK(scene.root.canvas.isDrawingGradient());
		scene.driver.moveTo(scene.screenOf({ 200, 50 }));
		scene.driver.release();
		CHECK(scene.document.history().revision() > revision);
		checkRgba({ 1, 0, 0, 1 }, scene.at(1, 50), 0.05f);
		checkRgba({ 0, 0, 1, 1 }, scene.at(198, 50), 0.05f);
		scene.root.commands.run(Command::Undo);
		CHECK_CLOSE(0.0f, scene.at(100, 50).a, 1e-6f);
	}

	TEST(AClickWithoutDraggingDrawsNothing) {
		GradientScene scene;
		scene.frame();
		scene.root.commands.run(Command::ToolGradient);
		const size_t revision = scene.document.history().revision();
		scene.driver.click(scene.screenOf({ 50, 50 }));
		CHECK_EQUAL(revision, scene.document.history().revision());
	}

	TEST(InfiniteCanvasShowsTheReferenceMessage) {
		GradientScene scene;
		scene.root.commands.run(Command::ToolGradient);
		scene.driver.drag(scene.screenOf({ 0, 50 }), scene.screenOf({ 100, 50 }));
		CHECK_EQUAL(std::string("You must have a framed canvas or a selection to use gradient"), scene.root.lastNotice());
	}

	TEST(PreviewLineFollowsTheDrag) {
		GradientScene scene;
		scene.frame();
		scene.root.commands.run(Command::ToolGradient);
		scene.root.canvas.settings().gradient.shape = GradientShape::Radial;
		scene.driver.moveTo(scene.screenOf({ 50, 50 }));
		scene.driver.press();
		scene.driver.moveTo(scene.screenOf({ 120, 50 }));
		size_t lines = 0;
		size_t circles = 0;
		for (const PaintCommand& command : scene.driver.paint().commands()) {
			if (command.kind == PaintCommand::Kind::Line) lines++;
			if (command.kind == PaintCommand::Kind::StrokeCircle) circles++;
		}
		CHECK(lines >= 2);
		CHECK(circles >= 2);
		scene.driver.release();
	}

	TEST(MenuAndSessionKeepTheOptions) {
		GradientScene scene;
		GradientSettings& settings = scene.root.canvas.settings().gradient;
		const auto entry = [&scene](const std::string& text) -> MenuItem* {
			for (const auto& item : scene.root.gradientMenu.items())
				if (item->entry().text == text) return item.get();
			return nullptr;
		};
		for (const char* text : { "Gradient tool", "Linear", "Radial", "Color to transparent", "Eraser" })
			REQUIRE CHECK(entry(text) != nullptr);
		CHECK(entry("Linear")->entry().isChecked());
		entry("Radial")->entry().action();
		entry("Color to transparent")->entry().action();
		entry("Eraser")->entry().action();
		CHECK(settings.shape == GradientShape::Radial);
		CHECK(settings.toTransparent);
		CHECK(settings.eraser);
		const auto restored = SessionSnapshot::parse(
			SessionSnapshot::capture(scene.document, scene.brush, scene.root.canvas.settings(), {}).serialize()
		);
		REQUIRE CHECK(restored.has_value());
		CHECK(restored->settings.gradient.shape == GradientShape::Radial);
		CHECK(restored->settings.gradient.toTransparent);
		CHECK(restored->settings.gradient.eraser);
	}

	TEST(TabletToolsMenuHasTheGradient) {
		Document document;
		Brush brush;
		GradientViewport viewport;
		GradientIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };
		UiDriver driver{ studio, { 1280, 800 } };
		studio.useTool("gradient");
		CHECK(classic.canvas.tool() == CanvasTool::Gradient);
	}
}
