#include <algorithm>
#include "BoxLayout.hpp"
#include "Button.hpp"
#include "Containers.hpp"
#include "Event.hpp"
#include "FloatingPanel.hpp"
#include "Label.hpp"
#include "Menus.hpp"
#include "Probes.hpp"
#include "Slider.hpp"
#include "StackLayout.hpp"
#include "Toggles.hpp"
#include "ScrollPhysics.hpp"
#include "UiDriver.hpp"
#include "Views.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cmath>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	struct Column {
		Widget root;

		Column() { root.emplaceLayout<ColumnLayout>(10).setPadding(Insets::uniform(10)); }

		template <typename... Widgets>
		explicit Column(Widgets&... widgets) :
			Column() {
			(root.addChild(widgets), ...);
		}
	};

	halnf monospaceWidth(const std::string& text, halnf fontSize) {
		return MonospaceTextMetrics().textWidth(text, fontSize);
	}
}

SUITE(LabelWidget) {
	TEST(ReservedTextKeepsTheWidthWhenTheTextChanges) {
		Label label("Short");
		label.setReservedText("A much longer text");
		Column scene(label);
		scene.root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Start);
		UiDriver driver(scene.root);
		const Metrics& metrics = driver.ui().theme().metrics;
		const halnf reserved = monospaceWidth("A much longer text", metrics.fontSize);

		CHECK_CLOSE(reserved, label.size().x, 0.01f);
		label.setText("Tiny");
		driver.settle();
		CHECK_CLOSE(reserved, label.size().x, 0.01f);
		label.setText("Longer than the reserved text");
		driver.settle();
		CHECK_CLOSE(monospaceWidth("Longer than the reserved text", metrics.fontSize), label.size().x, 0.01f);
	}

	TEST(MeasuresTextAndPaintsIt) {
		Label label("Hello");
		Column scene(label);
		scene.root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Start);
		UiDriver driver(scene.root);
		const Metrics& metrics = driver.ui().theme().metrics;

		CHECK_CLOSE(monospaceWidth("Hello", metrics.fontSize), label.size().x, 0.01f);
		CHECK_CLOSE(metrics.lineHeight, label.size().y, 0.01f);
		CHECK(driver.paint().hasText("Hello"));
	}

	TEST(ChangingTextRelayouts) {
		Label label("a");
		Column scene(label);
		scene.root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Start);
		UiDriver driver(scene.root);

		label.setText("a much longer text");
		driver.layout();

		CHECK_CLOSE(monospaceWidth("a much longer text", driver.ui().theme().metrics.fontSize), label.size().x, 0.01f);
	}
}

SUITE(ButtonWidget) {
	TEST(ClickInvokesActionOnce) {
		int clicks = 0;
		Button button("Press", [&clicks]() { clicks++; });
		Column scene(button);
		UiDriver driver(scene.root);

		driver.click(button);

		CHECK_EQUAL(1, clicks);
	}

	TEST(ReleasingOutsideCancelsClick) {
		int clicks = 0;
		Button button("Press", [&clicks]() { clicks++; });
		Column scene(button);
		UiDriver driver(scene.root);

		driver.moveTo(button);
		driver.press();
		driver.moveTo({ 700, 500 });
		driver.release();

		CHECK_EQUAL(0, clicks);
	}

	TEST(DisabledButtonIgnoresClicks) {
		int clicks = 0;
		Button button("Press", [&clicks]() { clicks++; });
		button.setEnabled(false);
		Column scene(button);
		UiDriver driver(scene.root);

		driver.click(button);

		CHECK_EQUAL(0, clicks);
	}

	TEST(KeyboardActivationAfterFocus) {
		int clicks = 0;
		Button button("Press", [&clicks]() { clicks++; });
		Column scene(button);
		UiDriver driver(scene.root);

		driver.click(button);
		driver.key(InputID::ENTER);
		driver.key(InputID::SPACE);

		CHECK_EQUAL(3, clicks);
	}

	TEST(HoverAnimatesBackgroundAndSettles) {
		Button button("Hover me");
		Column scene(button);
		UiDriver driver(scene.root);
		const Palette& palette = driver.ui().theme().palette;

		driver.moveTo(button);
		driver.advance(16);
		CHECK(driver.ui().needsFrame());

		driver.settle();
		const PaintCommand& background = driver.paint().commands().front();
		CHECK_CLOSE(palette.controlHovered.r, background.color.r, 0.01f);
		CHECK(!driver.ui().needsFrame());
	}

	TEST(MeasuresTextPlusPadding) {
		Button button("Wide label");
		Column scene(button);
		scene.root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Start);
		UiDriver driver(scene.root);
		const Metrics& metrics = driver.ui().theme().metrics;

		CHECK_CLOSE(
			monospaceWidth("Wide label", metrics.fontSize) + metrics.controlPadding.total(Axis::Horizontal),
			button.size().x,
			0.01f
		);
		const halnf contentHeight = metrics.lineHeight + metrics.controlPadding.total(Axis::Vertical);
		CHECK_CLOSE(contentHeight > metrics.controlHeight ? contentHeight : metrics.controlHeight, button.size().y, 0.01f);
	}
}

SUITE(ToggleWidgets) {
	TEST(CheckboxTogglesOnClickAndReportsChanges) {
		Checkbox checkbox("Enable");
		std::vector<bool> changes;
		checkbox.setOnChanged([&changes](bool checked) { changes.push_back(checked); });
		Column scene(checkbox);
		UiDriver driver(scene.root);

		driver.click(checkbox);
		CHECK(checkbox.isChecked());
		driver.click(checkbox);
		CHECK(!checkbox.isChecked());

		REQUIRE CHECK_EQUAL(2u, changes.size());
		CHECK(changes[0]);
		CHECK(!changes[1]);
	}

	TEST(SetCheckedProgrammaticallyAnimatesIndicator) {
		Checkbox checkbox("Enable");
		Column scene(checkbox);
		UiDriver driver(scene.root);
		driver.settle();

		checkbox.setChecked(true);
		CHECK(driver.ui().needsFrame());
		driver.settle();

		bool drewCheckMark = false;
		for (const PaintCommand& command : driver.paint().commands()) {
			drewCheckMark = drewCheckMark || command.kind == PaintCommand::Kind::Polyline;
		}
		CHECK(drewCheckMark);
	}

	TEST(SwitchToggles) {
		Switch toggle("Dark mode");
		Column scene(toggle);
		UiDriver driver(scene.root);

		driver.click(toggle);

		CHECK(toggle.isChecked());
	}

	TEST(ClickingLabelAreaToggles) {
		Checkbox checkbox("A long checkbox label");
		Column scene(checkbox);
		scene.root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Start);
		UiDriver driver(scene.root);

		driver.click(checkbox.windowRect().pos + Vec2F(checkbox.size().x - 4, checkbox.size().y / 2));

		CHECK(checkbox.isChecked());
	}

	TEST(RadioGroupKeepsExactlyOneSelected) {
		RadioButton small("Small");
		RadioButton medium("Medium");
		RadioButton large("Large");
		RadioGroup group;
		group.add(small);
		group.add(medium);
		group.add(large);
		std::vector<size_t> selections;
		group.setOnChanged([&selections](size_t index) { selections.push_back(index); });
		Column scene(small, medium, large);
		UiDriver driver(scene.root);

		driver.click(medium);
		CHECK_EQUAL(1, group.selectedIndex());

		driver.click(large);
		CHECK(!medium.isChecked());
		CHECK(large.isChecked());
		CHECK_EQUAL(2, group.selectedIndex());

		driver.click(large);
		CHECK(large.isChecked());

		CHECK_EQUAL(2u, selections.size());
	}

	TEST(RadioGroupSurvivesButtonDestruction) {
		RadioGroup group;
		{
			RadioButton temporary("temp");
			group.add(temporary);
			group.selectIndex(0);
		}
		CHECK_EQUAL(-1, group.selectedIndex());
	}
}

SUITE(SliderWidget) {
	TEST(NearbyTickMarksAreGroupedSoTheyNeverCrowd) {
		const std::vector<halnf> grouped = groupTickMarks({ 0.9f, 0.1f, 0.101f, 0.5f, 0.505f, 1.4f }, 0.02f);
		REQUIRE CHECK_EQUAL(4u, grouped.size());
		CHECK_CLOSE(0.1f, grouped[0], 0.0001f);
		CHECK_CLOSE(0.5f, grouped[1], 0.0001f);
		CHECK_CLOSE(0.9f, grouped[2], 0.0001f);
		CHECK_CLOSE(1.0f, grouped[3], 0.0001f);
	}

	TEST(TickMarksArePaintedAcrossTheTrack) {
		Slider slider(0, 1, 0);
		slider.setSizing(AxisSizing::fixed(214), AxisSizing::fit());
		slider.setTickMarks({ 0.0f, 0.5f, 0.5001f, 1.0f });
		Column scene(slider);
		UiDriver driver(scene.root, { 400, 100 });

		CHECK_EQUAL(3u, slider.visibleTickMarks().size());
		size_t ticks = 0;
		for (const PaintCommand& command : driver.paint().commands())
			if (command.kind == PaintCommand::Kind::Line && command.rect.size.x < 0.5f && command.rect.size.y > 1) ticks++;
		CHECK_EQUAL(3u, ticks);
	}

	TEST(DraggingSetsValueAndCapturesPointer) {
		Slider slider(0, 100, 0);
		std::vector<halnf> changes;
		slider.setOnChanged([&changes](halnf value) { changes.push_back(value); });
		Column scene(slider);
		UiDriver driver(scene.root, { 220, 100 });

		const RectF rect = slider.windowRect();
		const halnf inset = driver.ui().theme().metrics.sliderHandleSize / 2;

		driver.moveTo({ rect.pos.x + inset, rect.center().y });
		driver.press();
		CHECK(slider.isDragging());
		CHECK(driver.ui().pointerCapture() == &slider);

		driver.moveTo({ rect.pos.x + rect.size.x / 2, rect.center().y + 200 });
		CHECK_CLOSE(50, slider.value(), 1);

		driver.moveTo({ rect.pos.x + rect.size.x + 500, rect.center().y });
		CHECK_CLOSE(100, slider.value(), 0.01f);

		driver.release();
		CHECK(!slider.isDragging());
		CHECK(!changes.empty());
	}

	TEST(DraggingIsReportedOnPressAndRelease) {
		Slider slider(0, 100, 0);
		std::vector<bool> states;
		slider.setOnDragging([&states](bool dragging) { states.push_back(dragging); });
		Column scene(slider);
		UiDriver driver(scene.root, { 220, 100 });

		driver.drag(driver.centerOf(slider), driver.centerOf(slider) + Vec2F(30, 0));
		REQUIRE CHECK_EQUAL(2u, states.size());
		CHECK(states[0]);
		CHECK(!states[1]);
	}

	TEST(VerticalSliderGrowsUpwardsAndIsTall) {
		Slider slider(0, 100, 0);
		slider.setOrientation(Axis::Vertical);
		slider.setSizing(AxisSizing::fit(), AxisSizing::fixed(300));
		Column scene(slider);
		UiDriver driver(scene.root, { 220, 400 });

		const RectF rect = slider.windowRect();
		CHECK(rect.size.y > 250);
		const halnf inset = driver.ui().theme().metrics.sliderHandleSize / 2;

		driver.moveTo({ rect.center().x, rect.pos.y + rect.size.y - inset });
		driver.press();
		CHECK_CLOSE(0, slider.value(), 0.5f);
		driver.moveTo({ rect.center().x + 200, rect.pos.y + rect.size.y / 2 });
		CHECK_CLOSE(50, slider.value(), 1);
		driver.moveTo({ rect.center().x, rect.pos.y - 500 });
		CHECK_CLOSE(100, slider.value(), 0.01f);
		driver.release();

		slider.setValue(25);
		const Vec2F point = slider.windowPointForValue(25);
		CHECK_CLOSE(rect.center().x, point.x, 0.5f);
		CHECK_CLOSE(rect.pos.y + inset + (rect.size.y - 2 * inset) * 0.75f, point.y, 0.5f);
	}

	TEST(StepSnapsValues) {
		Slider slider(0, 10, 0);
		slider.setStep(2.5f);
		slider.setValue(3.4f);
		CHECK_CLOSE(2.5f, slider.value(), 0.001f);
		slider.setValue(4);
		CHECK_CLOSE(5, slider.value(), 0.001f);
	}

	TEST(KeyboardSteps) {
		Slider slider(0, 10, 5);
		slider.setStep(1);
		Column scene(slider);
		UiDriver driver(scene.root);
		driver.click(slider);
		const halnf clicked = slider.value();

		driver.key(InputID::RIGHT);
		CHECK_CLOSE(clicked + 1, slider.value(), 0.001f);
		driver.key(InputID::LEFT);
		driver.key(InputID::LEFT);
		CHECK_CLOSE(clicked - 1, slider.value(), 0.001f);
	}

	TEST(LabeledSliderShowsFormattedValue) {
		LabeledSlider labeled("Opacity", 0, 1, 0.25f);
		labeled.setValueFormatter([](halnf value) { return std::to_string((int) std::round(value * 100)) + "%"; });
		Column scene(labeled);
		UiDriver driver(scene.root);

		CHECK(driver.paint().hasText("Opacity"));
		CHECK(driver.paint().hasText("25%"));

		labeled.setValue(0.5f);
		CHECK(driver.paint().hasText("50%"));
	}

	TEST(ProgressBarAnimatesToFraction) {
		ProgressBar progress;
		Column scene(progress);
		UiDriver driver(scene.root, { 220, 100 });

		progress.setFraction(0.5f);
		driver.settle();

		const auto& commands = driver.paint().commands();
		REQUIRE CHECK_EQUAL(2u, commands.size());
		CHECK_CLOSE(progress.size().x * 0.5f, commands[1].rect.size.x, 0.5f);
	}
}

SUITE(StructuralWidgets) {
	TEST(SpacerPushesSiblingsApart) {
		Widget row;
		row.emplaceLayout<RowLayout>();
		Box left({ 50, 10 });
		Spacer spacer;
		Box right({ 50, 10 });
		row.addChild(left);
		row.addChild(spacer);
		row.addChild(right);
		UiDriver driver(row, { 400, 40 });

		CHECK_CLOSE(350, right.rect().pos.x, 0.01f);
	}

	TEST(SeparatorIsThinAndNotInteractive) {
		Box above({ 10, 10 });
		Separator separator;
		Box below({ 10, 10 });
		Column scene(above, separator, below);
		UiDriver driver(scene.root);

		const halnf thickness = driver.ui().theme().metrics.separatorThickness + driver.ui().theme().metrics.spacing;
		CHECK_CLOSE(thickness, separator.size().y, 0.01f);
		CHECK(driver.ui().widgetAt(driver.centerOf(separator)) != &separator);
	}

	TEST(PanelPaintsSurface) {
		Panel panel(Panel::Surface::Raised);
		panel.setSizing(AxisSizing::fixed(100), AxisSizing::fixed(50));
		Column scene(panel);
		UiDriver driver(scene.root);

		const auto& commands = driver.paint().commands();
		REQUIRE CHECK(!commands.empty());
		CHECK_CLOSE(driver.ui().theme().palette.raisedSurface.r, commands.front().color.r, 0.001f);
	}
}

SUITE(ScrollViewWidget) {
	struct ScrollScene {
		Widget root;
		ScrollView scroll;
		Widget list;
		Box rows[20];

		ScrollScene() {
			root.emplaceLayout<StackLayout>();
			list.emplaceLayout<ColumnLayout>();
			for (Box& row : rows) {
				row.setPreferred({ 100, 50 });
				list.addChild(row);
			}
			scroll.setContent(&list);
			root.addChild(scroll);
		}
	};

	TEST(FingerDragScrollsOnlyInTouchMode) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.drag(Vec2F(50.0f, 250.0f), Vec2F(50.0f, 50.0f), 8);
		driver.settle();
		CHECK_CLOSE(0.0f, scene.scroll.offset(), 0.5f);

		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.drag(Vec2F(50.0f, 250.0f), Vec2F(50.0f, 50.0f), 8);
		driver.settle();
		CHECK(scene.scroll.offset() > 100.0f);
	}

	TEST(FingerDragScrollsContentWithoutClickingIt) {
		ScrollScene scene;
		int clicked = 0;
		Button button{ "row", [&clicked]() { clicked++; } };
		button.setSizing(AxisSizing::fixed(100), AxisSizing::fixed(50));
		scene.list.insertChild(0, button);
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);

		driver.drag(Vec2F(50.0f, 25.0f), Vec2F(50.0f, -175.0f), 8);
		driver.settle();

		CHECK_CLOSE(200.0f, scene.scroll.offset(), 0.5f);
		CHECK_EQUAL(0, clicked);
	}

	TEST(FingerTapStillClicksInsideAScrollView) {
		ScrollScene scene;
		int clicked = 0;
		Button button{ "row", [&clicked]() { clicked++; } };
		button.setSizing(AxisSizing::fixed(100), AxisSizing::fixed(50));
		scene.list.insertChild(0, button);
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);

		driver.drag(Vec2F(50.0f, 25.0f), Vec2F(52.0f, 28.0f), 2);

		CHECK_EQUAL(1, clicked);
		CHECK_CLOSE(0.0f, scene.scroll.offset(), 0.01f);
	}

	TEST(QuickFingerSwipeKeepsScrollingAfterRelease) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.setAutoSettle(false);

		driver.swipe(Vec2F(50.0f, 250.0f), Vec2F(50.0f, 150.0f), 80);
		const halnf released = scene.scroll.currentOffset();
		CHECK((scene.scroll.flingVelocity() != 0));
		CHECK(scene.scroll.flingVelocity() > 0.5f);
		driver.settle();

		CHECK(scene.scroll.currentOffset() > released + 50);
		CHECK(!(scene.scroll.flingVelocity() != 0));
		CHECK(!driver.ui().needsFrame());
	}

	TEST(FlingSlowsDownAndStopsAtTheEnd) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.setAutoSettle(false);

		driver.swipe(Vec2F(50.0f, 290.0f), Vec2F(50.0f, 10.0f), 40);
		driver.settle();

		CHECK_CLOSE(scene.scroll.maximumOffset(), scene.scroll.currentOffset(), 0.5f);
		CHECK(!(scene.scroll.flingVelocity() != 0));
	}

	TEST(FlingFollowsTheAndroidSplineDistance) {
		const FlingSpline spline;
		const halnf expected =
			(halnf) (FlingSpline::kScrollFriction * 9.80665 * 39.37 * 160 * 0.84 *
							 std::exp(
								 std::log(0.78) / std::log(0.9) / (std::log(0.78) / std::log(0.9) - 1) *
								 std::log(0.35 * 1000 / (FlingSpline::kScrollFriction * 9.80665 * 39.37 * 160 * 0.84))
							 ));
		CHECK_CLOSE(expected, spline.distanceFor(1000), 0.01f);
		CHECK(spline.distanceFor(2000) > spline.distanceFor(1000) * 1.5f);
		CHECK(spline.durationFor(2000) > spline.durationFor(1000));
		CHECK_CLOSE(0.0f, FlingSpline::splinePosition(0), 1e-4f);
		CHECK_CLOSE(1.0f, FlingSpline::splinePosition(1), 1e-6f);
		for (int4 step = 1; step <= 100; step++)
			CHECK(FlingSpline::splinePosition((halnf) step / 100) >= FlingSpline::splinePosition((halnf) (step - 1) / 100));
	}

	TEST(FlingTravelsTheSplineDistanceAndStops) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.setAutoSettle(false);
		driver.swipe(Vec2F(50.0f, 250.0f), Vec2F(50.0f, 230.0f), 40);
		REQUIRE CHECK(scene.scroll.fling().isActive());
		const halnf target = scene.scroll.fling().finalPosition();
		driver.settle();
		CHECK_CLOSE(target, scene.scroll.currentOffset(), 1.0f);
		CHECK(!scene.scroll.fling().isActive());
		CHECK(!driver.ui().needsFrame());
	}

	TEST(FlingIntoTheEndStretchesAndSpringsBack) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.setAutoSettle(false);
		driver.swipe(Vec2F(50.0f, 290.0f), Vec2F(50.0f, 10.0f), 40);
		halnf furthest = 0;
		for (int frame = 0; frame < 400; frame++) {
			driver.advance(16);
			furthest = std::max(furthest, -scene.scroll.stretchOffset());
		}
		CHECK(furthest > 1);
		CHECK(furthest <= (StretchEffect::kLinearIntensity + StretchEffect::kExponentialIntensity) * 300 + 0.5f);
		CHECK_CLOSE(scene.scroll.maximumOffset(), scene.scroll.currentOffset(), 0.5f);
		CHECK_CLOSE(0.0f, scene.scroll.stretchOffset(), 0.01f);
		CHECK(!scene.scroll.isBouncing());
	}

	TEST(StretchSpringUsesTheAndroidConstants) {
		StretchEffect edge;
		edge.setLength(1000);
		edge.absorb(2000);
		halnf peak = 0;
		halnf elapsed = 0;
		halnf peakTime = 0;
		while (edge.advance(1)) {
			elapsed += 1;
			if (edge.distance() > peak) {
				peak = edge.distance();
				peakTime = elapsed;
			}
		}
		const double omega = StretchEffect::kNaturalFrequency;
		const double zeta = StretchEffect::kDampingRatio;
		const double damped = omega * std::sqrt(1 - zeta * zeta);
		const double expectedPeakTime = std::atan(damped / (zeta * omega)) / damped * 1000.0;
		CHECK_CLOSE((halnf) expectedPeakTime, peakTime, 4.0f);
		CHECK(peak > 0.01f);
		CHECK(elapsed < 1000);
		CHECK_CLOSE(0.0f, edge.distance(), 1e-6f);
	}

	TEST(StretchIntensityMatchesTheAndroidCurve) {
		CHECK_CLOSE(0.0f, StretchEffect::stretchIntensity(0), 1e-6f);
		const halnf one = StretchEffect::stretchIntensity(1);
		CHECK_CLOSE((halnf) (0.016 + 0.016 * (1 - std::exp(-2.718281828 / 0.33))), one, 1e-5f);
		CHECK(StretchEffect::stretchIntensity(0.1f) > 0.1f * one);
	}

	TEST(DraggingPastTheStartStretchesAndReleaseRecoils) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.setAutoSettle(false);
		driver.moveTo(Vec2F(50.0f, 50.0f));
		driver.press();
		for (int step = 1; step <= 10; step++) {
			driver.advance(30);
			driver.moveTo(Vec2F(50.0f, 50.0f + 15.0f * (halnf) step));
		}
		CHECK_CLOSE(0.0f, scene.scroll.currentOffset(), 0.01f);
		CHECK(scene.scroll.startStretch().isPulling());
		CHECK_CLOSE(150.0f / 300.0f, scene.scroll.startStretch().distance(), 0.02f);
		const halnf stretched = scene.scroll.stretchOffset();
		CHECK(stretched > 0);
		CHECK(stretched < 150);

		driver.moveTo(Vec2F(50.0f, 80.0f));
		CHECK(scene.scroll.startStretch().distance() < 0.2f);
		CHECK_CLOSE(0.0f, scene.scroll.currentOffset(), 0.01f);
		driver.release();
		driver.settle();
		CHECK_CLOSE(0.0f, scene.scroll.stretchOffset(), 0.01f);
		CHECK(!scene.scroll.fling().isActive());
	}

	TEST(PullingBackFromAStretchScrollsAgain) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.setAutoSettle(false);
		driver.moveTo(Vec2F(50.0f, 100.0f));
		driver.press();
		driver.advance(30);
		driver.moveTo(Vec2F(50.0f, 160.0f));
		driver.advance(30);
		driver.moveTo(Vec2F(50.0f, 40.0f));
		CHECK_CLOSE(0.0f, scene.scroll.startStretch().distance(), 1e-4f);
		CHECK_CLOSE(60.0f - 8.0f, scene.scroll.currentOffset(), 8.5f);
		driver.release();
	}

	TEST(TheSplineDeceleratesLikeAndroid) {
		CHECK(FlingSpline::splinePosition(0.1f) > 0.2f);
		CHECK(FlingSpline::splinePosition(0.25f) > 0.45f);
		CHECK(FlingSpline::splinePosition(0.5f) > 0.75f);
		CHECK(FlingSpline::splinePosition(0.9f) > 0.99f);
	}

	TEST(StretchNeverExceedsOneViewport) {
		StretchEffect edge;
		edge.setLength(100);
		const halnf consumed = edge.pull(500);
		CHECK_CLOSE(1.0f, edge.distance(), 1e-6f);
		CHECK_CLOSE(100.0f, consumed, 1e-3f);
		CHECK_CLOSE(StretchEffect::stretchIntensity(1) * 100, edge.visualOffset(), 1e-3f);
	}

	TEST(StretchMovesTheContent) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.setAutoSettle(false);
		const halnf restingTop = scene.list.windowRect().pos.y;
		driver.moveTo(Vec2F(50.0f, 50.0f));
		driver.press();
		for (int step = 1; step <= 6; step++) {
			driver.advance(30);
			driver.moveTo(Vec2F(50.0f, 50.0f + 25.0f * (halnf) step));
		}
		driver.layout();
		CHECK_CLOSE(restingTop + scene.scroll.stretchOffset(), scene.list.windowRect().pos.y, 1.0f);
		CHECK(scene.list.windowRect().pos.y > restingTop + 1);
		driver.release();
	}

	TEST(ReleasingWhileStretchedDoesNotFling) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.setAutoSettle(false);
		scene.scroll.jumpTo(40);
		driver.moveTo(Vec2F(50.0f, 50.0f));
		driver.press();
		for (int step = 1; step <= 4; step++) {
			driver.advance(15);
			driver.moveTo(Vec2F(50.0f, 50.0f + 40.0f * (halnf) step));
		}
		REQUIRE CHECK(scene.scroll.startStretch().distance() > 0);
		driver.release();
		CHECK(!scene.scroll.fling().isActive());
		driver.settle();
		CHECK_CLOSE(0.0f, scene.scroll.currentOffset(), 0.01f);
		CHECK_CLOSE(0.0f, scene.scroll.stretchOffset(), 0.01f);
	}

	TEST(TouchingABounceStopsItInRange) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.setAutoSettle(false);
		driver.swipe(Vec2F(50.0f, 290.0f), Vec2F(50.0f, 10.0f), 40);
		for (int frame = 0; frame < 400 && !scene.scroll.isBouncing(); frame++)
			driver.advance(4);
		REQUIRE CHECK(scene.scroll.isBouncing());
		driver.advance(16);
		driver.moveTo(Vec2F(50.0f, 150.0f));
		driver.press();
		CHECK(!scene.scroll.fling().isActive());
		CHECK(scene.scroll.currentOffset() <= scene.scroll.maximumOffset() + 0.5f);
		driver.release();
		driver.settle();
		CHECK_CLOSE(0.0f, scene.scroll.stretchOffset(), 0.01f);
	}

	TEST(SlowDragDoesNotFling) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.setAutoSettle(false);

		driver.swipe(Vec2F(50.0f, 250.0f), Vec2F(50.0f, 200.0f), 2000, 20);
		driver.advance(200);

		CHECK(!(scene.scroll.flingVelocity() != 0));
		CHECK_CLOSE(50.0f, scene.scroll.currentOffset(), 0.5f);
	}

	TEST(FlingUsesTheAverageSpeedOfTheLastHundredMilliseconds) {
		using Sample = ScrollView::DragSample;
		const std::vector<Sample> steady = { { 0, 0 }, { 20, 20 }, { 40, 40 }, { 60, 60 }, { 80, 80 } };
		CHECK_CLOSE(1.0f, ScrollView::averageVelocity(steady, 80), 1e-4f);
		const std::vector<Sample> jitter = { { 0, 0 }, { 20, 20 }, { 40, 40 }, { 60, 60 }, { 80, 80 }, { 82, 100 } };
		CHECK_CLOSE(100.0f / 82.0f, ScrollView::averageVelocity(jitter, 82), 1e-3f);
		const std::vector<Sample> old = { { 0, 0 }, { 50, 100 }, { 200, 120 }, { 250, 220 } };
		CHECK_CLOSE(2.0f, ScrollView::averageVelocity(old, 250), 1e-4f);
		CHECK_CLOSE(0.0f, ScrollView::averageVelocity(old, 400), 1e-6f);
		CHECK_CLOSE(0.0f, ScrollView::averageVelocity({ { 10, 5 } }, 10), 1e-6f);
	}

	TEST(ASuddenLastMoveDoesNotDominateTheFling) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.setAutoSettle(false);
		driver.moveTo(Vec2F(50.0f, 280.0f));
		driver.press();
		for (int step = 1; step <= 5; step++) {
			driver.advance(20);
			driver.moveTo(Vec2F(50.0f, 280.0f - 20.0f * (halnf) step));
		}
		driver.advance(2);
		driver.moveTo(Vec2F(50.0f, 160.0f));
		driver.release();
		CHECK(scene.scroll.flingVelocity() > 1.0f);
		CHECK(scene.scroll.flingVelocity() < 2.0f);
	}

	TEST(TouchingAFlingStopsItWithoutClicking) {
		ScrollScene scene;
		int clicked = 0;
		Button button{ "row", [&clicked]() { clicked++; } };
		button.setSizing(AxisSizing::fixed(100), AxisSizing::fixed(50));
		scene.list.insertChild(0, button);
		UiDriver driver(scene.root, { 200, 300 });
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.setAutoSettle(false);
		driver.swipe(Vec2F(50.0f, 250.0f), Vec2F(50.0f, 150.0f), 80);
		driver.advance(50);
		REQUIRE CHECK((scene.scroll.flingVelocity() != 0));

		driver.moveTo(Vec2F(50.0f, 100.0f));
		driver.press();
		const halnf caught = scene.scroll.currentOffset();
		driver.advance(200);
		driver.release();

		CHECK(!(scene.scroll.flingVelocity() != 0));
		CHECK_CLOSE(caught, scene.scroll.currentOffset(), 0.5f);
		CHECK_EQUAL(0, clicked);
	}

	TEST(MouseDragDoesNotScrollContent) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });

		driver.drag(Vec2F(50.0f, 200.0f), Vec2F(50.0f, 20.0f), 8);
		driver.settle();

		CHECK_CLOSE(0.0f, scene.scroll.offset(), 0.01f);
	}

	TEST(ScrollBarIsHiddenWhileHoveringContent) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.settle();

		driver.moveTo(Vec2F(60.0f, 150.0f));
		driver.settle();

		CHECK_CLOSE(0, scene.scroll.scrollBarReveal(), 0.01f);
		CHECK(!driver.ui().needsFrame());
	}

	TEST(ScrollBarAppearsWhenPointerIsOverItAndHidesWhenItLeaves) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.settle();
		const RectF bar = scene.scroll.scrollBar().windowRect();
		CHECK_CLOSE(200 - driver.ui().theme().metrics.scrollBarThickness, bar.pos.x, 0.01f);

		driver.moveTo(bar.center());
		driver.settle();
		CHECK_CLOSE(1, scene.scroll.scrollBarReveal(), 0.01f);

		driver.moveTo(Vec2F(60.0f, 150.0f));
		driver.settle();
		CHECK_CLOSE(0, scene.scroll.scrollBarReveal(), 0.01f);
		CHECK(!driver.ui().needsFrame());
	}

	TEST(ScrollBarThumbGrowsOutOfTheTrackEdge) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.settle();
		ScrollBar& bar = scene.scroll.scrollBar();
		const RectF thumbTarget = translated(bar.thumbRect(), bar.windowPosition());
		const auto thumbFill = [&driver, &thumbTarget]() {
			RectF found = makeRect(0, 0, 0, 0);
			for (const PaintCommand& command : driver.paint().commands()) {
				const bool onThumb = command.kind == PaintCommand::Kind::FillRect &&
														 std::abs(command.rect.pos.y - thumbTarget.pos.y) < 0.01f &&
														 std::abs(command.rect.size.y - thumbTarget.size.y) < 0.01f;
				if (onThumb) found = command.rect;
			}
			return found;
		};

		driver.moveTo(bar.windowRect().center());
		driver.advance(32);
		const RectF growing = thumbFill();

		driver.settle();
		const RectF full = thumbFill();

		CHECK(growing.size.x > 0);
		CHECK(growing.size.x < full.size.x);
		CHECK_CLOSE(full.pos.x + full.size.x, growing.pos.x + growing.size.x, 0.01f);
	}

	TEST(ScrollTrackIsThinAndKeepsClearOfTheEdge) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		ScrollBar& bar = scene.scroll.scrollBar();
		const Metrics& metrics = driver.ui().theme().metrics;
		const RectF track = translated(bar.trackRect(), bar.windowPosition());

		CHECK_CLOSE(metrics.scrollTrackThickness, track.size.x, 0.01f);
		CHECK_CLOSE(200 - metrics.scrollTrackEdgeGap, track.pos.x + track.size.x, 0.01f);
		CHECK(track.pos.y > 0);
		CHECK(track.pos.y + track.size.y < 300);
	}

	TEST(TrackIsPaintedWithTheThumbWhenShown) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		ScrollBar& bar = scene.scroll.scrollBar();
		driver.moveTo(bar.windowRect().center());
		driver.settle();

		const RectF track = translated(bar.trackRect(), bar.windowPosition());
		bool trackPainted = false;
		for (const PaintCommand& command : driver.paint().commands()) {
			trackPainted = trackPainted || (command.kind == PaintCommand::Kind::FillRect &&
																			std::abs(command.rect.size.y - track.size.y) < 0.01f);
		}
		CHECK(trackPainted);
	}

	TEST(ClickingTheTrackScrollsThereAndDragContinues) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.settle();
		ScrollBar& bar = scene.scroll.scrollBar();
		const RectF track = translated(bar.trackRect(), bar.windowPosition());

		driver.moveTo(Vec2F(track.center().x, track.pos.y + track.size.y * 0.75f));
		driver.press();
		driver.settle();
		const halnf afterClick = scene.scroll.offset();
		CHECK(afterClick > scene.scroll.maximumOffset() * 0.5f);

		driver.moveTo(Vec2F(track.center().x, track.pos.y + track.size.y - 1));
		driver.settle();
		CHECK_CLOSE(scene.scroll.maximumOffset(), scene.scroll.offset(), 1);
		driver.release();
	}

	TEST(ScrollBarFlashesBrieflyAfterScrolling) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.moveTo(Vec2F(60.0f, 150.0f));
		driver.settle();

		driver.wheel(-1);
		driver.advance(150);
		CHECK(scene.scroll.scrollBarReveal() > 0.5f);

		driver.settle();
		CHECK_CLOSE(0, scene.scroll.scrollBarReveal(), 0.01f);
		CHECK(scene.scroll.offset() > 0);
	}

	TEST(DraggingThumbScrollsAndKeepsBarVisibleOutsideIt) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		driver.settle();
		ScrollBar& bar = scene.scroll.scrollBar();
		const Vec2F thumb = translated(bar.thumbRect(), bar.windowPosition()).center();

		driver.moveTo(thumb);
		driver.press();
		driver.moveTo(Vec2F(40.0f, thumb.y + 120));
		driver.settle();

		CHECK(scene.scroll.offset() > 0);
		CHECK(bar.isDragging());
		CHECK_CLOSE(1, scene.scroll.scrollBarReveal(), 0.01f);

		driver.release();
		driver.settle();
		CHECK_CLOSE(0, scene.scroll.scrollBarReveal(), 0.01f);
	}

	TEST(ContentStillGetsClicksUnderTheHiddenBarArea) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });

		CHECK(driver.ui().widgetAt(Vec2F(60.0f, 25.0f)) == &scene.rows[0]);
	}

	TEST(ScrollBarStaysHiddenWhenNothingToScroll) {
		Widget root;
		root.emplaceLayout<StackLayout>();
		ScrollView scroll;
		Box small({ 50, 50 });
		scroll.setContent(&small);
		root.addChild(scroll);
		UiDriver driver(root, { 200, 300 });

		driver.moveTo(Vec2F(100.0f, 100.0f));
		driver.settle();

		CHECK_CLOSE(0, scroll.scrollBarReveal(), 0.01f);
	}

	TEST(ContentLongerThanViewportIsScrollable) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });

		CHECK(scene.scroll.isScrollable());
		CHECK_CLOSE(1000, scene.scroll.contentLength(), 0.01f);
		CHECK_CLOSE(700, scene.scroll.maximumOffset(), 0.01f);
	}

	TEST(WheelScrollsWithAnimationAndClamps) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });

		driver.moveTo({ 50, 50 });
		driver.wheel(-2);
		driver.settle();

		CHECK_CLOSE(96, scene.scroll.offset(), 0.01f);
		CHECK_CLOSE(-96, scene.rows[0].windowRect().pos.y, 0.5f);

		for (int step = 0; step < 50; step++)
			driver.wheel(-1);
		driver.settle();
		CHECK_CLOSE(700, scene.scroll.offset(), 0.01f);

		driver.wheel(100);
		driver.settle();
		CHECK_CLOSE(0, scene.scroll.offset(), 0.01f);
	}

	TEST(HitTestingFollowsScrolledContent) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });

		scene.scroll.scrollTo(500);
		driver.settle();

		CHECK(driver.ui().widgetAt({ 50, 10 }) == &scene.rows[10]);
	}

	TEST(DraggingThumbScrolls) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		const halnf thumbX = 200 - driver.ui().theme().metrics.scrollBarThickness / 2;

		driver.drag(Vec2F(thumbX, 20.0f), Vec2F(thumbX, 300.0f));

		CHECK_CLOSE(700, scene.scroll.offset(), 1);
	}

	TEST(ShortContentIsNotScrollable) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 2000 });

		CHECK(!scene.scroll.isScrollable());
		driver.moveTo({ 50, 50 });
		driver.wheel(-3);
		CHECK_CLOSE(0, scene.scroll.offset(), 0.01f);
	}

	TEST(ResizingViewportReclampsOffset) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });
		scene.scroll.scrollTo(700);
		driver.settle();

		driver.resize({ 200, 900 });
		driver.settle();

		CHECK_CLOSE(100, scene.scroll.offset(), 0.01f);
	}

	TEST(ContentIsClippedToViewport) {
		ScrollScene scene;
		UiDriver driver(scene.root, { 200, 300 });

		const auto& commands = driver.paint().commands();
		for (const PaintCommand& command : commands)
			CHECK(command.clip.pos.y + command.clip.size.y <= 300.01f);
	}
}

SUITE(SplitViewWidget) {
	TEST(PanesShareSpaceByRatioAroundHandle) {
		SplitView split(Axis::Horizontal);
		Box left;
		Box right;
		split.setFirst(&left);
		split.setSecond(&right);
		split.setRatio(0.25f);
		const halnf handle = Theme::standard().metrics.splitterThickness;
		UiDriver driver(split, Vec2F(400 + handle, 100.0f));

		CHECK_CLOSE(100, left.size().x, 0.01f);
		CHECK_CLOSE(100 + handle, right.rect().pos.x, 0.01f);
		CHECK_CLOSE(300, right.size().x, 0.01f);
	}

	TEST(DraggingHandleChangesRatioWithinLimits) {
		SplitView split(Axis::Vertical);
		Box top;
		Box bottom;
		split.setFirst(&top);
		split.setSecond(&bottom);
		split.setMinimumPaneLength(50);
		UiDriver driver(split, { 100, 406 });

		driver.drag(driver.centerOf(split), { 50, 300 });
		CHECK(split.ratio() > 0.6f);

		driver.drag(Vec2F(50.0f, split.handleRect().center().y), Vec2F(50.0f, 2000.0f));
		CHECK_CLOSE(350, top.size().y, 1);
	}

	TEST(HandleLineIsCenteredAndShowsOnlyOnHover) {
		SplitView split(Axis::Horizontal);
		Box left;
		Box right;
		split.setFirst(&left);
		split.setSecond(&right);
		UiDriver driver(split, { 400, 100 });
		driver.settle();

		const RectF handle = split.handle().windowRect();
		const RectF line = translated(split.handle().lineRect(), split.handle().windowPosition());
		CHECK_CLOSE(handle.center().x, line.center().x, 0.01f);
		CHECK_CLOSE(left.windowRect().pos.x + left.size().x, handle.pos.x, 0.01f);
		CHECK_CLOSE(0, split.handle().revealAmount(), 0.01f);

		driver.moveTo(handle.center());
		driver.settle();
		CHECK_CLOSE(1, split.handle().revealAmount(), 0.01f);

		driver.moveTo(Vec2F(20.0f, 50.0f));
		driver.settle();
		CHECK_CLOSE(0, split.handle().revealAmount(), 0.01f);
	}

	TEST(ClicksOutsideHandleReachPanes) {
		SplitView split(Axis::Horizontal);
		EventProbe left;
		EventProbe right;
		split.setFirst(&left);
		split.setSecond(&right);
		UiDriver driver(split, { 400, 100 });

		driver.click({ 20, 20 });

		CHECK_EQUAL(1, left.count(Event::Type::Click));
	}
}

SUITE(TabViewWidget) {
	TEST(OnlyCurrentPageIsVisibleAndClickingTabsSwitches) {
		TabView tabs;
		Label first("first page");
		Label second("second page");
		tabs.addTab("One", first);
		tabs.addTab("Two", second);
		UiDriver driver(tabs, { 400, 300 });

		CHECK_EQUAL(0, tabs.currentIndex());
		CHECK(first.isVisible());
		CHECK(!second.isVisible());
		CHECK(tabs.tabButton(0).isSelected());

		driver.click(tabs.tabButton(1));

		CHECK_EQUAL(1, tabs.currentIndex());
		CHECK(second.isVisible());
		CHECK(driver.paint().hasText("second page"));
		CHECK(!driver.paint().hasText("first page"));
	}

	TEST(CenteredTabsSitInTheMiddleOfTheTabStrip) {
		TabView tabs;
		Label first("first page");
		Label second("second page");
		tabs.addTab("One", first);
		tabs.addTab("Two", second);
		tabs.setTabAlignment(TabView::TabAlignment::Center);
		UiDriver driver(tabs, { 600, 300 });
		driver.settle();

		const RectF left = tabs.tabButton(0).windowRect();
		const RectF right = tabs.tabButton(1).windowRect();
		const halnf middle = (left.pos.x + right.pos.x + right.size.x) / 2;
		CHECK_CLOSE(300.0f, middle, 2.0f);
		CHECK(left.pos.x > 100);
	}

	TEST(PageSitsBelowTabsWithAGap) {
		TabView tabs;
		Label page("page");
		tabs.addTab("One", page);
		UiDriver driver(tabs, { 400, 300 });

		const RectF button = tabs.tabButton(0).windowRect();
		CHECK(page.windowRect().pos.y - (button.pos.y + button.size.y) >= 12.0f);
	}

	TEST(RemovingCurrentTabSelectsNeighbor) {
		TabView tabs;
		Box pages[3];
		tabs.addTab("A", pages[0]);
		tabs.addTab("B", pages[1]);
		tabs.addTab("C", pages[2]);
		UiDriver driver(tabs, { 400, 300 });
		tabs.setCurrentIndex(2);

		tabs.removeTab(pages[2]);

		CHECK_EQUAL(1, tabs.currentIndex());
		CHECK(pages[1].isVisible());
		CHECK_EQUAL(2u, tabs.tabCount());
	}
}

SUITE(CarouselTabViewWidget) {
	struct CarouselScene {
		TabView tabs{ TabView::Style::Carousel };
		Label pages[3] = { Label("first page"), Label("second page"), Label("third page") };
		UiDriver driver{ tabs, { 400, 300 } };

		CarouselScene() {
			tabs.addTab("One", pages[0]);
			tabs.addTab("Two", pages[1]);
			tabs.addTab("Three", pages[2]);
			driver.settle();
		}

		void trackpad(halnf horizontal) {
			driver.moveTo(pages[0].parent()->parent()->windowRect().center());
			driver.ui().dispatch(Event::wheel(driver.ui().pointerPosition(), 0, horizontal));
		}

		[[nodiscard]] bool shows(size_t index) { return driver.paint().hasText(pages[index].text()); }
	};

	TEST(ArrowButtonsSitAroundTheTabsAndCycleThroughPages) {
		CarouselScene scene;
		TabView& tabs = scene.tabs;
		CHECK(tabs.previousButton().windowRect().pos.x < tabs.tabButton(0).windowRect().pos.x);
		CHECK(tabs.nextButton().windowRect().pos.x > tabs.tabButton(2).windowRect().pos.x);

		scene.driver.click(tabs.previousButton());
		scene.driver.settle();
		CHECK_EQUAL(2, tabs.currentIndex());
		CHECK(scene.shows(2));
		CHECK(!scene.shows(0));

		scene.driver.click(tabs.nextButton());
		scene.driver.settle();
		CHECK_EQUAL(0, tabs.currentIndex());
		scene.driver.click(tabs.nextButton());
		scene.driver.settle();
		CHECK_EQUAL(1, tabs.currentIndex());
	}

	TEST(PlainTabViewHasNoArrowButtons) {
		TabView tabs;
		Label page("page");
		tabs.addTab("One", page);
		UiDriver driver(tabs, { 400, 300 });
		CHECK(tabs.previousButton().parent() == nullptr);
		CHECK(tabs.nextButton().parent() == nullptr);
	}

	TEST(SwitchingSlidesTheOldPageOutWhileTheNewPageComesInFromTheOtherSide) {
		CarouselScene scene;
		scene.driver.setAutoSettle(false);
		const RectF area = scene.pages[0].parent()->windowRect();

		scene.tabs.showNext();
		scene.driver.advance(48);
		scene.driver.ui().updateLayout();
		REQUIRE CHECK(scene.tabs.isSliding());
		const RectF outgoing = scene.pages[0].parent()->windowRect();
		const RectF incoming = scene.pages[1].parent()->windowRect();
		CHECK(outgoing.pos.x < area.pos.x);
		CHECK(incoming.pos.x > area.pos.x);
		CHECK_CLOSE(outgoing.pos.x + area.size.x, incoming.pos.x, 1.5f);

		scene.driver.settle();
		CHECK(!scene.tabs.isSliding());
		CHECK_CLOSE(area.pos.x, scene.pages[1].parent()->windowRect().pos.x, 0.5f);
		CHECK(!scene.pages[0].parent()->isVisible());
	}

	TEST(SelectionFrameSlidesBetweenTabsWithThePages) {
		CarouselScene scene;
		scene.driver.setAutoSettle(false);
		const RectF first = scene.tabs.tabButton(0).rect();
		const RectF second = scene.tabs.tabButton(1).rect();
		CHECK_CLOSE(first.pos.x, scene.tabs.selectionIndicatorRect().pos.x, 0.5f);

		scene.tabs.showNext();
		scene.driver.advance(48);
		const RectF moving = scene.tabs.selectionIndicatorRect();
		CHECK(moving.pos.x > first.pos.x + 1);
		CHECK(moving.pos.x < second.pos.x - 1);

		scene.driver.settle();
		CHECK_CLOSE(second.pos.x, scene.tabs.selectionIndicatorRect().pos.x, 0.5f);
	}

	TEST(UnhandledHorizontalWheelSwitchesPagesButAScrollingChildKeepsIt) {
		TabView tabs{ TabView::Style::Carousel };
		ScrollView scroller{ Axis::Horizontal };
		Box wide;
		wide.setSizing(AxisSizing::preferred(2000), AxisSizing::fill());
		scroller.setContent(&wide);
		Label plain("plain page");
		tabs.addTab("Scroller", scroller);
		tabs.addTab("Plain", plain);
		UiDriver driver(tabs, { 400, 300 });
		driver.settle();

		driver.moveTo(scroller.windowRect().center());
		driver.wheel(-1);
		driver.settle();
		CHECK(scroller.offset() > 0);
		CHECK_EQUAL(0, tabs.currentIndex());

		driver.horizontalWheel(-1);
		driver.settle();
		CHECK_EQUAL(1, tabs.currentIndex());

		driver.ui().dispatch(Event::wheel(plain.windowRect().center(), -1).withModifiers({ .shift = true }));
		driver.settle();
		CHECK_EQUAL(0, tabs.currentIndex());
	}

	TEST(TheSmallestSwipeMovesThePagesRightAway) {
		CarouselScene scene;
		scene.driver.setAutoSettle(false);
		const RectF area = scene.pages[0].parent()->windowRect();

		scene.trackpad(-0.1f);
		scene.driver.advance(16);
		CHECK(scene.tabs.scrollPosition() > 0);
		scene.driver.advance(48);
		scene.driver.ui().updateLayout();
		CHECK(scene.pages[0].parent()->windowRect().pos.x < area.pos.x);
		CHECK(scene.pages[1].parent()->isVisible());
	}

	TEST(ASmallSwipeResistsAndSpringsBackWithoutOvershoot) {
		CarouselScene scene;
		scene.driver.setAutoSettle(false);
		for (int step = 0; step < 3; step++) {
			scene.trackpad(-0.5f);
			scene.driver.advance(16);
		}
		const halnf pulled = 1.5f * TabView::kPagesPerTrackpadUnit;
		REQUIRE CHECK(scene.tabs.isPulling());
		CHECK(scene.tabs.scrollPosition() > 0);
		CHECK(scene.tabs.scrollPosition() < pulled);

		halnf lowest = scene.tabs.scrollPosition();
		for (int frame = 0; frame < 200 && scene.tabs.isSliding(); frame++) {
			scene.driver.advance(16);
			lowest = std::min(lowest, scene.tabs.scrollPosition());
		}
		CHECK(!scene.tabs.isSliding());
		CHECK(lowest >= -0.0005f);
		CHECK_EQUAL(0, scene.tabs.currentIndex());
		CHECK_CLOSE(0, scene.tabs.scrollPosition(), 0.0001f);
	}

	TEST(PullingPastTheCommitFractionMovesOnWithoutOvershoot) {
		CarouselScene scene;
		scene.driver.setAutoSettle(false);
		while (scene.tabs.currentIndex() == 0) {
			scene.trackpad(-0.5f);
			scene.driver.advance(16);
		}
		CHECK_EQUAL(1, scene.tabs.currentIndex());

		halnf furthest = scene.tabs.scrollPosition();
		for (int frame = 0; frame < 200 && scene.tabs.isSliding(); frame++) {
			scene.driver.advance(16);
			furthest = std::max(furthest, scene.tabs.scrollPosition());
		}
		CHECK(!scene.tabs.isSliding());
		CHECK(furthest <= 1.0005f);
		CHECK_EQUAL(1, scene.tabs.currentIndex());
		CHECK_CLOSE(1, scene.tabs.scrollPosition(), 0.0001f);
	}

	TEST(OneLongSwipePassesSeveralTabsAndRestsOnOne) {
		CarouselScene scene;
		scene.driver.setAutoSettle(false);
		std::vector<long> passed;
		for (int step = 0; step < 40; step++) {
			scene.trackpad(-1.05f);
			scene.driver.advance(16);
			if (passed.empty() || passed.back() != scene.tabs.currentIndex()) passed.push_back(scene.tabs.currentIndex());
		}
		CHECK(passed.size() >= 6);
		scene.driver.settle();
		CHECK(!scene.tabs.isSliding());
		CHECK_CLOSE(std::round(scene.tabs.scrollPosition()), scene.tabs.scrollPosition(), 0.001f);
	}

	TEST(SwipingBackUndoesTheStepItPassed) {
		CarouselScene scene;
		scene.driver.setAutoSettle(false);
		for (int step = 0; step < 4; step++) {
			scene.trackpad(-0.9f);
			scene.driver.advance(16);
		}
		REQUIRE CHECK_EQUAL(1, scene.tabs.currentIndex());
		for (int step = 0; step < 4; step++) {
			scene.trackpad(0.9f);
			scene.driver.advance(16);
		}
		scene.driver.settle();
		CHECK_EQUAL(0, scene.tabs.currentIndex());
		CHECK_CLOSE(0, scene.tabs.scrollPosition(), 0.0001f);
	}

	TEST(TheCarouselFeelsLikeTheTreeViewStep) {
		CarouselScene scene;
		scene.driver.setAutoSettle(false);
		MagneticStep reference;
		for (int step = 0; step < 3; step++) {
			scene.trackpad(-0.5f);
			reference.push(0.5f * TabView::kPagesPerTrackpadUnit);
			scene.driver.advance(16);
			reference.advance(16);
			CHECK_CLOSE(reference.position(), scene.tabs.scrollPosition(), 0.0001f);
		}
		for (int frame = 0; frame < 40; frame++) {
			scene.driver.advance(16);
			reference.advance(16);
			CHECK_CLOSE(reference.position(), scene.tabs.scrollPosition(), 0.0001f);
		}
	}

	TEST(MouseNotchesAddUpInsteadOfWaitingForTheSlide) {
		CarouselScene scene;
		scene.driver.setAutoSettle(false);
		scene.driver.moveTo(scene.pages[0].parent()->parent()->windowRect().center());
		for (int notch = 0; notch < 2; notch++)
			scene.driver.ui().dispatch(Event::wheel(scene.driver.ui().pointerPosition(), 0, -1));
		scene.driver.settle();
		CHECK_EQUAL(2, scene.tabs.currentIndex());
	}
}

SUITE(ColorWheelWidget) {
	TEST(SaturationValueMappingMatchesDrawnTriangle) {
		const halnf cos120 = std::cos(2.0943951f);
		const halnf sin120 = std::sin(2.0943951f);

		const Vec2F hueCorner = saturationValueAt({ 1, 0 });
		CHECK_CLOSE(1, hueCorner.x, 0.001f);
		CHECK_CLOSE(1, hueCorner.y, 0.001f);

		const Vec2F whiteCorner = saturationValueAt({ cos120, sin120 });
		CHECK_CLOSE(0, whiteCorner.x, 0.001f);
		CHECK_CLOSE(1, whiteCorner.y, 0.001f);

		const Vec2F blackCorner = saturationValueAt({ cos120, -sin120 });
		CHECK_CLOSE(0, blackCorner.y, 0.001f);
	}

	TEST(DraggingRingChangesHue) {
		ColorWheel wheel;
		std::vector<RGBA> changes;
		wheel.setOnChanged([&changes](const RGBA& color) { changes.push_back(color); });
		wheel.setSizing(AxisSizing::fixed(200), AxisSizing::fixed(200));
		Widget root;
		root.emplaceLayout<FreeLayout>();
		root.addChild(wheel);
		UiDriver driver(root);

		driver.click({ 100, 100 + 92 });

		CHECK_CLOSE(1.5708f, wheel.hsv().h, 0.05f);
		CHECK(!changes.empty());
	}

	TEST(SetColorRoundTrips) {
		ColorWheel wheel;
		wheel.setColor(RGBA(0.2f, 0.6f, 0.4f, 1));
		CHECK_CLOSE(0.2f, wheel.color().r, 0.01f);
		CHECK_CLOSE(0.6f, wheel.color().g, 0.01f);
		CHECK_CLOSE(0.4f, wheel.color().b, 0.01f);
	}
}

SUITE(FloatingPanelWidget) {
	struct FloatingScene {
		Widget desk;
		FloatingPanel panel{ "Tools" };
		Button inside{ "Inside" };

		FloatingScene() {
			desk.emplaceLayout<FreeLayout>();
			panel.setContent(&inside);
			panel.setPanelSize({ 200, 150 });
			panel.requestPosition({ 50, 50 });
			desk.addChild(panel);
		}
	};

	TEST(DraggingTitleMovesPanelAndGeometryAnimates) {
		FloatingScene scene;
		UiDriver driver(scene.desk);
		driver.settle();

		const Vec2F grab = scene.panel.windowRect().pos + Vec2F(40.0f, 10.0f);
		driver.setAutoSettle(false);
		driver.drag(grab, grab + Vec2F(100.0f, 60.0f));

		CHECK_CLOSE(150, scene.panel.targetRect().pos.x, 0.01f);
		CHECK_CLOSE(110, scene.panel.targetRect().pos.y, 0.01f);

		driver.advance(16);
		CHECK(scene.panel.rect().pos.x < 150);
		driver.settle();
		CHECK_CLOSE(150, scene.panel.rect().pos.x, 0.01f);
		CHECK_CLOSE(110, scene.panel.rect().pos.y, 0.01f);
	}

	TEST(ResizeGripResizesWithMinimum) {
		FloatingScene scene;
		UiDriver driver(scene.desk);

		const Vec2F grip = scene.panel.windowRect().pos + scene.panel.size() - Vec2F(4.0f, 4.0f);
		driver.drag(grip, grip + Vec2F(50.0f, 30.0f));
		CHECK_CLOSE(250, scene.panel.size().x, 0.5f);
		CHECK_CLOSE(180, scene.panel.size().y, 0.5f);

		const Vec2F newGrip = scene.panel.windowRect().pos + scene.panel.size() - Vec2F(4.0f, 4.0f);
		driver.drag(newGrip, newGrip - Vec2F(1000.0f, 1000.0f));
		CHECK_CLOSE(120, scene.panel.size().x, 0.5f);
		CHECK_CLOSE(80, scene.panel.size().y, 0.5f);
	}

	TEST(PanelNeverShrinksBelowContentMinimum) {
		Widget desk;
		desk.emplaceLayout<FreeLayout>();
		FloatingPanel panel{ "Brush color" };
		ColorWheel wheel;
		panel.setContent(&wheel);
		panel.setPanelSize({ 240, 260 });
		desk.addChild(panel);
		UiDriver driver(desk);

		const Vec2F grip = panel.windowRect().pos + panel.size() - Vec2F(4.0f, 4.0f);
		driver.drag(grip, grip - Vec2F(1000.0f, 1000.0f));

		const RectF body = makeRect(panel.windowRect().pos, panel.size());
		const RectF content = wheel.windowRect();
		CHECK(content.pos.x >= body.pos.x && content.pos.y >= body.pos.y);
		CHECK(content.pos.x + content.size.x <= body.pos.x + body.size.x + 0.01f);
		CHECK(content.pos.y + content.size.y <= body.pos.y + body.size.y + 0.01f);
	}

	TEST(PressingBringsPanelToFront) {
		Widget desk;
		desk.emplaceLayout<FreeLayout>();
		FloatingPanel back("Back");
		FloatingPanel front("Front");
		back.requestPosition({ 0, 0 });
		front.requestPosition({ 100, 0 });
		desk.addChild(back);
		desk.addChild(front);
		UiDriver driver(desk);

		driver.drag(back.windowRect().pos + Vec2F(20.0f, 10.0f), back.windowRect().pos + Vec2F(21.0f, 10.0f));

		CHECK(desk.children().back() == &back);
	}

	TEST(ContentStaysInteractive) {
		FloatingScene scene;
		int clicks = 0;
		scene.inside.setOnActivate([&clicks]() { clicks++; });
		UiDriver driver(scene.desk);

		driver.click(scene.inside);

		CHECK_EQUAL(1, clicks);
		CHECK(!scene.panel.isMoving());
	}
}

SUITE(ControlClickHandler) {
	TEST(ClickHandlerReceivesModifiersAndClickCount) {
		Button button("Row");
		Column scene(button);
		UiDriver driver(scene.root);
		int activations = 0;
		Modifiers seen;
		int clicks = 0;
		button.setOnActivate([&activations]() { activations++; });
		button.setOnClicked([&seen, &clicks](const Event& click) {
			seen = click.modifiers();
			clicks = click.clickCount();
		});

		Modifiers control;
		control.control = true;
		driver.click(button, control);
		CHECK(seen.control);
		CHECK_EQUAL(1, clicks);
		CHECK_EQUAL(0, activations);

		driver.key(InputID::ENTER);
		CHECK_EQUAL(1, activations);
	}
}

SUITE(LogarithmicSlider) {
	TEST(MiddleOfTrackIsGeometricMean) {
		Slider slider(1, 1000, 1);
		slider.setScale(Slider::Scale::Logarithmic);
		CHECK_CLOSE(31.62f, slider.valueForFraction(0.5f), 0.05f);
		CHECK_CLOSE(1.0f / 3.0f, slider.fractionForValue(10), 1e-4f);
		CHECK_CLOSE(1, slider.valueForFraction(0), 1e-4f);
		CHECK_CLOSE(1000, slider.valueForFraction(1), 0.01f);
	}

	TEST(LowValuesGetMostOfTheTrack) {
		Slider linear(1, 1000, 1);
		Slider logarithmic(1, 1000, 1);
		logarithmic.setScale(Slider::Scale::Logarithmic);
		CHECK(logarithmic.fractionForValue(20) > linear.fractionForValue(20) * 10);
	}

	TEST(DraggingFollowsTheLogarithmicCurve) {
		Slider slider(1, 1000, 1);
		slider.setScale(Slider::Scale::Logarithmic);
		Column scene(slider);
		UiDriver driver(scene.root, { 220, 100 });
		driver.setSlider(slider, 5);
		CHECK_CLOSE(5, slider.value(), 0.5f);
		driver.setSlider(slider, 400);
		CHECK_CLOSE(400, slider.value(), 30);
		CHECK_CLOSE(slider.fractionForValue(slider.value()), slider.fraction(), 1e-5f);
	}

	TEST(KeyboardStepsAreRelative) {
		Slider slider(1, 1000, 10);
		slider.setScale(Slider::Scale::Logarithmic);
		Column scene(slider);
		UiDriver driver(scene.root);
		driver.ui().setKeyboardFocus(&slider);
		driver.key(InputID::RIGHT);
		CHECK_CLOSE(10 * std::pow(1000.0f, 0.01f), slider.value(), 0.01f);
	}

	TEST(NonPositiveRangeFallsBackToLinear) {
		Slider slider(0, 100, 0);
		slider.setScale(Slider::Scale::Logarithmic);
		CHECK_CLOSE(50, slider.valueForFraction(0.5f), 1e-4f);
	}
}

SUITE(SliderPointForValue) {
	TEST(ClickingPointForValueSetsThatValue) {
		Slider slider(0, 360, 0);
		Column scene(slider);
		UiDriver driver(scene.root);
		driver.setSlider(slider, 270);
		CHECK_CLOSE(270, slider.value(), 2.0f);
	}
}

SUITE(MultilineLabel) {
	TEST(NewlinesBecomeSeparateLinesAndGrowHeight) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Label single("one line");
		Label multi("Album: Trench\nGenre: Alternative\nYear: 2018");
		root.addChild(single);
		root.addChild(multi);
		UiDriver driver(root);

		CHECK_CLOSE(single.size().y * 3, multi.size().y, 0.5f);
		RecordingPainter& painter = driver.paint();
		CHECK(painter.hasText("Album: Trench"));
		CHECK(painter.hasText("Genre: Alternative"));
		CHECK(painter.hasText("Year: 2018"));
		CHECK(!painter.hasText("Album: Trench\nGenre: Alternative\nYear: 2018"));

		const PaintCommand* first = painter.findText("Album: Trench");
		const PaintCommand* last = painter.findText("Year: 2018");
		CHECK(last->rect.pos.y > first->rect.pos.y);
	}

	TEST(WidthIsTheWidestLine) {
		Widget root;
		root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Start);
		Label shortLine("ab");
		Label multi("ab\nabcdefghij\nabc");
		Label longLine("abcdefghij");
		root.addChild(shortLine);
		root.addChild(multi);
		root.addChild(longLine);
		UiDriver driver(root);
		CHECK_CLOSE(longLine.size().x, multi.size().x, 0.5f);
	}
}

SUITE(InteractionMode) {
	TEST(HoverTooltipsOnlyInPointerMode) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Button button{ "hover me", []() {} };
		button.setTooltip("tip");
		root.addChild(button);
		UiDriver driver(root, { 300, 200 });
		driver.hoverFor(button, 2000);
		CHECK(driver.ui().openTooltip() != nullptr);

		driver.moveTo(Vec2F(290.0f, 190.0f));
		driver.advance(100);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.hoverFor(button, 2000);
		CHECK(driver.ui().openTooltip() == nullptr);
	}

	TEST(ChangingTheModeNotifiesWidgets) {
		struct Watcher : Widget {
			int changes = 0;
			void onInteractionModeChanged() override { changes++; }
		};
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Watcher watcher;
		root.addChild(watcher);
		UiDriver driver(root, { 300, 200 });
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.ui().setInteractionMode(InteractionMode::Pointer);
		CHECK_EQUAL(2, watcher.changes);
	}
}
