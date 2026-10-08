#pragma once

#include "BoxLayout.hpp"
#include "Button.hpp"
#include "Containers.hpp"
#include "Label.hpp"
#include "Menus.hpp"
#include "Slider.hpp"
#include "Theme.hpp"
#include "Views.hpp"

#include <memory>
#include <string>
#include <vector>

namespace tp::ui {

	struct ThemeColorField {
		const char* name;
		RGBA Palette::* member;
	};

	struct ThemeMetricField {
		const char* name;
		halnf Metrics::* member;
		halnf minimum;
		halnf maximum;
	};

	const std::vector<ThemeColorField>& themeColorFields();
	const std::vector<ThemeMetricField>& themeMetricFields();
	std::string colorHex(const RGBA& color);
	std::string themeSource(const Theme& theme);

	class ColorSwatch : public Control {
	public:
		ColorSwatch();

		void setColor(const RGBA& color);
		[[nodiscard]] const RGBA& color() const { return mColor; }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;

	private:
		RGBA mColor;
	};

	class ThemeColorRow : public Widget {
	public:
		explicit ThemeColorRow(const ThemeColorField& field);

		void refresh(const Theme& theme);
		void setSelected(bool selected);

		ColorSwatch swatch;
		Button name;
		Label hex;

	private:
		const ThemeColorField& mField;
	};

	class ThemeEditorPage : public Widget {
	public:
		ThemeEditorPage();

		[[nodiscard]] size_t selectedColor() const { return mSelected; }
		void selectColor(size_t index);
		[[nodiscard]] Theme editedTheme() const;

		ScrollView colorScroll;
		Widget colorList;
		std::vector<std::unique_ptr<ThemeColorRow>> colorRows;

		Label editingTitle;
		ColorWheel wheel;
		LabeledSlider alpha{ "Alpha", 0, 1, 1 };
		std::vector<std::unique_ptr<LabeledSlider>> metricSliders;
		Dropdown accentPreset{ { "Blue", "Purple", "Green", "Orange", "Pink" } };
		Button reset{ "Reset to standard" };
		Button copySource{ "Copy as C++" };
		Label status;

	protected:
		void onAttached() override;
		void onThemeChanged() override;

	private:
		template <typename Edit>
		void edit(Edit&& change);
		void refreshEditors();

		size_t mSelected = 0;
		bool mRefreshing = false;
		Widget mColorColumn;
		Widget mEditorColumn;
		Widget mActionRow;
		Separator mSeparator;
	};
}
