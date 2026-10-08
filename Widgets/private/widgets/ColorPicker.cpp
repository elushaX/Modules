#include "ColorPicker.hpp"

#include "BoxLayout.hpp"
#include "StackLayout.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kChipInset = 5;
	constexpr halnf kPopupPadding = 8;
	constexpr halnf kCellGap = 6;
	constexpr halnf kCellRadius = 5;
	constexpr halnf kWheelSize = 180;
	constexpr halnf kPlusArm = 6;
}

ColorCell::ColorCell(Kind kind, RGBA color) :
	mKind(kind),
	mColor(color) {
	setDebugName(kind == Kind::Add ? "add color cell" : "color cell");
	setTooltip(kind == Kind::Add ? "Add new color" : std::string());
	setGeometryAnimated(false);
}

void ColorCell::setSelected(bool selected) {
	if (mSelected == selected) return;
	mSelected = selected;
	requestPaint();
}

Extent ColorCell::measureContent(Axis, halnf) const { return Extent::rigid(kSize); }

void ColorCell::paint(Painter& painter) {
	const Palette& palette = theme().palette;
	const RectF cell = localRect();

	if (mKind == Kind::Add) {
		painter.fillRect(cell, background(), kCellRadius);
		painter.strokeRect(cell, palette.controlBorder, kCellRadius, 1);
		const Vec2F center = cell.center();
		painter.line(center - Vec2F(kPlusArm, 0.0f), center + Vec2F(kPlusArm, 0.0f), palette.text, 1.5f);
		painter.line(center - Vec2F(0.0f, kPlusArm), center + Vec2F(0.0f, kPlusArm), palette.text, 1.5f);
		return;
	}

	painter.fillRect(cell, mColor, kCellRadius);
	if (mSelected) painter.strokeRect(insetRect(cell, 1), palette.text, kCellRadius, 2);
	else if (isHovered()) painter.strokeRect(cell, palette.mutedText, kCellRadius, 1);
}

ColorPickerButton::ColorPickerButton() {
	setDebugName("color picker");

	PopupOptions options;
	options.placement = Placement::Below;
	options.gap = Theme::standard().metrics.popupGap;
	setPopupOptions(options);
	setPopupContent(&mPopup);

	buildPalettePopup();
	buildNewColorPopup();
}

void ColorPickerButton::buildPalettePopup() {
	mPopup.setBorder(true);
	mPopup.emplaceLayout<StackLayout>().setPadding(Insets::uniform(kPopupPadding));

	mGrid.setClipsChildren(true);
	mGrid.emplaceLayout<GridLayout>(kColumns, kCellGap).setCellSize(Vec2F(ColorCell::kSize, ColorCell::kSize));
	mAddCell.setOnActivate([this] { openNewColorEditor(); });

	mPopup.addChild(mGrid);
	rebuildCells();
}

void ColorPickerButton::buildNewColorPopup() {
	mNewColorPopup.setBorder(true);
	mNewColorPopup.emplaceLayout<ColumnLayout>(Theme::standard().metrics.spacing)
		.setPadding(Insets::uniform(kPopupPadding));

	mWheel.setSizing(AxisSizing::fixed(kWheelSize), AxisSizing::fixed(kWheelSize));
	mConfirmNewColor.setStyle(Button::Style::Accent);
	mNewColorActions.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mNewColorActions.emplaceLayout<RowLayout>(Theme::standard().metrics.spacing).setMainAlign(MainAlign::End);
	mNewColorActions.addChild(mCancelNewColor);
	mNewColorActions.addChild(mConfirmNewColor);

	mCancelNewColor.setOnActivate([this] { closeNewColorEditor(); });
	mConfirmNewColor.setOnActivate([this] { addNewColor(); });

	mNewColorPopup.addChild(mWheel);
	mNewColorPopup.addChild(mNewColorActions);
}

void ColorPickerButton::rebuildCells() {
	mGrid.removeAllChildren();
	mSwatches.clear();

	for (const RGBA& color : mPalette) {
		auto cell = std::make_unique<ColorCell>(ColorCell::Kind::Swatch, color);
		cell->setOnActivate([this, color] { pick(color); });
		mGrid.addChild(*cell);
		mSwatches.push_back(std::move(cell));
	}
	mGrid.addChild(mAddCell);
	refreshSelection();
}

void ColorPickerButton::setPalette(std::vector<RGBA> colors) {
	mPalette = std::move(colors);
	rebuildCells();
}

void ColorPickerButton::setFallbackColor(const RGBA& color) {
	mFallbackColor = color;
	requestPaint();
}

void ColorPickerButton::setColor(std::optional<RGBA> color) {
	mColor = color;
	refreshSelection();
	requestPaint();
}

void ColorPickerButton::openNewColorEditor() {
	if (!ui()) return;

	mWheel.setColor(shownColor());

	PopupRequest request;
	request.content = &mNewColorPopup;
	request.owner = this;
	request.anchor = windowRect();
	request.options.placement = Placement::Below;
	request.options.gap = theme().metrics.popupGap;

	closePopup();
	ui()->openPopup(request);
}

void ColorPickerButton::closeNewColorEditor() {
	if (isNewColorEditorOpen()) ui()->closePopup(mNewColorPopup);
}

bool ColorPickerButton::isNewColorEditorOpen() const { return ui() && ui()->isPopupOpen(mNewColorPopup); }

void ColorPickerButton::addNewColor() {
	const RGBA color = mWheel.color();
	if (std::find(mPalette.begin(), mPalette.end(), color) == mPalette.end()) {
		mPalette.push_back(color);
		rebuildCells();
		if (mOnPaletteChanged) mOnPaletteChanged(mPalette);
	}

	closeNewColorEditor();
	pick(color);
}

void ColorPickerButton::pick(const RGBA& color) {
	setColor(color);
	closePopup();
	if (mOnPicked) mOnPicked(color);
}

void ColorPickerButton::refreshSelection() {
	for (const std::unique_ptr<ColorCell>& cell : mSwatches)
		cell->setSelected(mColor && *mColor == cell->color());
}

Extent ColorPickerButton::measureContent(Axis axis, halnf) const {
	const halnf height = theme().metrics.controlHeight;
	return Extent::rigid(axis == Axis::Horizontal ? height * 1.6f : height);
}

void ColorPickerButton::paint(Painter& painter) {
	const Theme& style = theme();
	painter.fillRect(localRect(), background(), style.metrics.cornerRadius);
	painter.strokeRect(localRect(), style.palette.controlBorder, style.metrics.cornerRadius, style.metrics.borderWidth);
	painter.fillRect(insetRect(localRect(), kChipInset), shownColor(), style.metrics.indicatorCornerRadius);
}
