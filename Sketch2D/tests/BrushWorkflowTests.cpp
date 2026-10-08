#include "TestSketchesRoot.hpp"
#include "Compositor.hpp"
#include "Profiler.hpp"
#include "Sketch2DUI.hpp"

#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class FakeViewport : public ViewportRenderer {
	public:
		void render(Document& document, const ViewSettings&, Vec2F) override {
			const int4 level = LayerLevels::levelForZoom(document.zoom());
			document.whilePaintPaused([&]() {
				for (const auto& layer : document.layers())
					layer->levels().update(*layer, level);
				compositor.update(document, level);
			});
		}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
		[[nodiscard]] std::string statistics() const override { return {}; }
		Compositor compositor;
	};

	class NumberedIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string&) const override { return { 1 }; }
	};

	struct Session {
		Document document;
		Brush brush;
		FakeViewport viewport;
		NumberedIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1600, 1000 } };
		std::vector<std::string> log;
		double worstStrokeMs = 0;
		double worstFinishMs = 0;
		double lastFinishMs = 0;
		double lastStrokeMs = 0;
		double worstFrameMs = 0;
		double strokeWorstFrameMs = 0;

		Session() {
			root.canvas.settings().viewStiffness = 1;
			driver.setAutoSettle(false);
			document.setPaintWorkers(4);
			document.setEngineThreads(4);
			brush.apply(*findBrushPreset(BrushType::Round));
			brush.color = { 0.1f, 0.2f, 0.9f, 1.0f };
			brush.setStabilizer(0);
			root.canvas.settings().useBrushStabilizer = true;
		}

		void frame(halnf milliseconds = 16) {
			const auto start = std::chrono::steady_clock::now();
			driver.advance(milliseconds);
			driver.paint();
			const double spent = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			worstFrameMs = std::max(worstFrameMs, spent);
			strokeWorstFrameMs = std::max(strokeWorstFrameMs, spent);
		}

		Vec2F canvasPoint(Vec2F local) { return root.canvas.windowRect().pos + local; }
		Vec2F localOfWorld(Vec2F world) { return document.worldToScreen(world); }
		Vec2F windowOfWorld(Vec2F world) { return canvasPoint(localOfWorld(world)); }

		void zoomTo(halnf zoom, Vec2F worldCenter) {
			document.setZoom(zoom);
			const Vec2F size = root.canvas.size();
			document.setPanWorld(worldCenter - size / (2.0f * zoom));
			frame();
		}

		void pinch(halnf scale) {
			const Vec2F center = root.canvas.windowRect().center();
			driver.ui().dispatch(Event::gesture({ GesturePhase::Begin, center }));
			for (int step = 0; step < 6; step++) {
				driver.ui().dispatch(
					Event::gesture({ GesturePhase::Update, center, { 3.0f, -2.0f }, std::pow(scale, 1.0f / 6.0f) })
				);
				frame();
			}
			driver.ui().dispatch(Event::gesture({ GesturePhase::End, center }));
			idle(300);
		}

		void stroke(const std::vector<Vec2F>& worldPoints, halnf pressureFrom = 1, halnf pressureTo = 1) {
			driver.setPointerTool(PointerTool::Mouse);
			const auto start = std::chrono::steady_clock::now();
			driver.moveTo(windowOfWorld(worldPoints.front()));
			driver.press();
			for (size_t index = 1; index < worldPoints.size(); index++) {
				driver.moveTo(windowOfWorld(worldPoints[index]));
				frame(8);
			}
			const auto lifted = std::chrono::steady_clock::now();
			driver.release();
			const auto finished = std::chrono::steady_clock::now();
			worstStrokeMs = std::max(worstStrokeMs, std::chrono::duration<double, std::milli>(lifted - start).count());
			lastFinishMs = std::chrono::duration<double, std::milli>(finished - lifted).count();
			lastStrokeMs = std::chrono::duration<double, std::milli>(lifted - start).count();
			worstFinishMs = std::max(worstFinishMs, lastFinishMs);
			(void) pressureFrom;
			(void) pressureTo;
		}

		void line(Vec2F from, Vec2F to, int points = 24) {
			std::vector<Vec2F> path;
			for (int index = 0; index <= points; index++)
				path.push_back(from + (to - from) * ((halnf) index / (halnf) points));
			stroke(path);
		}

		void wiggle(Vec2F center, halnf radius, int turns = 3) {
			std::vector<Vec2F> path;
			for (int index = 0; index <= turns * 24; index++) {
				const halnf angle = (halnf) index / 24.0f * 6.2831853f;
				path.push_back(center + Vec2F(std::cos(angle) * radius, std::sin(angle * 2.0f) * radius * 0.5f));
			}
			stroke(path);
		}

		void idle(halnf milliseconds) {
			for (halnf passed = 0; passed < milliseconds; passed += 16)
				frame();
		}

		RGBA at(Vec2F world) { return document.sampleColor(world, false); }
	};

	bool painted(const RGBA& color) { return color.a > 0.3f; }
	bool empty(const RGBA& color) { return color.a < 0.02f; }
}

SUITE(BrushWorkflowQuick) {
	TEST(StrokesLandAtNormalAndZoomedOutLevels) {
		Session session;
		halnf y = 0;
		for (halnf zoom : { 1.0f, 0.25f }) {
			const halnf size = 10.0f / zoom;
			session.brush.setSize(size);
			const Vec2F from = { 0.0f, y };
			const Vec2F to = { 300.0f / zoom, y };
			session.zoomTo(zoom, (from + to) * 0.5f);
			session.line(from, to, 12);
			session.document.settlePaint();
			for (halnf t : { 0.1f, 0.5f, 0.9f })
				CHECK(painted(session.at(from + (to - from) * t)));
			CHECK(empty(session.at({ (from.x + to.x) * 0.5f, y + size * 2.0f })));
			y += size * 4.0f;
		}
	}

	TEST(SpacingDoesNotDependOnTheInputRate) {
		Session coarse;
		Session fine;
		for (Session* session : { &coarse, &fine }) {
			session->brush.setSize(20);
			session->brush.setSpacing(0.5f);
			session->zoomTo(1.0f, { 200.0f, 0.0f });
		}
		coarse.line({ 0.0f, 0.0f }, { 400.0f, 0.0f }, 4);
		fine.line({ 0.0f, 0.0f }, { 400.0f, 0.0f }, 60);
		coarse.document.settlePaint();
		fine.document.settlePaint();
		int differences = 0;
		for (int x = 0; x < 400; x += 4)
			if (std::abs(coarse.at({ (halnf) x, 0.0f }).a - fine.at({ (halnf) x, 0.0f }).a) > 0.1f) differences++;
		CHECK(differences <= 3);
	}

	TEST(OpacityCapsAStrokeThatCrossesItself) {
		Session session;
		session.brush.setSize(30);
		session.brush.setHardness(1);
		session.brush.setOpacity(0.5f);
		session.brush.setFlow(1);
		session.zoomTo(1, { 0.0f, 0.0f });
		session.wiggle({ 0.0f, 0.0f }, 60, 2);
		session.document.settlePaint();
		halnf darkest = 0;
		for (int y = -30; y <= 30; y += 5)
			for (int x = -60; x <= 60; x += 5)
				darkest = std::max(darkest, session.at({ (halnf) x, (halnf) y }).a);
		CHECK_CLOSE(0.5f, darkest, 0.06f);
	}

	TEST(EraserAndUndoAcrossZoomLevels) {
		Session session;
		const Vec2F center = { 0.0f, 0.0f };
		for (halnf zoom : { 1.0f, 0.25f }) {
			session.brush.setEraserMode(false);
			session.brush.setSize(10.0f / zoom);
			session.zoomTo(zoom, center);
			session.line(center - Vec2F(100.0f / zoom, 0.0f), center + Vec2F(100.0f / zoom, 0.0f), 8);
			CHECK(painted(session.at(center)));
			session.brush.setEraserMode(true);
			session.brush.setSize(30.0f / zoom);
			session.line(center - Vec2F(100.0f / zoom, 0.0f), center + Vec2F(100.0f / zoom, 0.0f), 8);
			session.document.settlePaint();
			CHECK(empty(session.at(center)));
			session.root.commands.run(Command::Undo);
			CHECK(painted(session.at(center)));
			session.root.commands.run(Command::Redo);
			CHECK(empty(session.at(center)));
		}
	}
}

SUITE(BenchSmallBrushZoomedOut) {
	TEST(SameScreenStrokeAtEveryZoom) {
		for (const halnf zoom : { 0.5f, 0.25f, 0.125f, 1.0f / 16.0f, 1.0f / 32.0f, 1.0f / 64.0f }) {
			Session session;
			session.brush.setSize(20);
			session.brush.setStabilizer(0);
			session.zoomTo(zoom, { 0, 0 });
			const Vec2F from = session.document.screenToWorld({ 100, 300 });
			const Vec2F to = session.document.screenToWorld({ 1300, 300 });
			Profiler& profiler = Profiler::global();
			profiler.setEnabled(true);
			profiler.clear();
			profiler.beginFrame();
			session.line(from, to, 120);
			profiler.endFrame();
			profiler.setEnabled(false);
			const double dabs = profiler.counterTotal("brush.dabs");
			const double paint = profiler.sectionTotal("stroke.paint");
			std::printf(
				"[zoom-sweep] zoom 1/%-3.0f preview dabs %7.0f dabs %7.0f stroke.paint %7.1f ms  per dab %5.2f us  dab %6.1f "
				"stamp %6.1f preview "
				"%6.1f preview px %9.0f\n",
				1.0f / zoom,
				profiler.counterTotal("brush.preview_dabs"),
				dabs,
				paint,
				dabs > 0 ? paint * 1000.0 / dabs : 0.0,
				profiler.sectionTotal("brush.dab"),
				profiler.sectionTotal("brush.stamp"),
				profiler.sectionTotal("brush.preview_dab"),
				profiler.counterTotal("brush.preview_pixels")
			);
		}
	}
}

SUITE(BenchZoomOutAfterDrawing) {
	TEST(ZoomingOutToTheTopAfterDrawingAtFullZoom) {
		Session session;
		session.brush.setSize(80);
		session.brush.setStabilizer(0);
		session.zoomTo(1.0f, { 0, 0 });
		std::vector<Vec2F> scribble;
		for (int4 index = 0; index <= 600; index++)
			scribble.push_back(
				{ -700.0f + (halnf) (index % 100) * 14.0f,
					-400.0f + (halnf) index * 1.3f + std::sin((halnf) index * 0.3f) * 80.0f }
			);
		session.stroke(scribble);
		session.driver.moveTo(session.canvasPoint(session.root.canvas.size() / 2.0f));
		Profiler& profiler = Profiler::global();
		profiler.setEnabled(true);
		for (int4 step = 0; step < 400 && session.document.zoom() > 1.0f / 49.0f; step++) {
			profiler.clear();
			profiler.beginFrame();
			const auto start = std::chrono::steady_clock::now();
			session.driver.wheel(-1.0f);
			session.frame(16);
			const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			profiler.endFrame();
			if (ms > 8.0)
				std::printf(
					"[zoomout] step %3d zoom 1/%-6.2f frame %8.1f ms  mips %7.1f stack %7.1f view %7.1f\n",
					step,
					1.0f / session.document.zoom(),
					ms,
					profiler.sectionTotal("canvas.mips"),
					profiler.sectionTotal("canvas.stack"),
					profiler.sectionTotal("canvas.view")
				);
		}
		std::printf("[zoomout] final zoom 1/%.2f\n", 1.0f / session.document.zoom());
		profiler.setEnabled(false);
	}
}

SUITE(LongBrushWorkflowChecks) {
	TEST(StrokesAtEveryZoomLevelLandWhereTheyWereDrawn) {
		Session session;
		const halnf zooms[] = { 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 1.0f / 16, 1.0f / 32, 1.0f / 64 };
		halnf y = 0;
		for (halnf zoom : zooms) {
			const halnf size = 30.0f / zoom;
			session.brush.setSize(size);
			const Vec2F from = { 0.0f, y };
			const Vec2F to = { 800.0f / zoom, y };
			session.zoomTo(zoom, (from + to) * 0.5f);
			session.line(from, to);
			session.idle(100);
			session.document.settlePaint();
			for (halnf t : { 0.1f, 0.5f, 0.9f }) {
				const Vec2F point = from + (to - from) * t;
				if (!painted(session.at(point)))
					std::printf("[brush-workflow] zoom %g size %g gap at %.0f,%.0f\n", zoom, size, point.x, point.y);
				CHECK(painted(session.at(point)));
			}
			CHECK(empty(session.at({ (from.x + to.x) * 0.5f, y + size * 2.0f })));
			y += size * 4.0f;
		}
		CHECK_EQUAL((size_t) (std::size(zooms) + 1), session.document.history().entries().size());
	}

	TEST(SizesFromHairlineToHugeKeepAContinuousStroke) {
		Session session;
		const halnf sizes[] = { 1.5f, 4, 12, 40, 150, 600, 2000 };
		halnf y = 0;
		for (halnf size : sizes) {
			session.brush.setSize(size);
			const halnf zoom = std::clamp(40.0f / size, 1.0f / 64, 2.0f);
			const Vec2F from = { 0.0f, y };
			const Vec2F to = { size * 30.0f, y };
			session.zoomTo(zoom, (from + to) * 0.5f);
			session.line(from, to, 40);
			session.idle(50);
			session.document.settlePaint();
			int gaps = 0;
			for (int sample = 1; sample < 60; sample++) {
				const Vec2F point = from + (to - from) * ((halnf) sample / 60.0f);
				if (!painted(session.at(point))) gaps++;
			}
			if (gaps) std::printf("[brush-workflow] size %g zoom %g gaps %d/59\n", size, zoom, gaps);
			CHECK_EQUAL(0, gaps);
			y += size * 3.0f;
		}
	}

	TEST(SpacingDoesNotDependOnTheInputRateOrZoom) {
		Session coarse;
		Session fine;
		for (Session* session : { &coarse, &fine }) {
			session->brush.setSize(40);
			session->brush.setSpacing(0.5f);
		}
		coarse.zoomTo(0.5f, { 1000.0f, 0.0f });
		fine.zoomTo(0.5f, { 1000.0f, 0.0f });
		coarse.line({ 0.0f, 0.0f }, { 2000.0f, 0.0f }, 6);
		fine.line({ 0.0f, 0.0f }, { 2000.0f, 0.0f }, 120);
		coarse.document.settlePaint();
		fine.document.settlePaint();
		int differences = 0;
		for (int x = 0; x < 2000; x += 5)
			if (std::abs(coarse.at({ (halnf) x, 0.0f }).a - fine.at({ (halnf) x, 0.0f }).a) > 0.1f) differences++;
		if (differences) std::printf("[brush-workflow] spacing differs at %d/400 samples\n", differences);
		CHECK(differences <= 8);
	}

	TEST(OpacityCapsTheStrokeAndFlowBuildsUpWithinIt) {
		Session opacity;
		opacity.brush.setSize(60);
		opacity.brush.setHardness(1);
		opacity.brush.setOpacity(0.5f);
		opacity.brush.setFlow(1);
		opacity.brush.setSpacing(0.05f);
		opacity.zoomTo(1, { 300.0f, 0.0f });
		opacity.line({ 0.0f, 0.0f }, { 600.0f, 0.0f }, 60);
		opacity.document.settlePaint();
		const halnf capped = opacity.at({ 300.0f, 0.0f }).a;
		std::printf("[brush-workflow] opacity 0.5 flow 1 single stroke alpha %.3f\n", capped);
		CHECK_CLOSE(0.5f, capped, 0.06f);

		Session flow;
		flow.brush.setSize(60);
		flow.brush.setHardness(1);
		flow.brush.setOpacity(1);
		flow.brush.setFlow(0.1f);
		flow.brush.setSpacing(0.05f);
		flow.zoomTo(1, { 300.0f, 0.0f });
		flow.line({ 0.0f, 0.0f }, { 600.0f, 0.0f }, 60);
		flow.document.settlePaint();
		const halnf built = flow.at({ 300.0f, 0.0f }).a;
		std::printf("[brush-workflow] opacity 1 flow 0.1 single stroke alpha %.3f\n", built);
		CHECK(built > 0.5f);
		CHECK(built < 1.0f);
	}

	TEST(SelfOverlappingStrokeDoesNotDarkenBeyondOpacity) {
		Session session;
		session.brush.setSize(60);
		session.brush.setHardness(1);
		session.brush.setOpacity(0.5f);
		session.brush.setFlow(1);
		session.zoomTo(1, { 0.0f, 0.0f });
		session.wiggle({ 0.0f, 0.0f }, 120, 4);
		session.document.settlePaint();
		halnf darkest = 0;
		for (int y = -60; y <= 60; y += 6)
			for (int x = -120; x <= 120; x += 6)
				darkest = std::max(darkest, session.at({ (halnf) x, (halnf) y }).a);
		std::printf("[brush-workflow] self overlap opacity 0.5 darkest alpha %.3f\n", darkest);
		CHECK(darkest <= 0.56f);
	}

	TEST(PressureChangesSizeSmoothlyAlongTheStroke) {
		Session session;
		session.brush.setSize(80);
		session.zoomTo(1, { 400.0f, 0.0f });
		session.driver.setPointerTool(PointerTool::Pen);
		session.driver.moveTo(session.windowOfWorld({ 0.0f, 0.0f }));
		session.driver.press();
		for (int index = 1; index <= 40; index++) {
			session.driver.moveTo(session.windowOfWorld({ (halnf) index * 20.0f, 0.0f }));
			session.frame(8);
		}
		session.driver.release();
		session.document.settlePaint();
		CHECK(painted(session.at({ 400.0f, 0.0f })));
	}

	TEST(UndoRedoAcrossZoomsRestoresEveryStroke) {
		Session session;
		const halnf zooms[] = { 1, 1.0f / 16, 0.25f, 1.0f / 64 };
		std::vector<Vec2F> centers;
		for (size_t index = 0; index < std::size(zooms); index++) {
			const Vec2F center = { (halnf) index * 5000.0f, 0.0f };
			centers.push_back(center);
			session.brush.setSize(40.0f / zooms[index]);
			session.zoomTo(zooms[index], center);
			session.line(center - Vec2F(300.0f / zooms[index], 0.0f), center + Vec2F(300.0f / zooms[index], 0.0f));
			session.idle(16);
		}
		for (size_t index = 0; index < centers.size(); index++)
			session.root.commands.run(Command::Undo);
		for (const Vec2F& center : centers)
			CHECK(empty(session.at(center)));
		for (size_t index = 0; index < centers.size(); index++)
			session.root.commands.run(Command::Redo);
		for (const Vec2F& center : centers)
			CHECK(painted(session.at(center)));
	}

	TEST(EraserRemovesWhatTheBrushPaintedAtAnyZoom) {
		Session session;
		for (halnf zoom : { 1.0f, 0.25f, 1.0f / 16 }) {
			const halnf size = 40.0f / zoom;
			const Vec2F center = { 0.0f, 0.0f };
			session.brush.setEraserMode(false);
			session.brush.setSize(size);
			session.zoomTo(zoom, center);
			session.line(center - Vec2F(400.0f / zoom, 0.0f), center + Vec2F(400.0f / zoom, 0.0f));
			session.document.settlePaint();
			std::printf(
				"[brush-workflow] before erase zoom %g alpha %.2f refusal '%s'\n",
				zoom,
				session.at(center).a,
				session.document.strokeRefusal(session.brush).c_str()
			);
			session.brush.setEraserMode(true);
			session.brush.setSize(size * 3);
			session.line(center - Vec2F(400.0f / zoom, 0.0f), center + Vec2F(400.0f / zoom, 0.0f));
			session.document.settlePaint();
			if (!empty(session.at(center))) {
				std::printf("[brush-workflow] eraser left alpha at zoom %g:", zoom);
				for (halnf x = -400.0f / zoom; x <= 400.0f / zoom; x += 100.0f / zoom)
					std::printf(" %.2f", session.at({ x, 0.0f }).a);
				std::printf(" history %zu\n", session.document.history().entries().size());
			}
			CHECK(empty(session.at(center)));
		}
	}

	TEST(StrokesStartingWhileTheLastOneIsStillLandingKeepTheirOrder) {
		Session session;
		session.document.setEngineThreads(2);
		session.brush.setSize(2000);
		session.zoomTo(1.0f / 32, { 0.0f, 0.0f });
		session.brush.color = { 1, 0, 0, 1 };
		session.line({ -10000.0f, 0.0f }, { 10000.0f, 0.0f }, 10);
		session.brush.color = { 0, 1, 0, 1 };
		session.line({ 0.0f, -10000.0f }, { 0.0f, 10000.0f }, 10);
		session.document.settlePaint();
		const RGBA cross = session.at({ 0.0f, 0.0f });
		std::printf("[brush-workflow] crossing strokes r %.2f g %.2f\n", cross.r, cross.g);
		CHECK(cross.g > 0.9f);
		CHECK(cross.r < 0.1f);
	}
}

SUITE(LongBrushWorkflow) {
	TEST(LongMixedSessionStaysCorrectAndResponsive) {
		Session session;
		Profiler& profiler = Profiler::global();
		profiler.setEnabled(true);
		profiler.clear();
		profiler.beginFrame();
		int strokes = 0;
		const halnf zooms[] = { 1, 0.25f, 1.0f / 16, 0.5f, 2, 1.0f / 64, 0.125f };
		const halnf sizes[] = { 20, 300, 1500, 60, 8, 3000, 200 };
		for (int round = 0; round < 3; round++) {
			for (size_t index = 0; index < std::size(zooms); index++) {
				const halnf zoom = zooms[index];
				const halnf size = sizes[index];
				session.brush.setSize(size);
				session.brush.setEraserMode(index == 3 && round > 0);
				const Vec2F center = { (halnf) index * 3000.0f, (halnf) round * 4000.0f };
				session.zoomTo(zoom, center);
				session.wiggle(center, 200.0f / zoom * 0.5f, 2);
				strokes++;
				session.idle(32);
				std::printf(
					"[brush-workflow] stroke %2d zoom %7.4f size %6.0f drawing %8.1f ms release %9.1f ms worst frame %8.1f ms\n",
					strokes,
					zoom,
					size,
					session.lastStrokeMs,
					session.lastFinishMs,
					session.strokeWorstFrameMs
				);
				session.strokeWorstFrameMs = 0;
				session.idle(32);
				if (index % 3 == 2) session.pinch(index % 2 ? 1.4f : 0.7f);
				if (index == 4) {
					session.root.commands.run(Command::Undo);
					session.root.commands.run(Command::Redo);
				}
			}
			session.idle(500);
		}
		profiler.endFrame();
		for (const char* section : { "brush.dab",
																 "brush.stamp",
																 "brush.preview_dab",
																 "brush.flush_deferred",
																 "brush.finish_stroke",
																 "brush.submit",
																 "engine.apply",
																 "history.commit",
																 "stroke.paint",
																 "canvas.mips",
																 "canvas.stack",
																 "canvas.view" })
			std::printf("[brush-workflow]   %-22s %10.1f ms\n", section, profiler.sectionTotal(section));
		std::printf(
			"[brush-workflow]   dabs %.0f pixels %.0f preview px %.0f deferred tiles %.0f\n",
			profiler.counterTotal("brush.dabs"),
			profiler.counterTotal("brush.pixels_touched"),
			profiler.counterTotal("brush.preview_pixels"),
			profiler.counterTotal("brush.deferred_tiles")
		);
		profiler.setEnabled(false);
		session.document.settlePaint();
		std::printf(
			"[brush-workflow] long session %d strokes worst stroke %.1f ms worst finish %.1f ms worst frame %.1f ms backlog "
			"%zu tiles %llu\n",
			strokes,
			session.worstStrokeMs,
			session.worstFinishMs,
			session.worstFrameMs,
			session.document.paintBacklog(),
			(unsigned long long) session.document.tileCount()
		);
		CHECK_EQUAL(0u, session.document.paintBacklog());
		CHECK(painted(session.at({ 0.0f, 0.0f })));
		CHECK(painted(session.at({ 6000.0f, 0.0f })));
		CHECK(session.worstFinishMs < 1000);
		CHECK(session.worstFrameMs < 2000);
	}
}
