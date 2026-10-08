#include "TestSketchesRoot.hpp"
#include "Sketch2DUI.hpp"
#include "StudioUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cmath>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class TransformViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class TransformIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	void fillRect(Document& document, int4 left, int4 top, int4 width, int4 height, const RGBA& color) {
		Layer& layer = *document.activeLayer();
		for (int4 y = top; y < top + height; y++)
			for (int4 x = left; x < left + width; x++) {
				const TileCoord coord = { floordiv(x, kTileSize), floordiv(y, kTileSize) };
				Tile* tile = layer.tileAt(coord);
				uint1* pixel =
					tile->pixels() + ((size_t) (y - coord.y * kTileSize) * kTileSize + (size_t) (x - coord.x * kTileSize)) * 4;
				pixel[0] = (uint1) (color.r * 255);
				pixel[1] = (uint1) (color.g * 255);
				pixel[2] = (uint1) (color.b * 255);
				pixel[3] = 255;
				tile->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
			}
	}

	struct TransformScene {
		Document document;
		Brush brush;
		TransformViewport viewport;
		TransformIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		TransformScene() {
			root.canvas.settings().viewStiffness = 1;
			brush.apply(*findBrushPreset(BrushType::Round));
			fillRect(document, 100, 100, 40, 20, { 1, 0, 0, 1 });
			fillRect(document, 100, 100, 10, 20, { 0, 0, 1, 1 });
			document.commit("Shape");
		}

		Vec2F screenOf(Vec2F world) { return root.canvas.windowRect().pos + document.worldToScreen(world); }
		RGBA at(int4 x, int4 y) { return document.sampleColor({ (halnf) x + 0.5f, (halnf) y + 0.5f }, false); }
	};

	void checkRgb(const RGBA& expected, const RGBA& actual) {
		CHECK_CLOSE(expected.r, actual.r, 0.02f);
		CHECK_CLOSE(expected.g, actual.g, 0.02f);
		CHECK_CLOSE(expected.b, actual.b, 0.02f);
		CHECK_CLOSE(expected.a, actual.a, 0.02f);
	}
}

SUITE(TransformTool) {
	TEST(TransformMathRoundTrips) {
		LayerTransform transform;
		transform.pivot = Vec2F{ 10.0f, 20.0f };
		transform.scale = Vec2F{ 2.0f, -0.5f };
		transform.rotation = 0.7f;
		transform.offset = Vec2F{ 5.0f, -3.0f };
		const Vec2F point = { 33, 41 };
		const Vec2F back = transform.invert(transform.apply(point));
		CHECK_CLOSE(point.x, back.x, 1e-3f);
		CHECK_CLOSE(point.y, back.y, 1e-3f);
		CHECK(LayerTransform().isIdentity());
		CHECK(!transform.isIdentity());
	}

	TEST(FlipHorizontalMirrorsTheLayerInPlace) {
		TransformScene scene;
		scene.root.commands.run(Command::FlipHorizontal);
		checkRgb({ 0, 0, 1, 1 }, scene.at(135, 110));
		checkRgb({ 1, 0, 0, 1 }, scene.at(102, 110));
		CHECK_CLOSE(0.0f, scene.at(98, 110).a, 1e-6f);
		CHECK_CLOSE(0.0f, scene.at(141, 110).a, 1e-6f);
		scene.root.commands.run(Command::Undo);
		checkRgb({ 0, 0, 1, 1 }, scene.at(102, 110));
	}

	TEST(FlipVerticalMirrorsTopAndBottom) {
		TransformScene scene;
		fillRect(scene.document, 100, 100, 40, 4, { 0, 1, 0, 1 });
		scene.root.commands.run(Command::FlipVertical);
		checkRgb({ 0, 1, 0, 1 }, scene.at(120, 118));
		checkRgb({ 1, 0, 0, 1 }, scene.at(120, 101));
	}

	TEST(EmptyLayerCannotBeTransformed) {
		TransformScene scene;
		scene.document.addLayer("Empty");
		scene.root.commands.run(Command::FreeTransform);
		CHECK(!scene.root.canvas.isTransforming());
		CHECK_EQUAL(std::string("You can't transform an empty layer"), scene.root.lastNotice());
		scene.root.commands.run(Command::FlipHorizontal);
		CHECK_EQUAL(std::string("You can't transform an empty layer"), scene.root.lastNotice());
	}

	TEST(FreeTransformShowsTheBarAndHandles) {
		TransformScene scene;
		CHECK(!scene.root.transformBar.isVisible());
		scene.root.commands.run(Command::FreeTransform);
		scene.driver.layout();
		CHECK(scene.root.canvas.isTransforming());
		CHECK(scene.root.transformBar.isVisible());
		CHECK(scene.root.toolStrip.button("transform").isSelected());
		const auto corners = scene.root.canvas.transformCorners();
		const Vec2F topLeft = scene.document.worldToScreen({ 100, 100 });
		const Vec2F bottomRight = scene.document.worldToScreen({ 140, 120 });
		CHECK_CLOSE(topLeft.x, corners[0].x, 0.01f);
		CHECK_CLOSE(topLeft.y, corners[0].y, 0.01f);
		CHECK_CLOSE(bottomRight.x, corners[2].x, 0.01f);
		CHECK_CLOSE(bottomRight.y, corners[2].y, 0.01f);
		size_t handles = 0;
		for (const PaintCommand& command : scene.driver.paint().commands())
			if (command.kind == PaintCommand::Kind::Circle) handles++;
		CHECK(handles >= 8);
	}

	TEST(DraggingInsideMovesAndApplyCommits) {
		TransformScene scene;
		scene.root.commands.run(Command::FreeTransform);
		scene.driver.layout();
		const size_t revision = scene.document.history().revision();
		scene.driver.drag(scene.screenOf({ 120, 110 }), scene.screenOf({ 150, 140 }));
		checkRgb({ 1, 0, 0, 1 }, scene.at(160, 140));
		CHECK_CLOSE(0.0f, scene.at(120, 105).a, 1e-6f);
		CHECK_EQUAL(revision, scene.document.history().revision());
		scene.driver.click(scene.root.transformBar.apply);
		CHECK(!scene.root.canvas.isTransforming());
		CHECK(!scene.root.transformBar.isVisible());
		CHECK_EQUAL(revision + 1, scene.document.history().revision());
		scene.root.commands.run(Command::Undo);
		checkRgb({ 1, 0, 0, 1 }, scene.at(120, 110));
	}

	TEST(CancelRestoresTheLayer) {
		TransformScene scene;
		scene.root.commands.run(Command::FreeTransform);
		scene.driver.layout();
		scene.driver.drag(scene.screenOf({ 120, 110 }), scene.screenOf({ 300, 300 }));
		scene.driver.click(scene.root.transformBar.cancel);
		CHECK(!scene.root.canvas.isTransforming());
		checkRgb({ 1, 0, 0, 1 }, scene.at(120, 110));
		CHECK_CLOSE(0.0f, scene.at(320, 300).a, 1e-6f);
	}

	TEST(EnterAppliesAndEscapeCancels) {
		TransformScene scene;
		scene.root.commands.run(Command::FreeTransform);
		scene.driver.layout();
		scene.driver.drag(scene.screenOf({ 120, 110 }), scene.screenOf({ 120, 160 }));
		scene.driver.key(InputID::ESCAPE);
		CHECK(!scene.root.canvas.isTransforming());
		checkRgb({ 1, 0, 0, 1 }, scene.at(120, 110));

		scene.root.commands.run(Command::FreeTransform);
		scene.driver.layout();
		scene.driver.drag(scene.screenOf({ 120, 110 }), scene.screenOf({ 120, 160 }));
		scene.driver.key(InputID::ENTER);
		CHECK(!scene.root.canvas.isTransforming());
		checkRgb({ 1, 0, 0, 1 }, scene.at(120, 160));
	}

	TEST(CornerHandleScalesAroundTheOppositeCorner) {
		TransformScene scene;
		scene.root.commands.run(Command::FreeTransform);
		scene.driver.layout();
		scene.driver.drag(scene.screenOf({ 140, 120 }), scene.screenOf({ 180, 140 }));
		const LayerTransform& transform = scene.root.canvas.pendingTransform();
		CHECK_CLOSE(2.0f, transform.scale.x, 0.02f);
		CHECK_CLOSE(2.0f, transform.scale.y, 0.02f);
		const Vec2F anchor = transform.apply({ 100, 100 });
		CHECK_CLOSE(100.0f, anchor.x, 0.05f);
		CHECK_CLOSE(100.0f, anchor.y, 0.05f);
		scene.root.canvas.applyTransform();
		checkRgb({ 1, 0, 0, 1 }, scene.at(170, 135));
		checkRgb({ 0, 0, 1, 1 }, scene.at(110, 130));
	}

	TEST(ProportionalScalingKeepsTheAspect) {
		TransformScene scene;
		scene.root.commands.run(Command::FreeTransform);
		scene.driver.layout();
		scene.driver.click(scene.root.transformBar.proportional);
		CHECK(scene.root.canvas.settings().transform.proportional);
		CHECK(scene.root.transformBar.proportional.isSelected());
		scene.driver.drag(scene.screenOf({ 140, 120 }), scene.screenOf({ 220, 125 }));
		const LayerTransform& transform = scene.root.canvas.pendingTransform();
		CHECK_CLOSE(transform.scale.x, transform.scale.y, 1e-3f);
		CHECK(transform.scale.x > 1.2f);
	}

	TEST(ShiftTogglesProportionalWhileDragging) {
		TransformScene scene;
		scene.root.commands.run(Command::FreeTransform);
		scene.driver.layout();
		scene.driver.moveTo(scene.screenOf({ 140, 120 }));
		scene.driver.press(PointerButton::Primary, Modifiers{ true, false, false });
		scene.driver.moveTo(scene.screenOf({ 220, 125 }));
		scene.driver.release();
		const LayerTransform& transform = scene.root.canvas.pendingTransform();
		CHECK_CLOSE(transform.scale.x, transform.scale.y, 1e-3f);
	}

	TEST(DraggingOutsideRotatesAroundTheCenter) {
		TransformScene scene;
		scene.root.commands.run(Command::FreeTransform);
		scene.driver.layout();
		scene.driver.drag(scene.screenOf({ 200, 110 }), scene.screenOf({ 120, 190 }), 10);
		const LayerTransform& transform = scene.root.canvas.pendingTransform();
		CHECK_CLOSE(3.14159265f / 2, transform.rotation, 0.02f);
		const Vec2F center = transform.apply({ 120, 110 });
		CHECK_CLOSE(120.0f, center.x, 0.1f);
		CHECK_CLOSE(110.0f, center.y, 0.1f);
		scene.root.canvas.applyTransform();
		checkRgb({ 1, 0, 0, 1 }, scene.at(120, 125));
		CHECK_CLOSE(0.0f, scene.at(135, 110).a, 1e-6f);
		checkRgb({ 0, 0, 1, 1 }, scene.at(118, 94));
	}

	TEST(FlipButtonsMirrorThePendingTransform) {
		TransformScene scene;
		scene.root.commands.run(Command::FreeTransform);
		scene.driver.layout();
		scene.driver.click(scene.root.transformBar.flip);
		CHECK_CLOSE(-1.0f, scene.root.canvas.pendingTransform().scale.x, 1e-6f);
		scene.driver.click(scene.root.transformBar.flop);
		CHECK_CLOSE(-1.0f, scene.root.canvas.pendingTransform().scale.y, 1e-6f);
		scene.root.commands.run(Command::FlipVertical);
		CHECK_CLOSE(1.0f, scene.root.canvas.pendingTransform().scale.y, 1e-6f);
		scene.root.canvas.applyTransform();
		checkRgb({ 0, 0, 1, 1 }, scene.at(135, 110));
	}

	TEST(ChoosingAnotherToolAppliesTheTransform) {
		TransformScene scene;
		scene.root.commands.run(Command::FreeTransform);
		scene.driver.layout();
		scene.driver.drag(scene.screenOf({ 120, 110 }), scene.screenOf({ 120, 150 }));
		scene.root.commands.run(Command::ToolPaint);
		CHECK(!scene.root.canvas.isTransforming());
		CHECK(scene.root.canvas.tool() == CanvasTool::Paint);
		checkRgb({ 1, 0, 0, 1 }, scene.at(120, 150));
	}

	TEST(ApplyingWithoutChangesAddsNoUndoStep) {
		TransformScene scene;
		const size_t revision = scene.document.history().revision();
		scene.root.commands.run(Command::FreeTransform);
		scene.root.canvas.applyTransform();
		CHECK_EQUAL(revision, scene.document.history().revision());
		checkRgb({ 1, 0, 0, 1 }, scene.at(120, 110));
	}

	TEST(TabletToolsMenuStartsATransform) {
		Document document;
		Brush brush;
		TransformViewport viewport;
		TransformIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };
		UiDriver driver{ studio, { 1280, 800 } };
		fillRect(document, 10, 10, 20, 20, { 1, 0, 0, 1 });
		studio.useTool("transform");
		CHECK(classic.canvas.isTransforming());
		CHECK(!classic.transformBar.isVisible());
		CHECK(studio.activePanel() == ActivePanel::Transform);
	}
}
