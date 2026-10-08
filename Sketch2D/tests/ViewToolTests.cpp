#include "TestSketchesRoot.hpp"
#include "Sketch2DUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cmath>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class ToolViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class ToolIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	struct ToolScene {
		Document document;
		Brush brush;
		ToolViewport viewport;
		ToolIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		ToolScene() {
			root.canvas.settings().viewStiffness = 1;
			brush.apply(*findBrushPreset(BrushType::Round));
		}

		Vec2F at(Vec2F local) { return root.canvas.windowRect().pos + local; }
		Vec2F center() { return at(root.canvas.size() / 2.0f); }
	};
}

SUITE(ViewTools) {
	TEST(EveryViewToolHasAStripButtonAndShortcut) {
		ToolScene scene;
		const std::pair<Command, CanvasTool> tools[] = {
			{ Command::ToolHand, CanvasTool::Hand },     { Command::ToolZoom, CanvasTool::Zoom },
			{ Command::ToolRotate, CanvasTool::Rotate }, { Command::ToolPicker, CanvasTool::Picker },
			{ Command::ToolPaint, CanvasTool::Paint },
		};
		for (const auto& [command, tool] : tools) {
			scene.root.commands.run(command);
			CHECK(scene.root.canvas.tool() == tool);
		}
		scene.root.toolStrip.reveal("rotate");
		scene.driver.layout();
		const RectF strip = scene.root.toolStrip.windowRect();
		const RectF rotate = scene.root.toolStrip.button("rotate").windowRect();
		CHECK(rotate.pos.y >= strip.pos.y - 0.5f);
		CHECK(rotate.pos.y + rotate.size.y <= strip.pos.y + strip.size.y + 0.5f);
		scene.driver.click(scene.root.toolStrip.button("rotate"));
		CHECK(scene.root.canvas.tool() == CanvasTool::Rotate);
		CHECK(scene.root.toolStrip.button("rotate").isSelected());
		CHECK(!scene.root.toolStrip.button("brush").isSelected());
		CHECK(!scene.root.commands.shortcutText(Command::ToolHand).empty());
		CHECK(!scene.root.commands.shortcutText(Command::ToolRotate).empty());
	}

	TEST(HandDragsTheViewWithoutPainting) {
		ToolScene scene;
		scene.root.commands.run(Command::ToolHand);
		const size_t revision = scene.document.history().revision();
		const Vec2F worldBefore = scene.document.screenToWorld({ 400, 400 });
		scene.driver.drag(scene.at({ 400, 400 }), scene.at({ 460, 430 }));
		CHECK_EQUAL(revision, scene.document.history().revision());
		const Vec2F worldAfter = scene.document.screenToWorld({ 460, 430 });
		CHECK_CLOSE(worldBefore.x, worldAfter.x, 0.01f);
		CHECK_CLOSE(worldBefore.y, worldAfter.y, 0.01f);
	}

	TEST(ZoomClickZoomsInAndAltClickZoomsOutAroundThePointer) {
		ToolScene scene;
		scene.root.commands.run(Command::ToolZoom);
		const Vec2F local = { 300, 250 };
		const Vec2F world = scene.document.screenToWorld(local);
		const halnf before = scene.document.zoom();
		scene.driver.click(scene.at(local));
		CHECK_CLOSE(before * CanvasWidget::kZoomClickFactor, scene.document.zoom(), 1e-4f);
		const Vec2F still = scene.document.worldToScreen(world);
		CHECK_CLOSE(local.x, still.x, 0.01f);
		CHECK_CLOSE(local.y, still.y, 0.01f);
		scene.driver.click(scene.at(local), Modifiers{ false, false, true });
		CHECK_CLOSE(before, scene.document.zoom(), 1e-4f);
	}

	TEST(ZoomDragScrubsTheZoom) {
		ToolScene scene;
		scene.root.commands.run(Command::ToolZoom);
		const halnf before = scene.document.zoom();
		scene.driver.drag(scene.at({ 400, 400 }), scene.at(Vec2F{ 400.0f + CanvasWidget::kZoomDoublingPixels, 400.0f }));
		CHECK_CLOSE(before * 2, scene.document.zoom(), 0.02f);
		scene.driver.drag(
			scene.at({ 400, 400 }), scene.at(Vec2F{ 400.0f - CanvasWidget::kZoomDoublingPixels * 2, 400.0f })
		);
		CHECK_CLOSE(before / 2, scene.document.zoom(), 0.02f);
	}

	TEST(RotateDragTurnsTheViewAroundTheCenterAndSnaps) {
		ToolScene scene;
		scene.root.commands.run(Command::ToolRotate);
		const Vec2F center = scene.center();
		const Vec2F worldCenter = scene.document.screenToWorld(scene.root.canvas.size() / 2.0f);
		scene.driver.drag(center + Vec2F{ 200, 0 }, center + Vec2F{ 0, 200 }, 12);
		CHECK_CLOSE(3.14159265f / 2, scene.document.rotation(), 0.02f);
		const Vec2F stays = scene.document.worldToScreen(worldCenter);
		CHECK_CLOSE(scene.root.canvas.size().x / 2, stays.x, 0.5f);
		CHECK_CLOSE(scene.root.canvas.size().y / 2, stays.y, 0.5f);

		scene.driver.drag(center + Vec2F{ 0, 200 }, center + Vec2F{ 200, 6 }, 12);
		CHECK_CLOSE(0.0f, scene.document.rotation(), 1e-6f);
	}

	TEST(ViewToolsDoNotPickOrPaint) {
		ToolScene scene;
		scene.brush.color = { 1, 0, 0, 1 };
		const size_t revision = scene.document.history().revision();
		for (const Command command : { Command::ToolHand, Command::ToolZoom, Command::ToolRotate }) {
			scene.root.commands.run(command);
			scene.driver.pressKey(InputID::LEFT_ALT);
			scene.driver.drag(scene.at({ 500, 300 }), scene.at({ 560, 340 }));
			scene.driver.releaseKey(InputID::LEFT_ALT);
		}
		CHECK_EQUAL(revision, scene.document.history().revision());
		CHECK_CLOSE(1.0f, scene.brush.color.r, 1e-6f);
	}

	TEST(PaintToolsLeaveViewTools) {
		ToolScene scene;
		scene.root.commands.run(Command::ToolHand);
		scene.root.commands.run(Command::ToolEraser);
		CHECK(scene.root.canvas.tool() == CanvasTool::Paint);
		CHECK(scene.root.toolStrip.button("eraser").isSelected());
		CHECK(!scene.root.toolStrip.button("hand").isSelected());
		const size_t revision = scene.document.history().revision();
		scene.root.commands.run(Command::ToolPaint);
		scene.driver.drag(scene.at({ 400, 400 }), scene.at({ 460, 400 }));
		CHECK(scene.document.history().revision() > revision);
	}
}
