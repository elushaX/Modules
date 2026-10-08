#include "FakeWindowHost.hpp"
#include "Gallery.hpp"
#include "ThemeEditor.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <set>
#include <string>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::gallery;
using namespace tp::ui::testing;

namespace {
	constexpr size_t kThemeTab = 6;

	size_t colorIndex(const std::string& name) {
		const auto& fields = themeColorFields();
		for (size_t index = 0; index < fields.size(); index++) {
			if (name == fields[index].name) return index;
		}
		return fields.size();
	}

	struct ThemeScene {
		Gallery gallery;
		UiDriver driver{ gallery, { 1200, 800 } };
		ThemeEditorPage& page = gallery.themePage;

		ThemeScene() { driver.click(gallery.tabs.tabButton(kThemeTab)); }
	};
}

SUITE(ThemeEditorFields) {
	TEST(EveryPaletteColorIsListedOnce) {
		std::set<std::string> names;
		for (const ThemeColorField& field : themeColorFields())
			names.insert(field.name);
		CHECK_EQUAL(themeColorFields().size(), names.size());
		CHECK_EQUAL(sizeof(Palette) / sizeof(RGBA), themeColorFields().size());
	}

	TEST(HexAndSourceDescribeTheTheme) {
		CHECK_EQUAL("#FF8000FF", colorHex({ 1, 0.5f, 0, 1 }));
		CHECK_EQUAL("#00000059", colorHex(Theme::standard().palette.scrim));

		Theme theme;
		theme.palette.accent = { 1, 0, 0, 1 };
		theme.metrics.cornerRadius = 3;
		const std::string source = themeSource(theme);
		CHECK(source.find("theme.palette.accent = { 1.000f, 0.000f, 0.000f, 1.000f };") != std::string::npos);
		CHECK(source.find("theme.metrics.cornerRadius = 3.000f;") != std::string::npos);
	}
}

SUITE(ThemeEditorPage) {
	TEST(TabShowsEveryColorWithSwatchAndHex) {
		ThemeScene scene;
		CHECK_EQUAL("Theme", scene.gallery.tabs.tabButton(kThemeTab).text());
		CHECK_EQUAL(themeColorFields().size(), scene.page.colorRows.size());
		CHECK_EQUAL("Editing window", scene.page.editingTitle.text());
		CHECK_EQUAL(colorHex(Theme::standard().palette.window), scene.page.colorRows[0]->hex.text());
		CHECK(scene.driver.paint().hasText("accent"));
	}

	TEST(SelectingColorLoadsItIntoEditors) {
		ThemeScene scene;
		const size_t scrim = colorIndex("scrim");
		scene.page.selectColor(scrim);
		CHECK_EQUAL("Editing scrim", scene.page.editingTitle.text());
		CHECK_CLOSE(Theme::standard().palette.scrim.a, scene.page.alpha.value(), 0.001f);
		CHECK(scene.page.colorRows[scrim]->name.isSelected());
		CHECK(!scene.page.colorRows[0]->name.isSelected());

		scene.driver.click(scene.page.colorRows[0]->name);
		CHECK_EQUAL(0u, scene.page.selectedColor());
	}

	TEST(WheelAndAlphaEditTheLiveTheme) {
		ThemeScene scene;
		const size_t text = colorIndex("text");
		scene.page.selectColor(text);

		scene.driver.settle();
		const RectF wheel = scene.page.wheel.windowRect();
		scene.driver.click(wheel.center());
		const RGBA edited = scene.driver.ui().theme().palette.text;
		CHECK(!(edited == Theme::standard().palette.text));
		CHECK_EQUAL(colorHex(edited), scene.page.colorRows[text]->hex.text());

		scene.driver.setSlider(scene.page.alpha.slider(), 0.5f);
		CHECK_CLOSE(0.5f, scene.driver.ui().theme().palette.text.a, 0.03f);
		CHECK_CLOSE(edited.r, scene.driver.ui().theme().palette.text.r, 0.001f);
	}

	TEST(MetricSlidersEditTheLiveTheme) {
		ThemeScene scene;
		LabeledSlider& corner = *scene.page.metricSliders[1];
		CHECK_CLOSE(Theme::standard().metrics.cornerRadius, corner.value(), 0.001f);

		scene.driver.setSlider(corner.slider(), 2);
		CHECK_CLOSE(2, scene.driver.ui().theme().metrics.cornerRadius, 0.6f);
	}

	TEST(AccentPresetAndReset) {
		ThemeScene scene;
		scene.driver.click(scene.page.accentPreset);
		scene.driver.click(*scene.page.accentPreset.menu().items()[2]);
		CHECK(scene.driver.ui().theme().palette.accent == accents::green);

		scene.driver.click(scene.page.reset);
		CHECK(scene.driver.ui().theme().palette.accent == Theme::standard().palette.accent);
		CHECK_EQUAL("Reset to the standard theme", scene.page.status.text());
	}

	TEST(CopyPutsThemeSourceOnClipboard) {
		ThemeScene scene;
		FakeWindowHost host;
		scene.driver.ui().setWindowHost(&host);
		scene.driver.click(scene.page.copySource);
		CHECK_EQUAL(themeSource(scene.page.editedTheme()), host.clipboard);
		CHECK_EQUAL("Copied theme as C++", scene.page.status.text());
		scene.driver.ui().setWindowHost(nullptr);
	}

	TEST(ControlsPageAccentIsPickedUpByEditor) {
		ThemeScene scene;
		scene.driver.click(scene.gallery.tabs.tabButton(0));
		scene.driver.ui().setTheme(Theme::standard().withAccent(accents::orange));
		scene.driver.click(scene.gallery.tabs.tabButton(kThemeTab));
		scene.page.selectColor(colorIndex("text"));
		scene.driver.setSlider(scene.page.alpha.slider(), 0.9f);
		CHECK(scene.driver.ui().theme().palette.accent == accents::orange);
	}
}

SUITE(ThemeScaling) {
	TEST(ScalingMultipliesLengthsAndKeepsTimesAndHairlines) {
		const Theme base = Theme::standard();
		const Theme half = base.scaled(0.5f);

		CHECK_CLOSE(base.metrics.fontSize * 0.5f, half.metrics.fontSize, 0.001f);
		CHECK_CLOSE(base.metrics.controlHeight * 0.5f, half.metrics.controlHeight, 0.001f);
		CHECK_CLOSE(base.metrics.menuItemHeight * 0.5f, half.metrics.menuItemHeight, 0.001f);
		CHECK_CLOSE(base.metrics.popupCornerRadius * 0.5f, half.metrics.popupCornerRadius, 0.001f);
		CHECK_CLOSE(base.metrics.controlPadding.left * 0.5f, half.metrics.controlPadding.left, 0.001f);
		CHECK_CLOSE(base.metrics.menuPadding.top * 0.5f, half.metrics.menuPadding.top, 0.001f);
		CHECK_CLOSE(base.metrics.tooltipDelayMs, half.metrics.tooltipDelayMs, 0.001f);
		CHECK_CLOSE(base.metrics.borderWidth, half.metrics.borderWidth, 0.001f);
		CHECK_CLOSE(base.metrics.separatorThickness, half.metrics.separatorThickness, 0.001f);
	}

	TEST(ScalingKeepsThePalette) {
		const Theme base = Theme::standard().withAccent(accents::green);

		CHECK(base.scaled(1.5f).palette.accent == base.palette.accent);
	}
}
