#pragma once

#include "Sketch2DUI.hpp"
#include "SettingsView.hpp"
#include "Theme.hpp"

namespace tp::sketch2d {

	struct TouchLayoutSettings {
		static constexpr halnf kMinimumScale = 0.6f;
		static constexpr halnf kMaximumScale = 1.4f;
		static constexpr halnf kDefaultScale = 0.85f;
	};

	[[nodiscard]] ui::Theme
	touchTheme(halnf scale = TouchLayoutSettings::kDefaultScale, const ui::Theme& base = ui::Theme::standard());

	class ColorSwatchButton : public ui::PopupButton {
	public:
		explicit ColorSwatchButton(const Brush& brush);

	protected:
		[[nodiscard]] bool hasIcon() const override { return true; }
		void paintIcon(ui::Painter& painter, const RectF& box) override;

	private:
		const Brush& mBrush;
	};

	enum class Glyph { Collapse, Expand };

	template <typename ButtonType>
	class GlyphButton : public ButtonType {
	public:
		explicit GlyphButton(Glyph glyph) :
			mGlyph(glyph) {}

		void setGlyph(Glyph glyph);

	protected:
		[[nodiscard]] bool hasIcon() const override { return true; }
		void paintIcon(ui::Painter& painter, const RectF& box) override;

	private:
		Glyph mGlyph;
	};

	enum class ActivePanel { None, Brush, Fill, Transform, Selection };

	enum class SliderGlyph { Size, Opacity };

	class SliderGlyphMark : public ui::Widget {
	public:
		explicit SliderGlyphMark(SliderGlyph glyph);

		[[nodiscard]] SliderGlyph glyph() const { return mGlyph; }
		void setExtent(halnf extent);

	protected:
		[[nodiscard]] ui::Extent measureContent(ui::Axis axis, halnf crossLength) const override;
		void paint(ui::Painter& painter) override;

	private:
		SliderGlyph mGlyph;
		halnf mExtent = 24;
	};

	struct StudioToolGroup {
		std::string name;
		std::string label;
		std::vector<std::string> tools;
		std::string current;
		std::unique_ptr<ui::PopupButton> button;
		ui::Menu menu;
	};

	class StudioRoot : public ui::Widget {
	public:
		static constexpr halnf kEdgeMargin = 12;
		static constexpr halnf kButton = 56;
		static constexpr halnf kSliderLength = 240;
		static constexpr halnf kSmallestSlider = 80;
		static constexpr halnf kBarPadding = 4;
		static constexpr halnf kBarGap = 2;
		static constexpr halnf kCornerRadius = 22;
		static constexpr halnf kSmallestButton = 30;

		StudioRoot(Sketch2DRoot& root, Brush& brush, const IconProvider& icons);
		~StudioRoot() override;

		[[nodiscard]] bool areControlsShown() const { return toolBar.isVisible(); }
		void setControlsShown(bool shown);
		void refreshFromState();
		[[nodiscard]] bool isMenuBarShown() const;
		[[nodiscard]] bool isTouchMode() const;

		[[nodiscard]] halnf scale() const { return mScale; }
		void setScale(halnf scale);
		[[nodiscard]] halnf buttonSize() const { return kButton * mScale; }
		[[nodiscard]] ActivePanel activePanel() const { return mActivePanel; }
		[[nodiscard]] bool isToolBarVertical() const { return mToolBarVertical; }
		[[nodiscard]] ui::Button& toolButton(const std::string& name);
		[[nodiscard]] std::vector<ui::Button*> toolButtons();
		[[nodiscard]] StudioToolGroup* group(const std::string& name);
		[[nodiscard]] StudioToolGroup* groupOf(const std::string& tool);
		[[nodiscard]] bool isToolSelected(const std::string& tool) const;
		void useTool(const std::string& name);

		ui::Panel topLeft{ ui::Panel::Surface::Raised };
		ui::Panel topRight{ ui::Panel::Surface::Raised };
		ui::Panel toolBar{ ui::Panel::Surface::Raised };
		ui::Panel activeBar{ ui::Panel::Surface::Raised };

		ui::Button library;
		ui::Button undo;
		ui::Button redo;
		GlyphButton<ui::Button> hide{ Glyph::Collapse };
		ui::Button settingsButton;

		ui::PopupButton presets;
		ColorSwatchButton color;
		ui::PopupButton brushOptions;
		ui::PopupButton layers;

		ui::Slider size{ 1, Brush::kMaximumSize, 1 };
		ui::Slider opacity{ 0, 1, 1 };
		SliderGlyphMark sizeMark{ SliderGlyph::Size };
		SliderGlyphMark opacityMark{ SliderGlyph::Opacity };
		ui::Button flip{ "Flip" };
		ui::Button flop{ "Flop" };
		ui::Button proportional{ "Prop." };
		ui::Button cancelTransform{ "Cancel" };
		ui::Button applyTransform{ "Apply" };
		ui::Button deselect{ "Deselect" };
		ui::Button invertSelection{ "Invert" };

		GlyphButton<ui::Button> reveal{ Glyph::Expand };
		ui::SettingsView settings;

	protected:
		void onAttached() override;
		void onInteractionModeChanged() override;
		void onEvent(ui::Event& event) override;

	private:
		class StudioLayout;
		friend class StudioLayout;

		void addTool(const std::string& name, const std::string& label);
		void addGroup(
			const std::string& name, const std::string& label, const std::vector<std::pair<std::string, std::string>>& tools
		);
		void showPreview(SliderGlyph glyph, bool shown);
		void fitTopBars(halnf available);
		void applyInteractionMode();
		[[nodiscard]] static size_t visibleChildren(const ui::Widget& bar);
		void configure(ui::Button& button, const char* icon, const char* label);
		void refreshActiveBar();
		void fitSliders(halnf available);
		[[nodiscard]] bool wantsActiveBar() const;
		void fitToolBar(halnf available);
		void sizeButtons();
		[[nodiscard]] RectF controlArea(const RectF& area) const;
		[[nodiscard]] ActivePanel panelFor() const;
		[[nodiscard]] std::vector<ui::Button*> barButtons();

		Sketch2DRoot& mRoot;
		Brush& mBrush;
		const IconProvider& mIcons;
		halnf mScale = TouchLayoutSettings::kDefaultScale;
		halnf mSliderLength = kSliderLength;
		halnf mToolButton = kButton;
		halnf mTopButton = kButton;
		bool mToolBarVertical = false;
		bool mPortrait = false;
		void orientToolBar(bool vertical);
		std::vector<std::unique_ptr<StudioToolGroup>> mGroups;
		std::vector<ui::Button*> mBarOrder;
		ActivePanel mActivePanel = ActivePanel::None;
		std::vector<std::pair<std::string, std::unique_ptr<ui::Button>>> mTools;
	};
}
