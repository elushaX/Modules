#include "BoxLayout.hpp"
#include "Slider.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"
#include "Views.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	struct SquareHost : Widget {
		ColorSquare square;

		SquareHost() {
			emplaceLayout<ColumnLayout>(0).setPadding(Insets::uniform(10));
			square.setSizing(AxisSizing::fixed(200), AxisSizing::fixed(224));
			addChild(square);
		}
	};
}

SUITE(ColorSquareWidget) {
	TEST(DraggingTheSquarePicksSaturationAndValue) {
		SquareHost host;
		UiDriver driver(host, { 400, 400 });
		HSV picked;
		int4 changes = 0;
		host.square.setOnChanged([&](const HSV& hsv) {
			picked = hsv;
			changes++;
		});
		driver.drag(host.square.windowPointFor(0.2f, 0.9f), host.square.windowPointFor(0.75f, 0.4f));
		CHECK_CLOSE(0.75f, picked.s, 0.02f);
		CHECK_CLOSE(0.4f, picked.v, 0.02f);
		CHECK(changes > 1);
		CHECK_CLOSE(0.75f, host.square.hsv().s, 0.02f);
	}

	TEST(TheHueStripPicksHueAndClamps) {
		SquareHost host;
		UiDriver driver(host, { 400, 400 });
		const halnf twoPi = 6.2831853f;
		driver.click(host.square.windowPointForHue(twoPi / 4));
		CHECK_CLOSE(twoPi / 4, host.square.hsv().h, 0.05f);
		driver.drag(host.square.windowPointForHue(twoPi / 4), host.square.windowPointForHue(twoPi) + Vec2F{ 50, 0 });
		CHECK(host.square.hsv().h < twoPi);
		CHECK(host.square.hsv().h > twoPi * 0.95f);
	}

	TEST(DragsAreClampedToTheSquare) {
		SquareHost host;
		UiDriver driver(host, { 400, 400 });
		driver.drag(host.square.windowPointFor(0.5f, 0.5f), host.square.windowPointFor(0.5f, 0.5f) + Vec2F{ 500, 500 });
		CHECK_CLOSE(1.0f, host.square.hsv().s, 1e-4f);
		CHECK_CLOSE(0.0f, host.square.hsv().v, 1e-4f);
	}

	TEST(PaintsGradientsForSquareAndHue) {
		SquareHost host;
		UiDriver driver(host, { 400, 400 });
		host.square.setHsv(HSV(0, 0.5f, 0.5f));
		int4 gradients = 0;
		for (const PaintCommand& command : driver.paint().commands())
			if (command.kind == PaintCommand::Kind::Gradient) gradients++;
		CHECK_EQUAL(8, gradients);
	}
}

SUITE(GradientSlider) {
	TEST(SliderPaintsItsTrackGradient) {
		struct Host : Widget {
			Slider slider{ 0, 1, 0.5f };
			Host() {
				emplaceLayout<ColumnLayout>(0).setPadding(Insets::uniform(10));
				slider.setSizing(AxisSizing::fixed(200), AxisSizing::fixed(30));
				addChild(slider);
			}
		} host;
		UiDriver driver(host, { 300, 100 });
		host.slider.setTrackGradient([]() { return std::vector<RGBA>{ { 1, 0, 0, 1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 1 } }; });
		std::vector<PaintCommand> gradients;
		for (const PaintCommand& command : driver.paint().commands())
			if (command.kind == PaintCommand::Kind::Gradient) gradients.push_back(command);
		REQUIRE CHECK_EQUAL((size_t) 2, gradients.size());
		CHECK_CLOSE(1.0f, gradients[0].color.r, 1e-6f);
		CHECK_CLOSE(1.0f, gradients[0].endColor.g, 1e-6f);
		CHECK_CLOSE(1.0f, gradients[1].endColor.b, 1e-6f);
		CHECK_CLOSE(gradients[0].rect.size.x, gradients[1].rect.size.x, 1e-3f);

		driver.click(host.slider.windowPointForValue(0.9f));
		CHECK_CLOSE(0.9f, host.slider.value(), 0.02f);
	}
}
