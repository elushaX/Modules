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
	class LineViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class LineIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	struct LineScene {
		Document document;
		Brush brush;
		LineViewport viewport;
		LineIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		LineScene() {
			root.canvas.settings().viewStiffness = 1;
			root.canvas.settings().line.holdForLine = true;
			brush.apply(*findBrushPreset(BrushType::Round));
			brush.setSize(6);
			brush.setStabilizer(0);
		}

		Vec2F screenOf(Vec2F world) { return root.canvas.windowRect().pos + document.worldToScreen(world); }
		halnf alphaAt(halnf x, halnf y) {
			document.settlePaint();
			return document.sampleColor({ x, y }, false).a;
		}

		void wiggle() {
			press({ 100, 100 });
			driver.moveTo(screenOf({ 150, 160 }));
			driver.moveTo(screenOf({ 220, 120 }));
			driver.moveTo(screenOf({ 300, 200 }));
		}

		void press(Vec2F world) {
			driver.moveTo(screenOf(world));
			driver.press();
		}
	};
}

SUITE(HoldForLine) {
	TEST(ReferenceDefaults) {
		const LineSettings settings;
		CHECK(!settings.holdForLine);
		CHECK_CLOSE(0.5f, settings.holdSeconds, 1e-6f);
	}

	TEST(HoldingStillTurnsTheStrokeIntoAStraightLine) {
		LineScene scene;
		scene.wiggle();
		CHECK(scene.alphaAt(150, 160) > 0.5f);
		CHECK(!scene.root.canvas.isDrawingLine());
		scene.driver.advance(600);
		CHECK(scene.root.canvas.isDrawingLine());
		CHECK_CLOSE(0.0f, scene.alphaAt(150, 160), 1e-6f);
		CHECK(scene.alphaAt(200, 150) > 0.5f);
		CHECK(scene.alphaAt(100, 100) > 0.5f);
		CHECK(scene.alphaAt(300, 200) > 0.5f);
	}

	TEST(TheLineFollowsThePenAfterLocking) {
		LineScene scene;
		scene.wiggle();
		scene.driver.advance(600);
		scene.driver.moveTo(scene.screenOf({ 100, 300 }));
		CHECK(scene.alphaAt(100, 200) > 0.5f);
		CHECK_CLOSE(0.0f, scene.alphaAt(200, 150), 1e-6f);
		const size_t revision = scene.document.history().revision();
		scene.driver.release();
		CHECK_EQUAL(revision + 1, scene.document.history().revision());
		CHECK(scene.alphaAt(100, 250) > 0.5f);
		CHECK(!scene.root.canvas.isDrawingLine());
		scene.root.commands.run(Command::Undo);
		CHECK_CLOSE(0.0f, scene.alphaAt(100, 250), 1e-6f);
	}

	TEST(MovingResetsTheHoldTimer) {
		LineScene scene;
		scene.wiggle();
		for (int4 step = 0; step < 6; step++) {
			scene.driver.advance(200);
			scene.driver.moveTo(scene.screenOf({ 300.0f + (halnf) step * 10.0f, 200.0f }));
		}
		CHECK(!scene.root.canvas.isDrawingLine());
		CHECK(scene.alphaAt(150, 160) > 0.5f);
	}

	TEST(ShortHoldOrDisabledKeepsTheFreehandStroke) {
		LineScene scene;
		scene.wiggle();
		scene.driver.advance(300);
		CHECK(!scene.root.canvas.isDrawingLine());
		scene.driver.release();

		LineScene off;
		off.root.canvas.settings().line.holdForLine = false;
		off.wiggle();
		off.driver.advance(1500);
		CHECK(!off.root.canvas.isDrawingLine());
		CHECK(off.alphaAt(150, 160) > 0.5f);
	}

	TEST(HoldingAtTheStartDoesNotLock) {
		LineScene scene;
		scene.press({ 100, 100 });
		scene.driver.advance(1000);
		CHECK(!scene.root.canvas.isDrawingLine());
		scene.driver.moveTo(scene.screenOf({ 150, 160 }));
		scene.driver.moveTo(scene.screenOf({ 200, 100 }));
		CHECK(scene.alphaAt(150, 160) > 0.5f);
	}

	TEST(HoldTimeIsConfigurableAndSaved) {
		LineScene scene;
		MenuItem* longer = nullptr;
		for (const auto& item : scene.root.lineMenu.items())
			if (item->entry().text == "Hold time 1.2 s") longer = item.get();
		REQUIRE CHECK(longer != nullptr);
		longer->entry().action();
		scene.wiggle();
		scene.driver.advance(800);
		CHECK(!scene.root.canvas.isDrawingLine());
		scene.driver.advance(600);
		CHECK(scene.root.canvas.isDrawingLine());
		scene.driver.release();

		const auto restored = SessionSnapshot::parse(
			SessionSnapshot::capture(scene.document, scene.brush, scene.root.canvas.settings(), {}).serialize()
		);
		REQUIRE CHECK(restored.has_value());
		CHECK_CLOSE(1.2f, restored->settings.line.holdSeconds, 1e-4f);
		CHECK(restored->settings.line.holdForLine);
	}

	TEST(ToolbarAndTabletOpenTheLineMenu) {
		LineScene scene;
		scene.driver.click(scene.root.toolBar.line);
		CHECK(scene.root.lineMenu.isOpen());

		Document document;
		Brush brush;
		LineViewport viewport;
		LineIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };
		UiDriver driver{ studio, { 1280, 800 } };
		bool found = false;
		for (const auto& item : classic.mainMenu.items())
			if (item->entry().text == "Line" && item->entry().submenu == &classic.lineMenu) found = true;
		CHECK(found);
	}
}

SUITE(LineModes) {
	TEST(RulerModeDrawsAStraightLineImmediately) {
		LineScene scene;
		scene.root.commands.run(Command::LineRuler);
		CHECK(scene.root.canvas.settings().line.mode == LineMode::Ruler);
		scene.press({ 100, 100 });
		scene.driver.moveTo(scene.screenOf({ 150, 180 }));
		scene.driver.moveTo(scene.screenOf({ 300, 100 }));
		CHECK(scene.root.canvas.isDrawingLine());
		CHECK_CLOSE(0.0f, scene.alphaAt(150, 180), 1e-6f);
		CHECK(scene.alphaAt(200, 100) > 0.5f);
		scene.driver.release();
		CHECK(scene.alphaAt(200, 100) > 0.5f);
	}

	TEST(EllipseModeDrawsAnEllipseInTheDraggedBox) {
		LineScene scene;
		scene.root.commands.run(Command::LineEllipse);
		const size_t revision = scene.document.history().revision();
		scene.press({ 100, 100 });
		scene.driver.moveTo(scene.screenOf({ 200, 150 }));
		scene.driver.moveTo(scene.screenOf({ 300, 200 }));
		scene.driver.release();
		CHECK_EQUAL(revision + 1, scene.document.history().revision());
		CHECK(scene.alphaAt(300, 150) > 0.5f);
		CHECK(scene.alphaAt(100, 150) > 0.5f);
		CHECK(scene.alphaAt(200, 100) > 0.5f);
		CHECK(scene.alphaAt(200, 200) > 0.5f);
		CHECK_CLOSE(0.0f, scene.alphaAt(200, 150), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(100, 100), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(300, 200), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(200, 125), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(150, 150), 1e-6f);
	}

	TEST(ShiftMakesACircle) {
		LineScene scene;
		scene.root.commands.run(Command::LineEllipse);
		scene.press({ 100, 100 });
		scene.driver.pressKey(InputID::LEFT_SHIFT);
		scene.driver.moveTo(scene.screenOf({ 300, 160 }));
		scene.driver.release();
		scene.driver.releaseKey(InputID::LEFT_SHIFT);
		CHECK(scene.alphaAt(200, 300) > 0.5f);
		CHECK(scene.alphaAt(300, 200) > 0.5f);
	}

	TEST(ModesAreCheckedInTheMenuAndSaved) {
		LineScene scene;
		std::vector<std::string> texts;
		for (const auto& item : scene.root.lineMenu.items())
			texts.push_back(item->entry().text);
		CHECK(texts.size() >= 3);
		CHECK_EQUAL(std::string("Line"), texts[0]);
		CHECK_EQUAL(std::string("Ruler"), texts[1]);
		CHECK_EQUAL(std::string("Ellipse"), texts[2]);
		scene.driver.key(InputID::N4, Modifiers{ false, true, false });
		CHECK(scene.root.canvas.settings().line.mode == LineMode::Ellipse);
		CHECK(scene.root.lineMenu.items()[2]->entry().isChecked());
		CHECK(!scene.root.lineMenu.items()[0]->entry().isChecked());
		const auto restored = SessionSnapshot::parse(
			SessionSnapshot::capture(scene.document, scene.brush, scene.root.canvas.settings(), {}).serialize()
		);
		REQUIRE CHECK(restored.has_value());
		CHECK(restored->settings.line.mode == LineMode::Ellipse);
		scene.root.commands.run(Command::LineNormal);
		CHECK(scene.root.canvas.settings().line.mode == LineMode::Normal);
	}
}

SUITE(Symmetry) {
	TEST(MirrorReflectsAcrossTheLine) {
		SymmetrySettings vertical;
		vertical.center = { 100, 0 };
		const Vec2F mirrored = vertical.mirror({ 70, 40 });
		CHECK_CLOSE(130.0f, mirrored.x, 1e-3f);
		CHECK_CLOSE(40.0f, mirrored.y, 1e-3f);
		SymmetrySettings diagonal;
		diagonal.angle = 3.14159265f / 4;
		const Vec2F flipped = diagonal.mirror({ 10, 0 });
		CHECK_CLOSE(0.0f, flipped.x, 1e-3f);
		CHECK_CLOSE(10.0f, flipped.y, 1e-3f);
	}

	TEST(StrokesArePaintedOnBothSides) {
		LineScene scene;
		scene.root.commands.run(Command::LineSymmetry);
		REQUIRE CHECK(scene.root.canvas.settings().symmetry.enabled);
		scene.root.canvas.settings().symmetry.center = { 300, 0 };
		scene.root.canvas.settings().symmetry.angle = 3.14159265f / 2;
		const size_t revision = scene.document.history().revision();
		scene.press({ 200, 100 });
		scene.driver.moveTo(scene.screenOf({ 250, 150 }));
		scene.driver.release();
		CHECK_EQUAL(revision + 1, scene.document.history().revision());
		CHECK(scene.alphaAt(250, 150) > 0.5f);
		CHECK(scene.alphaAt(350, 150) > 0.5f);
		CHECK(scene.alphaAt(400, 100) > 0.5f);
		scene.root.commands.run(Command::LineSymmetry);
		scene.press({ 200, 300 });
		scene.driver.moveTo(scene.screenOf({ 250, 300 }));
		scene.driver.release();
		CHECK_CLOSE(0.0f, scene.alphaAt(375, 300), 1e-6f);
	}

	TEST(EnablingCentersTheLineOnTheView) {
		LineScene scene;
		scene.root.commands.run(Command::LineSymmetry);
		const Vec2F expected = scene.document.screenToWorld(scene.root.canvas.size() / 2.0f);
		CHECK_CLOSE(expected.x, scene.root.canvas.settings().symmetry.center.x, 0.01f);
		CHECK_CLOSE(expected.y, scene.root.canvas.settings().symmetry.center.y, 0.01f);
	}

	TEST(LineIsDrawnAndCanBeHidden) {
		LineScene scene;
		scene.root.commands.run(Command::LineSymmetry);
		const auto blueLines = [&scene]() {
			scene.root.canvas.requestPaint();
			scene.driver.settle();
			size_t count = 0;
			for (const PaintCommand& command : scene.driver.paint().commands())
				if (command.kind == PaintCommand::Kind::Line && command.color.b > 0.9f && command.color.r < 0.5f) count++;
			return count;
		};
		CHECK(blueLines() > 0);
		scene.root.commands.run(Command::ShowSymmetryLine);
		CHECK_EQUAL((size_t) 0, blueLines());
		CHECK(!scene.root.canvas.settings().symmetry.visible);
	}

	TEST(MoveDragsTheLineAndLockStopsIt) {
		LineScene scene;
		scene.root.commands.run(Command::LineSymmetry);
		scene.root.canvas.settings().symmetry.center = { 300, 200 };
		scene.root.commands.run(Command::MoveSymmetryLine);
		CHECK(scene.root.canvas.isSymmetryMoveArmed());
		scene.driver.drag(scene.screenOf({ 300, 200 }), scene.screenOf({ 350, 220 }));
		CHECK_CLOSE(350.0f, scene.root.canvas.settings().symmetry.center.x, 0.5f);
		CHECK_CLOSE(220.0f, scene.root.canvas.settings().symmetry.center.y, 0.5f);
		CHECK(!scene.root.canvas.isSymmetryMoveArmed());
		CHECK_CLOSE(0.0f, scene.alphaAt(325, 210), 1e-6f);

		scene.root.commands.run(Command::LockSymmetryLine);
		scene.root.commands.run(Command::MoveSymmetryLine);
		CHECK(!scene.root.canvas.isSymmetryMoveArmed());
		CHECK_EQUAL(std::string("The symmetry line is locked"), scene.root.lastNotice());
		scene.root.commands.run(Command::ResetSymmetryLine);
		CHECK_CLOSE(350.0f, scene.root.canvas.settings().symmetry.center.x, 0.5f);
		scene.root.commands.run(Command::LockSymmetryLine);
		scene.root.commands.run(Command::ResetSymmetryLine);
		const Vec2F expected = scene.document.screenToWorld(scene.root.canvas.size() / 2.0f);
		CHECK_CLOSE(expected.x, scene.root.canvas.settings().symmetry.center.x, 0.01f);
	}

	TEST(SymmetryIsSavedWithTheSession) {
		LineScene scene;
		scene.root.commands.run(Command::LineSymmetry);
		scene.root.canvas.settings().symmetry.center = { 123, 45 };
		scene.root.canvas.settings().symmetry.angle = 0.5f;
		scene.root.commands.run(Command::LockSymmetryLine);
		const auto restored = SessionSnapshot::parse(
			SessionSnapshot::capture(scene.document, scene.brush, scene.root.canvas.settings(), {}).serialize()
		);
		REQUIRE CHECK(restored.has_value());
		const SymmetrySettings& symmetry = restored->settings.symmetry;
		CHECK(symmetry.enabled);
		CHECK(symmetry.locked);
		CHECK(symmetry.visible);
		CHECK_CLOSE(123.0f, symmetry.center.x, 1e-3f);
		CHECK_CLOSE(45.0f, symmetry.center.y, 1e-3f);
		CHECK_CLOSE(0.5f, symmetry.angle, 1e-4f);
	}

	TEST(SymmetryMenuListsTheCommands) {
		LineScene scene;
		std::vector<std::string> texts;
		for (const auto& item : scene.root.symmetryMenu.items())
			texts.push_back(item->entry().text);
		CHECK(
			texts == std::vector<std::string>(
								 { "Symmetry", "Show symmetry line", "Move symmetry line", "Lock symmetry line", "Reset symmetry line" }
							 )
		);
	}
}

SUITE(Perspective) {
	TEST(SnapPicksTheClosestGuide) {
		PerspectiveSettings perspective;
		perspective.type = PerspectiveType::TwoPoint;
		perspective.points[0] = { -1000, 0 };
		perspective.points[1] = { 1000, 0 };
		const Vec2F vertical = perspective.snap({ 0, 100 }, { 5, 200 });
		CHECK_CLOSE(0.0f, vertical.x, 1e-3f);
		CHECK_CLOSE(200.0f, vertical.y, 0.2f);
		const Vec2F toRight = perspective.snap({ 0, 100 }, { 300, 80 });
		const Vec2F expected = Vec2F{ 1000.0f, -100.0f };
		const halnf cross = (toRight.x - 0) * expected.y - (toRight.y - 100) * expected.x;
		CHECK_CLOSE(0.0f, cross, 1.0f);
	}

	TEST(IsometricUsesItsAngle) {
		PerspectiveSettings perspective;
		perspective.type = PerspectiveType::Isometric;
		perspective.isometricDegrees = 30;
		const Vec2F snapped = perspective.snap({ 0, 0 }, { 100, 70 });
		CHECK_CLOSE(std::tan(30.0f * 3.14159265f / 180.0f), snapped.y / snapped.x, 1e-3f);
		CHECK_EQUAL((size_t) 3, perspective.directions({ 0, 0 }).size());
	}

	TEST(RadialPointsAtTheCenter) {
		PerspectiveSettings perspective;
		perspective.type = PerspectiveType::Radial;
		perspective.points[0] = { 0, 0 };
		const Vec2F snapped = perspective.snap({ 100, 0 }, { 150, 30 });
		CHECK_CLOSE(0.0f, snapped.y, 1e-3f);
		CHECK_EQUAL(1, perspective.pointCount());
	}

	TEST(PerspectiveModeDrawsSnappedLines) {
		LineScene scene;
		scene.root.commands.run(Command::LinePerspective);
		CHECK(scene.root.canvas.settings().line.mode == LineMode::Perspective);
		PerspectiveSettings& perspective = scene.root.canvas.settings().perspective;
		perspective.type = PerspectiveType::OnePoint;
		perspective.points[0] = { 600, 100 };
		perspective.placed = true;
		scene.press({ 100, 300 });
		scene.driver.moveTo(scene.screenOf({ 300, 290 }));
		scene.driver.release();
		CHECK(scene.alphaAt(200, 300) > 0.5f);
		CHECK_CLOSE(0.0f, scene.alphaAt(300, 290), 1e-6f);
		scene.press({ 100, 400 });
		scene.driver.moveTo(scene.screenOf({ 350, 237 }));
		scene.driver.release();
		CHECK(scene.alphaAt(350, 400.0f - 250.0f * 300.0f / 500.0f) > 0.5f);
	}

	TEST(GuidesAreDrawnOnlyInPerspectiveMode) {
		LineScene scene;
		const auto orangeLines = [&scene]() {
			scene.root.canvas.requestPaint();
			scene.driver.settle();
			size_t count = 0;
			for (const PaintCommand& command : scene.driver.paint().commands())
				if (command.kind == PaintCommand::Kind::Line && command.color.r > 0.9f && command.color.b < 0.3f) count++;
			return count;
		};
		CHECK_EQUAL((size_t) 0, orangeLines());
		scene.root.commands.run(Command::LinePerspective);
		scene.root.commands.run(Command::ResetPerspective);
		CHECK_EQUAL((size_t) 48, orangeLines());
	}

	TEST(TypesMoveAndLock) {
		LineScene scene;
		scene.root.commands.run(Command::LinePerspective);
		MenuItem* three = nullptr;
		for (const auto& item : scene.root.perspectiveMenu.items())
			if (item->entry().text == "3 point") three = item.get();
		REQUIRE CHECK(three != nullptr);
		three->entry().action();
		PerspectiveSettings& perspective = scene.root.canvas.settings().perspective;
		CHECK(perspective.type == PerspectiveType::ThreePoint);
		CHECK(perspective.placed);
		const Vec2F second = perspective.points[1];
		scene.root.commands.run(Command::MovePerspectivePoint);
		CHECK(scene.root.canvas.isPerspectiveMoveArmed());
		scene.driver.drag(scene.screenOf(second), scene.screenOf(second + Vec2F{ 40.0f, 30.0f }));
		CHECK_CLOSE(second.x + 40, perspective.points[1].x, 0.5f);
		CHECK_CLOSE(second.y + 30, perspective.points[1].y, 0.5f);
		CHECK(!scene.root.canvas.isPerspectiveMoveArmed());

		scene.root.commands.run(Command::LockPerspective);
		scene.root.commands.run(Command::MovePerspectivePoint);
		CHECK(!scene.root.canvas.isPerspectiveMoveArmed());
		CHECK_EQUAL(std::string("The perspective is locked"), scene.root.lastNotice());
		three->entry().action();
		CHECK_CLOSE(second.x + 40, perspective.points[1].x, 0.5f);
	}

	TEST(IsometricCommandAndSession) {
		LineScene scene;
		scene.driver.key(InputID::N7, Modifiers{ false, true, false });
		CHECK(scene.root.canvas.settings().line.mode == LineMode::Perspective);
		CHECK(scene.root.canvas.settings().perspective.type == PerspectiveType::Isometric);
		scene.root.canvas.settings().perspective.isometricDegrees = 45;
		scene.root.canvas.settings().perspective.points[0] = { 11, 22 };
		const auto restored = SessionSnapshot::parse(
			SessionSnapshot::capture(scene.document, scene.brush, scene.root.canvas.settings(), {}).serialize()
		);
		REQUIRE CHECK(restored.has_value());
		CHECK(restored->settings.line.mode == LineMode::Perspective);
		CHECK(restored->settings.perspective.type == PerspectiveType::Isometric);
		CHECK_CLOSE(45.0f, restored->settings.perspective.isometricDegrees, 1e-3f);
		CHECK_CLOSE(22.0f, restored->settings.perspective.points[0].y, 1e-3f);
	}
}
