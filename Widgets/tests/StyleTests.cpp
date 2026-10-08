#include "BoxLayout.hpp"
#include "Gallery.hpp"
#include "Button.hpp"
#include "Menus.hpp"
#include "Slider.hpp"
#include "Theme.hpp"
#include "Toggles.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"
#include "Views.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	bool sameColor(const RGBA& first, const RGBA& second) {
		const halnf tolerance = 0.002f;
		return std::abs(first.r - second.r) < tolerance && std::abs(first.g - second.g) < tolerance &&
					 std::abs(first.b - second.b) < tolerance && std::abs(first.a - second.a) < tolerance;
	}

	struct ControlsScene {
		Widget root;
		Button button{ "Button" };
		Button primary{ "Primary" };
		Checkbox checkbox{ "Checkbox" };
		Dropdown dropdown{ { "One", "Two" } };

		ControlsScene() {
			root.emplaceLayout<ColumnLayout>(8).setCrossAlign(CrossAlign::Start);
			primary.setStyle(Button::Style::Accent);
			checkbox.setChecked(true);
			for (Widget* widget : std::initializer_list<Widget*>{ &button, &primary, &checkbox, &dropdown }) {
				root.addChild(*widget);
			}
		}
	};
}

SUITE(Style) {
	TEST(ControlsPaintRoundedShapesOnly) {
		ControlsScene scene;
		UiDriver driver(scene.root);
		driver.settle();

		size_t shapes = 0;
		for (const PaintCommand& command : driver.paint().commands()) {
			const bool box = command.kind == PaintCommand::Kind::FillRect || command.kind == PaintCommand::Kind::StrokeRect;
			if (!box || command.color.a <= 0.01f) continue;
			shapes++;
			CHECK(command.cornerRadius > 0);
		}
		CHECK(shapes >= 5);
	}

	TEST(PopupMenusUseLargerRadiusThanControls) {
		const Metrics& metrics = Theme::standard().metrics;
		CHECK(metrics.popupCornerRadius > metrics.cornerRadius);
		CHECK(metrics.cornerRadius >= 3);
	}

	TEST(AccentDerivesAllAccentShades) {
		const Theme purple = Theme::standard().withAccent(accents::purple);

		CHECK(sameColor(accents::purple, purple.palette.accent));
		CHECK(purple.palette.accentHovered.r > purple.palette.accent.r);
		CHECK(purple.palette.accentPressed.r < purple.palette.accent.r);
		CHECK_CLOSE(accents::purple.r, purple.palette.focusRing.r, 0.001f);
		CHECK(sameColor(Theme::standard().palette.window, purple.palette.window));
	}

	TEST(ChangingAccentRepaintsControlsInNewColor) {
		ControlsScene scene;
		UiDriver driver(scene.root);
		driver.settle();

		driver.ui().setTheme(Theme::standard().withAccent(accents::green));
		driver.settle();

		bool primaryIsGreen = false;
		for (const PaintCommand& command : driver.paint().commands()) {
			const bool onPrimary =
				command.kind == PaintCommand::Kind::FillRect && contains(command.rect, scene.primary.windowRect().center());
			primaryIsGreen = primaryIsGreen || (onPrimary && sameColor(command.color, accents::green));
		}
		CHECK(primaryIsGreen);
	}

	TEST(ControlsStartInTheirRestingColorWithoutAnimating) {
		Widget root;
		root.emplaceLayout<ColumnLayout>(8);
		Button button("Button");
		Button primary("Primary");
		primary.setStyle(Button::Style::Accent);
		root.addChild(button);
		root.addChild(primary);
		UiDriver driver(root);

		driver.ui().updateLayout();

		CHECK(driver.ui().animatingWidgets().empty());
	}

	TEST(TabsDockingMenusAndSlidersUseNoAccentColor) {
		gallery::Gallery gallery;
		UiDriver driver(gallery, { 1200, 800 });
		const RGBA accent = Theme::standard().palette.accent;
		const auto accentPainted = [&driver, &accent]() {
			for (const PaintCommand& command : driver.paint().commands()) {
				if (command.color.a > 0.01f && sameColor(command.color, accent)) return true;
			}
			return false;
		};

		driver.click(gallery.tabs.tabButton(3));
		driver.settle();
		CHECK(!accentPainted());

		driver.click(gallery.dockPage.project.optionsButton());
		driver.moveTo(*gallery.dockPage.project.optionsMenu().items()[2]);
		driver.settle();
		CHECK(!accentPainted());
		driver.key(InputID::ESCAPE);

		driver.click(gallery.tabs.tabButton(0));
		driver.moveTo(gallery.controls.volume);
		driver.settle();
		bool sliderUsesAccent = false;
		const RectF slider = gallery.controls.volume.windowRect();
		for (const PaintCommand& command : driver.paint().commands()) {
			const bool onSlider = contains(slider, command.rect.center());
			sliderUsesAccent = sliderUsesAccent || (onSlider && sameColor(command.color, accent));
		}
		CHECK(!sliderUsesAccent);
	}

	TEST(SelectedTabIsAGrayFill) {
		TabView tabs;
		Widget first;
		Widget second;
		tabs.addTab("First", first);
		tabs.addTab("Second", second);
		UiDriver driver(tabs, Vec2F(400.0f, 200.0f));
		driver.moveTo(Vec2F(-50.0f, -50.0f));
		driver.settle();

		const RectF tab = tabs.tabButton(0).windowRect();
		bool grayFill = false;
		for (const PaintCommand& command : driver.paint().commands()) {
			const bool coversTab = command.kind == PaintCommand::Kind::FillRect && command.rect.size.y >= tab.size.y - 1 &&
														 contains(command.rect, tab.center());
			grayFill = grayFill || (coversTab && sameColor(command.color, Theme::standard().palette.neutralSelection));
		}
		CHECK(grayFill);
	}

	TEST(CheckedTogglesAreMonochrome) {
		Widget root;
		root.emplaceLayout<ColumnLayout>(8);
		Checkbox checkbox("Checkbox");
		Switch toggle("Switch");
		RadioButton radio("Radio");
		for (ToggleControl* control : std::initializer_list<ToggleControl*>{ &checkbox, &toggle, &radio }) {
			control->setChecked(true);
			root.addChild(*control);
		}
		UiDriver driver(root);
		driver.settle();

		const Palette& palette = Theme::standard().palette;
		bool usedToggleOn = false;
		for (const PaintCommand& command : driver.paint().commands()) {
			CHECK(!sameColor(command.color, palette.accent));
			usedToggleOn = usedToggleOn || sameColor(command.color, palette.toggleOn);
			const bool gray =
				std::abs(command.color.r - command.color.g) < 0.03f && std::abs(command.color.g - command.color.b) < 0.05f;
			CHECK(gray);
		}
		CHECK(usedToggleOn);
	}
}
