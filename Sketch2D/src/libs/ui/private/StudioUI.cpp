#include "StudioUI.hpp"

#include "BoxLayout.hpp"
#include "Painter.hpp"
#include "Layout.hpp"
#include "Ui.hpp"
#include "WindowHost.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace tp::sketch2d {

	using namespace tp::ui;

	namespace {
		constexpr halnf kSwatchInset = 9;
		constexpr halnf kIconFraction = 0.5f;
		constexpr halnf kPopupGap = 8;
		constexpr halnf kMarkSize = 22;

	}

	SliderGlyphMark::SliderGlyphMark(SliderGlyph glyph) :
		mGlyph(glyph) {
		setDebugName(glyph == SliderGlyph::Size ? "size mark" : "opacity mark");
		setTooltip(glyph == SliderGlyph::Size ? "Size" : "Opacity");
	}

	void SliderGlyphMark::setExtent(halnf extent) {
		if (extent == mExtent) return;
		mExtent = extent;
		requestLayout();
	}

	Extent SliderGlyphMark::measureContent(Axis, halnf) const { return Extent::rigid(mExtent); }

	void SliderGlyphMark::paint(Painter& painter) {
		const RGBA ink = theme().palette.mutedText;
		const Vec2F center = localRect().center();
		const halnf radius = mExtent * 0.36f;
		if (mGlyph == SliderGlyph::Size) {
			painter.fillCircle(center + Vec2F(-radius * 0.55f, radius * 0.35f), radius * 0.3f, ink);
			painter.strokeCircle(center + Vec2F(radius * 0.25f, -radius * 0.2f), radius * 0.65f, ink, 1.5f);
			return;
		}
		painter.strokeCircle(center, radius, ink, 1.5f);
		painter.fillPolygon(
			{ center + Vec2F(0.0f, -radius), center + Vec2F(radius, 0.0f), center + Vec2F(0.0f, radius) }, ink
		);
	}

	Theme touchTheme(halnf scale, const Theme& base) {
		Theme theme = base;
		Metrics& metrics = theme.metrics;
		metrics.fontSize = 15;
		metrics.smallFontSize = 13;
		metrics.lineHeight = 22;
		metrics.cornerRadius = 12;
		metrics.popupCornerRadius = 18;
		metrics.surfaceCornerRadius = 18;
		metrics.indicatorCornerRadius = 6;
		metrics.controlHeight = 40;
		metrics.controlPadding = Insets::symmetric(14, 8);
		metrics.iconButtonSize = 32;
		metrics.indicatorSize = 22;
		metrics.spacing = 10;
		metrics.sliderTrackThickness = 6;
		metrics.sliderHandleSize = 26;
		metrics.menuItemHeight = 44;
		metrics.menuPadding = Insets::uniform(8);
		metrics.popupGap = 8;
		return theme.scaled(scale);
	}

	ColorSwatchButton::ColorSwatchButton(const Brush& brush) :
		mBrush(brush) {
		setDebugName("color swatch");
	}

	void ColorSwatchButton::paintIcon(Painter& painter, const RectF&) {
		const halnf radius = std::min(size().x, size().y) * 0.5f - kSwatchInset;
		const Vec2F center = localRect().center();
		const RGBA color = { mBrush.color.r, mBrush.color.g, mBrush.color.b, 1 };
		painter.fillCircle(center, radius, color);
		painter.strokeCircle(center, radius, theme().palette.controlBorder, 2);
	}

	template <typename ButtonType>
	void GlyphButton<ButtonType>::setGlyph(Glyph glyph) {
		mGlyph = glyph;
		this->requestPaint();
	}

	template <typename ButtonType>
	void GlyphButton<ButtonType>::paintIcon(Painter& painter, const RectF&) {
		const RGBA color = this->isEffectivelyEnabled() ? this->theme().palette.text : this->theme().palette.disabledText;
		const halnf unit = 1.0f;
		const Vec2F center = this->localRect().center();
		const halnf direction = mGlyph == Glyph::Collapse ? 1.0f : -1.0f;
		const Vec2F along = Vec2F(direction * 4, 0.0f) * unit;
		const Vec2F across = Vec2F(0.0f, 7.0f) * unit;
		painter.polyline({ center - along - across, center + along, center - along + across }, color, 2.5f * unit);
	}

	template class GlyphButton<Button>;
	template class GlyphButton<PopupButton>;

	class StudioRoot::StudioLayout : public Layout {
	public:
		explicit StudioLayout(StudioRoot& studio) :
			mStudio(studio) {}

	protected:
		Extent measureChildren(const LayoutContext&, Axis, halnf) const override { return {}; }

		void arrangeChildren(LayoutContext& context, const RectF& fullArea) override {
			context.place(mStudio.mRoot, fullArea);

			const RectF area = mStudio.controlArea(belowMenuBar(context, fullArea));
			const halnf margin = kEdgeMargin * mStudio.mScale;
			const auto sizeOf = [&context](Widget& widget) {
				return Vec2F(
					context.measure(widget, Axis::Horizontal, kUnbounded).preferred,
					context.measure(widget, Axis::Vertical, kUnbounded).preferred
				);
			};

			const halnf right = area.pos.x + area.size.x - margin;
			const halnf bottom = area.pos.y + area.size.y - margin;
			const halnf top = area.pos.y + margin;

			const bool landscape = fullArea.size.x > fullArea.size.y;
			mStudio.orientToolBar(landscape);
			mStudio.fitTopBars(area.size.x - 3 * margin);
			const Vec2F leftSize = sizeOf(mStudio.topLeft);
			const Vec2F rightSize = sizeOf(mStudio.topRight);
			const halnf belowTop = top + std::max(leftSize.y, rightSize.y) + margin;
			mStudio.fitToolBar(landscape ? bottom - belowTop : area.size.x - 2 * margin);
			const Vec2F barSize = sizeOf(mStudio.toolBar);
			const RectF barRect =
				landscape ? makeRect({ right - barSize.x, belowTop + (bottom - belowTop - barSize.y) * 0.5f }, barSize)
									: makeRect({ area.pos.x + (area.size.x - barSize.x) * 0.5f, bottom - barSize.y }, barSize);

			const halnf columnTop = top + leftSize.y + margin;
			const halnf columnBottom = bottom;
			mStudio.mPortrait = !landscape;
			mStudio.fitSliders(columnBottom - columnTop);
			Vec2F columnSize = sizeOf(mStudio.activeBar);
			columnSize.y = std::min(columnSize.y, std::max(0.0f, columnBottom - columnTop));

			if (mStudio.topLeft.isVisible()) context.place(mStudio.topLeft, makeRect({ area.pos.x + margin, top }, leftSize));
			if (mStudio.topRight.isVisible())
				context.place(mStudio.topRight, makeRect({ right - rightSize.x, top }, rightSize));
			if (mStudio.toolBar.isVisible()) context.place(mStudio.toolBar, barRect);
			mStudio.activeBar.setVisible(mStudio.wantsActiveBar());
			if (mStudio.activeBar.isVisible()) {
				const halnf span = columnBottom - columnTop;
				const halnf y = columnTop + std::max(0.0f, (span - columnSize.y) * 0.5f);
				context.place(mStudio.activeBar, makeRect({ area.pos.x + margin, y }, columnSize));
			}
			if (mStudio.reveal.isVisible()) {
				const halnf button = mStudio.mTopButton;
				const Vec2F corner = { area.pos.x + margin + kBarPadding, top + kBarPadding };
				context.place(mStudio.reveal, makeRect(corner, { button, button }));
			}
		}

	private:
		RectF belowMenuBar(const LayoutContext& context, const RectF& fullArea) const {
			if (!mStudio.isMenuBarShown()) return fullArea;
			const halnf height = context.measure(mStudio.mRoot.menuBar, Axis::Vertical, fullArea.size.x).preferred;
			return makeRect({ fullArea.pos.x, fullArea.pos.y + height }, { fullArea.size.x, fullArea.size.y - height });
		}

		StudioRoot& mStudio;
	};

	StudioRoot::StudioRoot(Sketch2DRoot& root, Brush& brush, const IconProvider& icons) :
		color(brush),
		mRoot(root),
		mBrush(brush),
		mIcons(icons) {
		setDebugName("sketch2d studio");
		setSizing(AxisSizing::fill(), AxisSizing::fill());
		setLayout(std::make_unique<StudioLayout>(*this));

		root.menuBar.setVisible(false);
		root.toolBar.setVisible(false);
		root.toolStrip.setVisible(false);
		root.transformBar.setVisible(false);

		for (Panel* panel : { &topLeft, &topRight, &toolBar, &activeBar }) {
			panel->setBorder(true);
			panel->setCornerRadius(kCornerRadius * mScale);
		}
		topLeft.setDebugName("studio top left");
		topRight.setDebugName("studio top right");
		toolBar.setDebugName("studio tool bar");
		activeBar.setDebugName("studio active tool bar");
		for (Panel* panel : { &topLeft, &topRight, &toolBar })
			panel->emplaceLayout<RowLayout>(kBarGap).setPadding(Insets::uniform(kBarPadding));
		ColumnLayout& column = activeBar.emplaceLayout<ColumnLayout>(kBarGap * 4);
		column.setPadding(Insets::uniform(kBarPadding * 2));
		column.setCrossAlign(CrossAlign::Center);

		configure(library, "library", "Sketches");
		configure(undo, "undo", "Undo");
		configure(redo, "redo", "Redo");
		configure(hide, "", "Hide controls");
		configure(settingsButton, "settings", "Settings");
		for (Button* button : std::initializer_list<Button*>{ &hide, &library, &undo, &redo, &settingsButton })
			topLeft.addChild(*button);

		configure(presets, "brush", "Brushes");
		configure(brushOptions, "size", "Brush options");
		configure(color, "", "Color");
		configure(layers, "layers", "Layers");
		for (Button* button : std::initializer_list<Button*>{ &presets, &brushOptions, &color, &layers })
			topRight.addChild(*button);

		ToolBar& bar = root.toolBar;
		presets.setPopupContent(&bar.brushListPopup);
		brushOptions.setPopupContent(&bar.brushPopup);
		color.setPopupContent(&bar.colorPopup);
		layers.setPopupContent(&bar.layersPopup);
		bar.layersPopup.showThrough(layers);
		for (PopupButton* button : std::initializer_list<PopupButton*>{ &presets, &brushOptions, &color, &layers }) {
			button->popupOptions().placement = Placement::Below;
			button->popupOptions().align = PlacementAlign::End;
		}
		presets.setOnOpening([this]() {
			if (!ToolStrip::isPaintingBrush(mBrush)) mRoot.toolStrip.button("brush").activate();
			mRoot.toolBar.brushListPopup.refresh();
		});
		bar.brushListPopup.onChosen = [this]() {
			mRoot.toolBar.brushPopup.refresh();
			presets.closePopup();
			mRoot.refreshFromState();
		};
		brushOptions.setOnOpening([this]() { mRoot.toolBar.brushPopup.refresh(); });
		color.setOnOpening([this]() { mRoot.toolBar.colorPopup.refresh(); });
		layers.setOnOpening([this]() {
			mRoot.toolBar.layersPopup.refresh();
			mRoot.toolBar.layersPopup.opened();
		});

		library.setOnActivate([this]() {
			if (ui()) mRoot.library.open(*ui());
		});
		undo.setOnActivate([this]() {
			mRoot.commands.run(Command::Undo);
			mRoot.refreshFromState();
		});
		redo.setOnActivate([this]() {
			mRoot.commands.run(Command::Redo);
			mRoot.refreshFromState();
		});
		hide.setOnActivate([this]() { setControlsShown(false); });
		root.isDesktopLayout = [this]() { return ui() && !ui()->isTouchMode(); };
		root.setDesktopLayout = [this](bool desktop) {
			if (ui()) ui()->setInteractionMode(desktop ? InteractionMode::Pointer : InteractionMode::Touch);
		};
		settings.setRoot("Menu", root.mainMenu);
		settingsButton.setOnActivate([this]() {
			if (ui()) settings.open(*ui());
		});

		addTool("brush", "Brush");
		addTool("eraser", "Eraser");
		addGroup(
			"paint", "Smudge, fill and gradient", { { "smudge", "Smudge" }, { "fill", "Fill" }, { "gradient", "Gradient" } }
		);
		addTool("eyedropper", "Color picker");
		addGroup(
			"select", "Selection", { { "lasso", "Lasso" }, { "rectselect", "Rectangle select" }, { "wand", "Magic wand" } }
		);
		addGroup(
			"transform", "Transform and crop", { { "transform", "Transform" }, { "move", "Move" }, { "crop", "Crop" } }
		);

		size.setOrientation(Axis::Vertical);
		size.setScale(Slider::Scale::Logarithmic);
		size.setTooltip("Size");
		size.setOnChanged([this](halnf value) {
			mBrush.setSize(value);
			mRoot.toolBar.brushPopup.refresh();
		});
		opacity.setOrientation(Axis::Vertical);
		opacity.setTooltip("Opacity");
		opacity.setOnChanged([this](halnf value) {
			mBrush.setOpacity(value);
			mRoot.toolBar.brushPopup.refresh();
		});
		size.setDebugName("studio size slider");
		opacity.setDebugName("studio opacity slider");
		size.setOnDragging([this](bool dragging) { showPreview(SliderGlyph::Size, dragging); });
		bar.brushPopup.size.slider().setOnDragging([this](bool dragging) { showPreview(SliderGlyph::Size, dragging); });
		bar.brushPopup.opacity.slider().setOnDragging([this](bool dragging) {
			showPreview(SliderGlyph::Opacity, dragging);
		});
		opacity.setOnDragging([this](bool dragging) { showPreview(SliderGlyph::Opacity, dragging); });

		applyTransform.setStyle(Button::Style::Accent);
		flip.setOnActivate([this]() { mRoot.transformBar.flip.activate(); });
		flop.setOnActivate([this]() { mRoot.transformBar.flop.activate(); });
		proportional.setOnActivate([this]() {
			mRoot.transformBar.proportional.activate();
			refreshFromState();
		});
		cancelTransform.setOnActivate([this]() {
			mRoot.transformBar.cancel.activate();
			refreshFromState();
		});
		applyTransform.setOnActivate([this]() {
			mRoot.transformBar.apply.activate();
			refreshFromState();
		});
		deselect.setOnActivate([this]() {
			mRoot.commands.run(Command::Deselect);
			mRoot.refreshFromState();
		});
		invertSelection.setOnActivate([this]() {
			mRoot.commands.run(Command::InvertSelection);
			mRoot.refreshFromState();
		});
		for (Widget* item : std::initializer_list<Widget*>{ &size,
																												&sizeMark,
																												&opacity,
																												&opacityMark,
																												&flip,
																												&flop,
																												&proportional,
																												&cancelTransform,
																												&applyTransform,
																												&deselect,
																												&invertSelection })
			activeBar.addChild(*item);

		const auto transformActive = root.canvas.onTransformActive;
		root.canvas.onTransformActive = [this, transformActive](bool active) {
			if (transformActive) transformActive(active);
			mRoot.transformBar.setVisible(false);
			refreshFromState();
		};

		configure(reveal, "", "Show controls");
		reveal.setVisible(false);
		reveal.setOnActivate([this]() { setControlsShown(true); });

		root.onStateRefreshed = [this]() { refreshFromState(); };

		addChild(root);
		for (Widget* widget : std::initializer_list<Widget*>{ &activeBar, &toolBar, &topLeft, &topRight, &reveal })
			addChild(*widget);
		sizeButtons();
		refreshFromState();
	}

	void StudioRoot::configure(Button& button, const char* icon, const char* label) {
		button.setStyle(Button::Style::Flat);
		const ImageId image = *icon ? mIcons.icon(icon) : ImageId{};
		if (image.isValid()) button.setIcon(image);
		else if (*icon) button.setText(label);
		button.setTooltip(label);
	}

	void StudioRoot::addTool(const std::string& name, const std::string& label) {
		auto button = std::make_unique<Button>();
		configure(*button, name.c_str(), label.c_str());
		button->setDebugName("studio tool " + name);
		button->setOnActivate([this, name]() { useTool(name); });
		toolBar.addChild(*button);
		mBarOrder.push_back(button.get());
		mTools.emplace_back(name, std::move(button));
	}

	void StudioRoot::addGroup(
		const std::string& name, const std::string& label, const std::vector<std::pair<std::string, std::string>>& tools
	) {
		auto group = std::make_unique<StudioToolGroup>();
		group->name = name;
		group->label = label;
		group->current = tools.front().first;
		group->button = std::make_unique<PopupButton>();
		PopupButton& button = *group->button;
		configure(button, tools.front().first.c_str(), label.c_str());
		button.setDebugName("studio group " + name);
		button.setPopupContent(&group->menu);
		button.popupOptions().placement = Placement::Above;
		button.popupOptions().align = PlacementAlign::Center;
		StudioToolGroup* owner = group.get();
		for (const auto& [tool, toolLabel] : tools) {
			group->tools.push_back(tool);
			const std::string chosen = tool;
			MenuEntry entry = MenuEntry::item(toolLabel, [this, chosen]() { useTool(chosen); });
			entry.icon = mIcons.icon(tool);
			entry.isChecked = [this, chosen]() { return isToolSelected(chosen); };
			group->menu.add(std::move(entry));
		}
		group->menu.setOnItemActivated([owner]() { owner->button->closePopup(); });
		button.setOnOpening([owner]() { owner->menu.refresh(); });
		toolBar.addChild(button);
		mBarOrder.push_back(&button);
		mGroups.push_back(std::move(group));
	}

	StudioToolGroup* StudioRoot::group(const std::string& name) {
		for (auto& group : mGroups)
			if (group->name == name) return group.get();
		return nullptr;
	}

	StudioToolGroup* StudioRoot::groupOf(const std::string& tool) {
		for (auto& group : mGroups)
			if (std::find(group->tools.begin(), group->tools.end(), tool) != group->tools.end()) return group.get();
		return nullptr;
	}

	bool StudioRoot::isToolSelected(const std::string& tool) const { return mRoot.toolStrip.button(tool).isSelected(); }

	void StudioRoot::showPreview(SliderGlyph glyph, bool shown) {
		const BrushParameter parameter = glyph == SliderGlyph::Size ? BrushParameter::Size : BrushParameter::Opacity;
		mRoot.canvas.setBrushPreview(shown ? std::optional<BrushParameter>(parameter) : std::nullopt);
	}

	void StudioRoot::useTool(const std::string& name) {
		if (StudioToolGroup* owner = groupOf(name)) {
			owner->current = name;
			const ImageId icon = mIcons.icon(name);
			if (icon.isValid()) owner->button->setIcon(icon);
			owner->button->setTooltip(owner->label);
		}
		mRoot.toolStrip.button(name).activate();
		mRoot.refreshFromState();
		refreshFromState();
	}

	Button& StudioRoot::toolButton(const std::string& name) {
		for (auto& tool : mTools) {
			if (tool.first == name) return *tool.second;
		}
		for (auto& group : mGroups) {
			if (group->name == name) return *group->button;
		}
		if (StudioToolGroup* owner = groupOf(name)) return *owner->button;
		return *mTools.front().second;
	}

	std::vector<Button*> StudioRoot::toolButtons() { return mBarOrder; }

	std::vector<Button*> StudioRoot::barButtons() {
		return { &hide, &library, &undo, &redo, &settingsButton, &presets, &brushOptions, &color, &layers };
	}

	void StudioRoot::sizeButtons() {
		const halnf button = buttonSize();
		for (Button* item : barButtons()) {
			item->setSizing(AxisSizing::fixed(mTopButton), AxisSizing::fixed(mTopButton));
			item->setIconSize(mTopButton * kIconFraction);
			item->setCornerRadius(mTopButton * 0.35f);
		}
		reveal.setCornerRadius(mTopButton * 0.35f);
		for (SliderGlyphMark* mark : { &sizeMark, &opacityMark })
			mark->setExtent(kMarkSize * mScale);
		for (Button* item : toolButtons()) {
			item->setSizing(AxisSizing::fixed(mToolButton), AxisSizing::fixed(mToolButton));
			item->setIconSize(mToolButton * kIconFraction);
			item->setCornerRadius(mToolButton * 0.35f);
		}
		for (Button* item : std::initializer_list<Button*>{
					 &flip, &flop, &proportional, &cancelTransform, &applyTransform, &deselect, &invertSelection })
			item->setSizing(AxisSizing::fixed(button * 1.6f), AxisSizing::fixed(button * 0.8f));
		for (Slider* slider : { &size, &opacity })
			slider->setSizing(AxisSizing::fit(), AxisSizing::fixed(mSliderLength));
		for (Panel* panel : { &topLeft, &topRight, &toolBar, &activeBar })
			panel->setCornerRadius(kCornerRadius * mScale);
		for (PopupButton* popup : std::initializer_list<PopupButton*>{ &presets, &brushOptions, &color, &layers })
			popup->popupOptions().gap = kPopupGap * mScale;
		requestLayout();
	}

	void StudioRoot::fitToolBar(halnf available) {
		const halnf count = (halnf) mBarOrder.size();
		const halnf fitting = (available - 2 * kBarPadding - (count - 1) * kBarGap) / count;
		const halnf next = std::clamp(fitting, std::min(kSmallestButton * mScale, buttonSize()), buttonSize());
		if (next == mToolButton) return;
		mToolButton = next;
		sizeButtons();
	}

	void StudioRoot::orientToolBar(bool vertical) {
		if (vertical == mToolBarVertical) return;
		mToolBarVertical = vertical;
		BoxLayout& box = vertical ? (BoxLayout&) toolBar.emplaceLayout<ColumnLayout>(kBarGap)
															: (BoxLayout&) toolBar.emplaceLayout<RowLayout>(kBarGap);
		box.setPadding(Insets::uniform(kBarPadding));
		for (const auto& owner : mGroups)
			owner->button->popupOptions().placement = vertical ? Placement::Left : Placement::Above;
		requestLayout();
	}

	void StudioRoot::fitTopBars(halnf available) {
		const halnf count = (halnf) (visibleChildren(topLeft) + visibleChildren(topRight));
		const halnf fixed = 4 * kBarPadding + (count - 2) * kBarGap;
		const halnf both = (available - fixed) / count;
		const halnf next = std::clamp(both, std::min(16.0f, buttonSize()), buttonSize());
		if (next == mTopButton) return;
		mTopButton = next;
		sizeButtons();
	}

	void StudioRoot::fitSliders(halnf available) {
		const halnf sliders = (halnf) (size.isVisible() + opacity.isVisible());
		if (sliders == 0) return;
		const halnf marks = sliders * kMarkSize * mScale;
		const halnf gaps = (2 * sliders - 1) * kBarGap * 4;
		const halnf fitting = (available - 4 * kBarPadding - marks - gaps) / sliders;
		const halnf next =
			std::clamp(fitting, std::min(kSmallestSlider * mScale, std::max(fitting, 16.0f)), kSliderLength * mScale);
		if (next == mSliderLength) return;
		mSliderLength = next;
		sizeButtons();
	}

	bool StudioRoot::wantsActiveBar() const {
		return areControlsShown() && !mPortrait && mActivePanel != ActivePanel::None;
	}

	void StudioRoot::setScale(halnf scale) {
		mScale = scale;
		mSliderLength = kSliderLength * scale;
		mToolButton = buttonSize();
		mTopButton = buttonSize();
		sizeButtons();
		applyInteractionMode();
	}

	ActivePanel StudioRoot::panelFor() const {
		switch (mRoot.canvas.tool()) {
			case CanvasTool::Paint: return ActivePanel::Brush;
			case CanvasTool::Bucket:
			case CanvasTool::Gradient: return ActivePanel::Fill;
			case CanvasTool::Transform: return ActivePanel::Transform;
			case CanvasTool::Marquee:
			case CanvasTool::Lasso:
			case CanvasTool::Wand: return ActivePanel::Selection;
			default: return ActivePanel::None;
		}
	}

	void StudioRoot::refreshActiveBar() {
		const ActivePanel panel = panelFor();
		const bool changed = panel != mActivePanel;
		mActivePanel = panel;
		size.setVisible(panel == ActivePanel::Brush);
		opacity.setVisible(panel == ActivePanel::Brush || panel == ActivePanel::Fill);
		sizeMark.setVisible(size.isVisible());
		opacityMark.setVisible(opacity.isVisible());
		for (Button* button :
				 std::initializer_list<Button*>{ &flip, &flop, &proportional, &cancelTransform, &applyTransform })
			button->setVisible(panel == ActivePanel::Transform);
		deselect.setVisible(panel == ActivePanel::Selection);
		invertSelection.setVisible(panel == ActivePanel::Selection);
		activeBar.setVisible(wantsActiveBar());
		if (changed) requestLayout();

		size.setRange(1, mBrush.maximumSize());
		if (!size.isDragging()) size.setValue(mBrush.size());
		if (!opacity.isDragging()) opacity.setValue(mBrush.opacity());
		if (mRoot.canvas.brushPreview()) mRoot.canvas.requestPaint();
		proportional.setSelected(mRoot.transformBar.proportional.isSelected());
		deselect.setEnabled(mRoot.commands.isEnabled(Command::Deselect));
	}

	void StudioRoot::refreshFromState() {
		undo.setEnabled(mRoot.commands.isEnabled(Command::Undo));
		redo.setEnabled(mRoot.commands.isEnabled(Command::Redo));
		for (auto& tool : mTools)
			tool.second->setSelected(isToolSelected(tool.first));
		for (auto& owner : mGroups) {
			bool selected = false;
			for (const std::string& tool : owner->tools) {
				if (!isToolSelected(tool)) continue;
				selected = true;
				if (owner->current != tool) {
					owner->current = tool;
					const ImageId icon = mIcons.icon(tool);
					if (icon.isValid()) owner->button->setIcon(icon);
				}
			}
			owner->button->setSelected(selected);
		}
		color.requestPaint();
		refreshActiveBar();
	}

	void StudioRoot::setControlsShown(bool shown) {
		for (Panel* panel : { &topLeft, &topRight, &toolBar })
			panel->setVisible(shown);
		reveal.setVisible(!shown);
		if (!shown) mRoot.canvas.setBrushPreview(std::nullopt);
		refreshActiveBar();
		requestLayout();
	}

	StudioRoot::~StudioRoot() {
		mRoot.isDesktopLayout = {};
		mRoot.setDesktopLayout = {};
	}

	bool StudioRoot::isMenuBarShown() const { return mRoot.menuBar.isVisible(); }

	bool StudioRoot::isTouchMode() const { return !ui() || ui()->isTouchMode(); }

	void StudioRoot::applyInteractionMode() {
		const bool touch = isTouchMode();
		mRoot.menuBar.setVisible(!touch);
		settingsButton.setVisible(touch);
		if (ui()) ui()->setTheme(touchTheme(mScale));
		requestLayout();
	}

	void StudioRoot::onInteractionModeChanged() { applyInteractionMode(); }

	size_t StudioRoot::visibleChildren(const ui::Widget& bar) {
		size_t count = 0;
		for (const ui::Widget* child : bar.children())
			count += child->isVisible() ? 1 : 0;
		return count;
	}

	RectF StudioRoot::controlArea(const RectF& area) const {
		const WindowHost* host = ui() ? ui()->windowHost() : nullptr;
		if (!host) return area;
		const EdgeInsets safe = host->platformInfo().safeInsets;
		return makeRect(
			{ area.pos.x + safe.left, area.pos.y + safe.top },
			{ area.size.x - safe.left - safe.right, area.size.y - safe.top - safe.bottom }
		);
	}

	void StudioRoot::onAttached() { setScale(mScale); }

	void StudioRoot::onEvent(Event& event) {
		if (event.type() != Event::Type::KeyDown || event.keyCode() != InputID::ESCAPE || event.isAccepted()) return;
		if (ui()) mRoot.library.open(*ui());
		event.accept();
	}
}
